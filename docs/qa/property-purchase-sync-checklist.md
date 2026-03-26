# Property purchase sync parity checklist

## Script inventory (buy points + ownership flags)

### Ownership flags observed in `scm/scripts/*.txt`
- `$already_bought_house[0..31]` initialization/reset in `INITIAL.txt` (32 property ownership bits).  
- `$zeros_property_bought` set in `BUYPRO1.txt` and checked in `MOB_SF.txt` for Zero's asset progression gating.

### Property buy pickup slots observed in scripts
- `save_housepickup[0..31]` is the canonical buy/save pickup array.
- Initial state (`INITIAL.txt` + `MAIN.txt`): slots `0..2` are locked asset properties, and mixed locked/for-sale setup for `3..31`.
- Unlock promotions (`OPENUP.txt`): converts additional locked slots to `Pickup.CreateForSaleProperty('PROP_3', ...)`.
- Entry/exit linkage is managed with `World.SetClosestEntryExitFlag(...)` calls tied to the same `propertyX/Y/Z` slots.

## Test matrix

### 1) Purchase propagation (host-authoritative)
1. Start with two peers connected and ensure both can see the same for-sale marker for one target slot.
2. Host purchases the property.
3. Verify non-host immediately observes:
   - ownership state changed,
   - pickup/interior linkage state updated (for-sale marker removed/converted),
   - no local rollback after a few seconds.
4. Repeat with a second property in a different area/interior.

### 2) Rejoin parity
1. After at least two purchases, disconnect non-host.
2. Continue gameplay on host for 30+ seconds.
3. Reconnect non-host.
4. Verify reconnect snapshot restores identical ownership/unlock/pickup-interior linkage state.

### 3) Late-join parity
1. Keep existing session running after purchases.
2. Join with a fresh third peer (no prior state).
3. Verify late joiner receives full property snapshot immediately on connect (no temporary for-sale rollback visuals).

### 4) Host-migration parity
1. Purchase at least one property.
2. Force host migration (original host leaves).
3. Verify remaining peers keep identical property ownership state post-migration.
4. Verify newly assigned host can continue updates without reverting previously purchased properties.

## Pass/Fail
- **PASS:** ownership flags, unlocked state, and linked pickup/interior state remain identical across connected peers, rejoiners, late joiners, and post-migration peers.
- **FAIL:** any purchased property reverts to for-sale/locked for any peer, or diverges after reconnect/host migration.

## Execution record

- **Date (UTC):** 2026-03-26
- **Build/commit:** `1919137c80dc63ddc63bb5ce4f1ac710f80109a7` (repo HEAD)
- **Peer count exercised:** 0 (blocked: runtime environment does not include GTA:SA client/runtime needed for multi-peer end-to-end session)
- **Case IDs:**
  - `PP-01` Basic buy propagation — **FAIL (blocked by environment; not executable in this container)**
  - `PP-02` Reconnect parity — **FAIL (blocked by environment; not executable in this container)**
  - `PP-03` Late-join snapshot parity — **FAIL (blocked by environment; not executable in this container)**
  - `PP-04` Host-migration parity — **FAIL (blocked by environment; not executable in this container)**
- **Passed case IDs:** none
- **Notes:** Attempted to prepare a runnable build environment, but `xmake` is unavailable in this container (`xmake: command not found`), and no GTA:SA runtime/assets are present to execute multiplayer parity scenarios end-to-end.

## Retry record (after installing `xmake`)

- **Date (UTC):** 2026-03-26
- **Build/commit:** `28a198c` (retry run HEAD before compile-fix patch)
- **Peer count exercised:** 0 (blocked: no GTA:SA runtime/assets or multiplayer clients in container)
- **Retry steps:**
  1. Installed `xmake` via apt.
  2. Fixed `server/src/CPlayerManager.h` compile issue in `MissionFlowSyncState` by removing in-struct default member initializers that broke this toolchain.
  3. Rebuilt with `XMAKE_ROOT=y xmake f -m release && XMAKE_ROOT=y xmake --build server`.
  4. Smoke-launched server binary with `timeout 5s ./build/linux/x86_64/release/server`.
- **Build result:** **PASS** (server target builds successfully in this container after the header fix).
- **Case IDs:**
  - `PP-01` Basic buy propagation — **FAIL (blocked by lack of GTA:SA client runtime + multi-peer session harness)**
  - `PP-02` Reconnect parity — **FAIL (blocked by lack of GTA:SA client runtime + multi-peer session harness)**
  - `PP-03` Late-join snapshot parity — **FAIL (blocked by lack of GTA:SA client runtime + multi-peer session harness)**
  - `PP-04` Host-migration parity — **FAIL (blocked by lack of GTA:SA client runtime + multi-peer session harness)**
- **Passed case IDs:** none
