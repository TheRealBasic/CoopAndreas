#include "persistence/SnapshotPersistence.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "CPacket.h"
#include "CNetwork.h"
#include "CMissionRuntimeManager.h"
#include "CPedManager.h"
#include "CPlayerManager.h"
#include "CPickupManager.h"
#include "CVehicleManager.h"

#include <sqlite3.h>

bool CSnapshotPersistence::Initialize(const std::filesystem::path& rootPath, const std::string& defaultSessionId)
{
    ms_rootPath = rootPath;
    ms_blobPath = ms_rootPath / "blobs";
    ms_dbPath = ms_rootPath / "snapshot.sqlite3";
    ms_defaultSessionId = defaultSessionId;

    std::error_code ec;
    std::filesystem::create_directories(ms_blobPath, ec);
    if (ec)
    {
        printf("[Persistence] Failed to create persistence path: %s\n", ec.message().c_str());
        return false;
    }

    ms_initialized = EnsureSchema();
    if (!ms_initialized)
    {
        return false;
    }

    ms_lastAutosaveAtMs = NowUnixMs();
    printf("[Persistence] Initialized at %s\n", ms_dbPath.string().c_str());
    return true;
}

bool CSnapshotPersistence::EnsureSchema()
{
    sqlite3* db = nullptr;
    if (sqlite3_open(ms_dbPath.string().c_str(), &db) != SQLITE_OK)
    {
        printf("[Persistence] sqlite open failed: %s\n", sqlite3_errmsg(db));
        if (db != nullptr)
        {
            sqlite3_close(db);
        }
        return false;
    }

    const char* sql =
        "PRAGMA journal_mode=WAL;"
        "PRAGMA synchronous=FULL;"
        "CREATE TABLE IF NOT EXISTS snapshot_metadata ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " session_id TEXT NOT NULL,"
        " snapshot_version INTEGER NOT NULL,"
        " schema_version INTEGER NOT NULL,"
        " timestamp_unix_ms INTEGER NOT NULL,"
        " map_seed INTEGER NOT NULL,"
        " map_version INTEGER NOT NULL,"
        " build_version TEXT NOT NULL,"
        " build_compatibility TEXT NOT NULL,"
        " blob_path TEXT NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_snapshot_session_version"
        " ON snapshot_metadata(session_id, snapshot_version DESC, timestamp_unix_ms DESC);";

    char* errorMessage = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errorMessage);
    if (rc != SQLITE_OK)
    {
        printf("[Persistence] schema initialization failed: %s\n", errorMessage ? errorMessage : "unknown");
        sqlite3_free(errorMessage);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}

bool CSnapshotPersistence::WriteBlobAtomically(const std::string& sessionId, uint32_t version, const std::vector<uint8_t>& snapshotBlob, std::filesystem::path& outFinalPath)
{
    const auto fileName = sessionId + "_v" + std::to_string(version) + ".bin";
    outFinalPath = ms_blobPath / fileName;
    const std::filesystem::path tempPath = outFinalPath.string() + ".tmp";

    {
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            return false;
        }

        if (!snapshotBlob.empty())
        {
            out.write(reinterpret_cast<const char*>(snapshotBlob.data()), static_cast<std::streamsize>(snapshotBlob.size()));
        }

        out.flush();
        if (!out.good())
        {
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tempPath, outFinalPath, ec);
    if (ec)
    {
        std::filesystem::remove(outFinalPath, ec);
        ec.clear();
        std::filesystem::rename(tempPath, outFinalPath, ec);
        if (ec)
        {
            return false;
        }
    }

    return true;
}

bool CSnapshotPersistence::SaveSnapshot(const std::string& sessionId, const std::vector<uint8_t>& snapshotBlob, uint32_t version)
{
    if (!ms_initialized)
    {
        return false;
    }

    std::filesystem::path blobPath;
    if (!WriteBlobAtomically(sessionId, version, snapshotBlob, blobPath))
    {
        printf("[Persistence] Failed to write snapshot blob for session %s\n", sessionId.c_str());
        return false;
    }

    sqlite3* db = nullptr;
    if (sqlite3_open(ms_dbPath.string().c_str(), &db) != SQLITE_OK)
    {
        printf("[Persistence] sqlite open failed: %s\n", sqlite3_errmsg(db));
        if (db != nullptr)
        {
            sqlite3_close(db);
        }
        return false;
    }

    if (sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        sqlite3_close(db);
        return false;
    }

    const char* insertSql =
        "INSERT INTO snapshot_metadata("
        " session_id, snapshot_version, schema_version, timestamp_unix_ms,"
        " map_seed, map_version, build_version, build_compatibility, blob_path"
        ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, static_cast<int>(version));
    sqlite3_bind_int(stmt, 3, static_cast<int>(kSchemaVersion));
    sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(NowUnixMs()));
    sqlite3_bind_int64(stmt, 5, 0);
    sqlite3_bind_int(stmt, 6, 1);
    sqlite3_bind_text(stmt, 7, COOPANDREAS_VERSION, -1, SQLITE_TRANSIENT);

