#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class CSnapshotPersistence
{
public:
    struct SnapshotRecord
    {
        std::string sessionId;
        uint32_t snapshotVersion = 0;
        uint32_t schemaVersion = 0;
        uint64_t timestampUnixMs = 0;
        uint64_t mapSeed = 0;
        uint32_t mapVersion = 0;
        std::string buildVersion;
        std::string buildCompatibility;
        std::vector<uint8_t> blob;
    };

    static bool Initialize(const std::filesystem::path& rootPath, const std::string& defaultSessionId);
    static bool SaveSnapshot(const std::string& sessionId, const std::vector<uint8_t>& snapshotBlob, uint32_t version);
    static std::optional<SnapshotRecord> LoadLatestSnapshot(const std::string& sessionId);

    static void MarkDirty(const char* reason);
    static void TickAutosave();
    static void SaveOnShutdown();
    static std::vector<uint8_t> BuildRuntimeSnapshotBlob();

private:
    static bool EnsureSchema();
    static bool WriteBlobAtomically(const std::string& sessionId, uint32_t version, const std::vector<uint8_t>& snapshotBlob, std::filesystem::path& outFinalPath);
    static bool ReadBlob(const std::filesystem::path& path, std::vector<uint8_t>& outBlob);
    static uint64_t NowUnixMs();
    static std::string BuildCompatibilityMarker();

private:
    static inline constexpr uint32_t kSchemaVersion = 1;
    static inline constexpr uint32_t kAutosaveIntervalMs = 30000;

    static inline std::filesystem::path ms_rootPath{};
    static inline std::filesystem::path ms_blobPath{};
    static inline std::filesystem::path ms_dbPath{};
    static inline std::string ms_defaultSessionId{"default-session"};
    static inline uint32_t ms_snapshotVersion = 0;
    static inline uint64_t ms_lastAutosaveAtMs = 0;
    static inline bool ms_initialized = false;
    static inline bool ms_dirty = false;
};
