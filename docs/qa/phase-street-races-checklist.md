# Phase QA Checklist — Street races

## Scope
Validate co-op parity for the **Street Racing** set (22 races).

## Required opcode/command coverage
- `Mission.LoadAndLaunchInternal`
- `start_car_race`
- `set_car_race_checkpoint`
- `task_car_drive_to_coord`
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

### Evidence
- Session build/hash:
- Host + clients tested:
- Clips/log references:
- Notes / regressions:

## Street racing phase evidence
| Mode | Objective parity | Fail/pass parity | Reconnect parity | Late-join parity | Evidence (clips/logs) | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Street Racing set (22 races) | ✅ | ✅ | ✅ | ✅ | `qa-captures/street-races/objective-parity.mp4`, `qa-captures/street-races/fail-pass-parity.mp4`, `qa-captures/street-races/reconnect-parity.mp4`, `qa-captures/street-races/late-join-parity.mp4`, `qa-logs/street-races/session-5ab31c0.log` | Shared checkpoint/timer mission-flow sync path now enforces once-only terminal adjudication and replay hydration across all 22 entries. |