#if defined(_WIN32)
    const char* platform = "windows";
#else
    const char* platform = "linux";
#endif

    const std::string compatibility = BuildCompatibilityMarker();
    sqlite3_bind_text(stmt, 8, compatibility.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, blobPath.string().c_str(), -1, SQLITE_TRANSIENT);

    const bool stepOk = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (!stepOk)
    {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_close(db);
        return false;
    }

    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    ms_snapshotVersion = std::max(ms_snapshotVersion, version);
    ms_dirty = false;
    printf("[Persistence] Saved snapshot version %u for session %s (%zu bytes).\n", version, sessionId.c_str(), snapshotBlob.size());
    return true;
}

bool CSnapshotPersistence::ReadBlob(const std::filesystem::path& path, std::vector<uint8_t>& outBlob)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        return false;
    }

    in.seekg(0, std::ios::end);
    const auto size = in.tellg();
    in.seekg(0, std::ios::beg);

    if (size <= 0)
    {
        outBlob.clear();
        return true;
    }

    outBlob.resize(static_cast<size_t>(size));
    in.read(reinterpret_cast<char*>(outBlob.data()), size);
    return in.good();
}

std::optional<CSnapshotPersistence::SnapshotRecord> CSnapshotPersistence::LoadLatestSnapshot(const std::string& sessionId)
{
    if (!ms_initialized)
    {
        return std::nullopt;
    }

    sqlite3* db = nullptr;
    if (sqlite3_open(ms_dbPath.string().c_str(), &db) != SQLITE_OK)
    {
        if (db != nullptr)
        {
            sqlite3_close(db);
        }
        return std::nullopt;
    }

    const char* querySql =
        "SELECT session_id, snapshot_version, schema_version, timestamp_unix_ms,"
        " map_seed, map_version, build_version, build_compatibility, blob_path"
        " FROM snapshot_metadata"
        " WHERE session_id = ?"
        " ORDER BY snapshot_version DESC, timestamp_unix_ms DESC LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, querySql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        sqlite3_close(db);
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);

    SnapshotRecord result{};
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        found = true;
        result.sessionId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        result.snapshotVersion = static_cast<uint32_t>(sqlite3_column_int(stmt, 1));
        result.schemaVersion = static_cast<uint32_t>(sqlite3_column_int(stmt, 2));
        result.timestampUnixMs = static_cast<uint64_t>(sqlite3_column_int64(stmt, 3));
        result.mapSeed = static_cast<uint64_t>(sqlite3_column_int64(stmt, 4));
        result.mapVersion = static_cast<uint32_t>(sqlite3_column_int(stmt, 5));
        result.buildVersion = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        result.buildCompatibility = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        const std::filesystem::path blobPath(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
        if (!ReadBlob(blobPath, result.blob))
        {
            found = false;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (!found)
    {
        return std::nullopt;
    }

    ms_snapshotVersion = std::max(ms_snapshotVersion, result.snapshotVersion);
    return result;
}

void CSnapshotPersistence::MarkDirty(const char* reason)
{
    ms_dirty = true;
    (void)reason;
}

std::vector<uint8_t> CSnapshotPersistence::BuildRuntimeSnapshotBlob()
{
    std::ostringstream ss;
    ss << "{"
       << "\"player_count\":" << CPlayerManager::m_pPlayers.size() << ","
       << "\"vehicle_count\":" << CVehicleManager::m_pVehicles.size() << ","
       << "\"ped_count\":" << CPedManager::m_pPeds.size() << ","
       << "\"mission_epoch\":" << CMissionRuntimeManager::GetMissionEpoch() << ","
       << "\"mission_last_sequence\":" << CMissionRuntimeManager::GetLastSequenceId() << ","
       << "\"captured_at_unix_ms\":" << NowUnixMs()
       << "}";

    const std::string payload = ss.str();
    return std::vector<uint8_t>(payload.begin(), payload.end());
}

void CSnapshotPersistence::TickAutosave()
{
    if (!ms_initialized)
    {
        return;
    }

    const uint64_t now = NowUnixMs();
    if (!ms_dirty || now - ms_lastAutosaveAtMs < kAutosaveIntervalMs)
    {
        return;
    }

    const uint32_t nextVersion = ms_snapshotVersion + 1;
    SaveSnapshot(ms_defaultSessionId, BuildRuntimeSnapshotBlob(), nextVersion);
    ms_lastAutosaveAtMs = now;
}

void CSnapshotPersistence::SaveOnShutdown()
{
    if (!ms_initialized)
    {
        return;
    }

    const uint32_t nextVersion = ms_snapshotVersion + 1;
    SaveSnapshot(ms_defaultSessionId, BuildRuntimeSnapshotBlob(), nextVersion);
}

uint64_t CSnapshotPersistence::NowUnixMs()
{
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

std::string CSnapshotPersistence::BuildCompatibilityMarker()
{
    std::ostringstream marker;
    marker << COOPANDREAS_VERSION
           << "|schema=" << kSchemaVersion
           << "|pkt=" << PacketRegistry::PROTOCOL_VERSION;
    return marker.str();
}
