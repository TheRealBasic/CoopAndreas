# Property purchase sync parity checklist

## Scope
Validate host-authoritative property purchase synchronization for the four required parity gates:
1. purchase flow propagation,
2. reconnect restore,
3. late-join hydration,
4. post-host-migration state consistency.

## Script inventory (buy points + ownership flags)

### Ownership flags observed in `scm/scripts/*.txt`
- `$already_bought_house[0..31]` initialization/reset in `INITIAL.txt` (32 property ownership bits).
- `$zeros_property_bought` set in `BUYPRO1.txt` and checked in `MOB_SF.txt` for Zero's asset progression gating.

### Property buy pickup slots observed in scripts
- `save_housepickup[0..31]` is the canonical buy/save pickup array.
- Initial state (`INITIAL.txt` + `MAIN.txt`): slots `0..2` are locked asset properties, and mixed locked/for-sale setup for `3..31`.
- Unlock promotions (`OPENUP.txt`): converts additional locked slots to `Pickup.CreateForSaleProperty('PROP_3', ...)`.
- Entry/exit linkage is managed with `World.SetClosestEntryExitFlag(...)` calls tied to the same `propertyX/Y/Z` slots.

## Scenario matrix

| Scenario ID | Parity gate | Steps | Expected | Observed |
| --- | --- | --- | --- | --- |
| `PP-01` | Purchase flow propagation | 1) Connect host + at least one client. 2) Confirm both peers see same target for-sale pickup. 3) Host purchases property. 4) Repeat on a second property in a different area/interior. | Non-host peers immediately converge on host-authoritative ownership bit, pickup conversion/removal, and interior linkage updates with no rollback after several seconds. | **Blocked in container**: GTA:SA runtime and multiplayer clients are unavailable, so end-to-end peer observation cannot be executed here. |
| `PP-02` | Reconnect restore | 1) After at least two purchases, disconnect one client. 2) Keep host running for 30+ seconds. 3) Reconnect client. | Reconnecting peer restores identical ownership, unlocked state, and pickup/interior linkage snapshot from host with no stale for-sale markers. | **Blocked in container**: no GTA:SA runtime/assets and no reconnect-capable client harness in this environment. |
| `PP-03` | Late-join hydration | 1) Keep a session running after purchases. 2) Join with a fresh peer that had no prior state. | Late joiner hydrates full property snapshot immediately on connect; no temporary rollback to for-sale/locked state. | **Blocked in container**: no GTA:SA runtime/assets and no multi-peer late-join harness in this environment. |
| `PP-04` | Post-host-migration state consistency | 1) Purchase at least one property. 2) Force host migration (original host leaves). 3) Continue session with new host. | Remaining peers keep identical purchased-property state after migration, and new host continues authoritative updates without reverting previously purchased properties. | **Blocked in container**: host-migration scenario cannot be executed without full multiplayer runtime. |

## Pass/Fail criteria
- **PASS:** all four scenario IDs (`PP-01..PP-04`) satisfy their expected results with no ownership/pickup/interior divergence.
- **FAIL:** any scenario shows rollback, stale for-sale state, missing snapshot hydrate, or post-migration divergence.

## Execution record

- **Date (UTC):** 2026-03-30
- **Build/commit:** `28a198c` (repo HEAD observed during latest QA artifact refresh)
- **Peer count exercised:** 0 (blocked: runtime environment does not include GTA:SA client/runtime needed for multi-peer end-to-end session)
- **Scenario outcomes:**
  - `PP-01` Purchase flow propagation — **FAIL (blocked by environment; not executable in this container)**
  - `PP-02` Reconnect restore — **FAIL (blocked by environment; not executable in this container)**
  - `PP-03` Late-join hydration — **FAIL (blocked by environment; not executable in this container)**
  - `PP-04` Post-host-migration state consistency — **FAIL (blocked by environment; not executable in this container)**
- **Passed scenario IDs:** none
- **Notes:** This checklist now explicitly captures all required parity gates and expected/observed fields. Runtime execution remains blocked by missing GTA:SA client/assets and lack of a multi-peer harness in the container.
