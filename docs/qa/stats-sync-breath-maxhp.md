# QA Notes: Breath + Max-Health Stat Sync

## Scope
Validate synced HUD-bar stat behavior for remote players when `STAT_LUNG_CAPACITY`, `STAT_STAMINA`, and `STAT_MAX_HEALTH` are replicated via `PLAYER_STATS`.

## Preconditions
- 2+ connected players in a session.
- One player can host-migrate scenario (original host leaves).
- Testers can force underwater breathing and health pickups.

## Scenario A: Breath depletion and recovery
1. Player A raises lung capacity (if needed), then goes underwater long enough to deplete breath.
2. Player B observes Player A over multiple frames while Player A is underwater.
3. Player A surfaces and recovers breath.
4. Verify Player B sees stable breath-bar behavior (no flicker/reset to local player values).
5. Verify no stat-sync spam loop appears in logs while Player B is only observing.

### Expected
- Remote context uses Player A stat values for breath HUD calculations each frame.
- Once local context is restored, Player B local stats remain unchanged.
- No repeated rebroadcast caused by remotely-applied stat changes.

## Scenario B: Max HP pickup behavior
1. Player A collects max-health-affecting pickup(s) and modifies stamina where applicable.
2. Player B observes Player A health bar before and after the pickup.
3. Repeat with Player A taking damage after pickup to confirm new max baseline is honored.

### Expected
- Player B sees Player A HUD bar scale update correctly for new max health.
- New max health/stamina values persist in replicated stats payload.

## Scenario C: Reconnect snapshot
1. Player A updates max-health/breath-related stats.
2. Player B disconnects and reconnects.
3. On reconnect, verify snapshot-forwarded `PLAYER_STATS` includes the expanded synced stat list.

### Expected
- Player B immediately receives Player A current max-health and breath-related values after reconnect.
- No rollback to defaults until Player A legitimately changes the stat again.

## Scenario D: Host migration
1. Ensure Player A has non-default max-health/breath-related stats.
2. Current host leaves so host migrates.
3. Remaining players continue gameplay; Player A performs breath depletion/recovery and health pickup actions again.

### Expected
- Wanted-level reset flows do not overwrite or clear max-health/breath stat fields.
- Post-migration stat updates continue forwarding with correct payload size.
