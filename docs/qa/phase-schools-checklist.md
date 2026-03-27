# Phase QA Checklist — Schools

## Scope
Validate co-op parity for school modes: **Driving**, **Flight**, **Bike**, **Boat**.

## Required opcode/command coverage
- `Mission.LoadAndLaunchInternal`
- `set_player_control`
- `set_objective`
- `set_timers`
- `register_mission_passed`
- `fail_current_mission`

## Required state transitions
- `idle -> start`
- `start -> objective_active`
- `objective_active -> checkpoint_progress`
- `checkpoint_progress -> pass`
- `checkpoint_progress -> fail`
- `active_state -> reconnect_restore`
- `active_state -> late_join_restore`

## Flight school hydration checkpoints (active attempt restore)
Validate that replicated mission-flow state updates include and preserve:
- `Mission.LoadAndLaunchInternal` → `missionId` reset + active attempt baseline.
- `set_player_control` → `playerControlState`.
- `set_objective` → `objective` (GXT key) and monotonic `checkpointIndex`.
- `set_timers`/timer controls → `timerVisible`, `timerFrozen`, `timerMs`.
- `register_mission_passed` / `fail_current_mission` → `passFailPending` (`1 = pass`, `2 = fail`).

Expected reconnect/late-join behavior while Flight school attempt is still active:
1. Joiner receives the latest mission-flow replay packet with current `objective`, `timer*` fields, and `checkpointIndex`.
2. Hydration restores in-flight state only (no restart of attempt and no objective rewind).
3. `passFailPending` remains `0` during steady active play, and only flips on terminal opcode emission.
4. Terminal pass/fail UI event is applied once per outcome (no duplicate pass/fail prints during hydration replay).

## Reusable checklist template
### Setup
1. Run one host + two clients minimum.
2. Start one mode variant from this phase.
3. Ensure one client joins late and one client reconnects during an active attempt.

### Acceptance criteria (fixed)
- [ ] **Objective parity:** objective text/checkpoint progression matches host for all peers.
- [ ] **Fail/pass parity:** fail and pass outcomes trigger once and match on all peers.
- [ ] **Reconnect parity:** reconnecting peer restores active mode state and timers without reset.
- [ ] **Late-join parity:** late-joining peer hydrates active mode state without duplicate events.
- [ ] **Flight-state parity:** `objective`, `timer*`, `checkpointIndex`, and `passFailPending` fields match host state at hydration time.

### Evidence
- Session build/hash:
- Host + clients tested:
- Clips/log references:
- Notes / regressions:

## Mode-specific evidence (Driving school)
| Mode | Objective parity | Fail/pass parity | Reconnect parity | Late-join parity | Evidence (clips/logs) | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Driving school tests (`Mission.LoadAndLaunchInternal(71)` from `TRACE`) | ☐ | ☐ | ☐ | ☐ |  |  |
