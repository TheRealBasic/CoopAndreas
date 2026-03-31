# Phase QA Checklist — Asset missions

## Scope
Validate co-op parity for asset missions: **Trucker**, **Valet**, **Career**.

## Required opcode/command coverage
- `Mission.LoadAndLaunchInternal`
- `set_objective`
- `set_timers`
- `register_mission_passed`
- `fail_current_mission`
- mode-specific objective opcodes (`task_car_drive_to_coord`, `set_player_money`, `set_player_control`)

## Required state transitions
- `idle -> start`
- `start -> objective_active`
- `objective_active -> delivery_progress`
- `delivery_progress -> pass`
- `delivery_progress -> fail`
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

## Asset mission execution sections

### Trucker execution
- **Objective parity:** ✅ Pass — cargo objective and delivery progression matched host for all peers.
- **Fail/pass parity:** ✅ Pass — pass/fail outcomes emitted once across peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active cargo run state without reset.
- **Late-join parity:** ✅ Pass — late join hydrated active run state without duplicate mission events.
- **Session build/hash:** `coopandreas-dev-2026-03-31 / 44b9a6f`
- **Evidence refs:** `qa-captures/assets/trucker/objective-parity.mp4`, `qa-captures/assets/trucker/fail-pass-parity.mp4`, `qa-captures/assets/trucker/reconnect-parity.mp4`, `qa-captures/assets/trucker/late-join-parity.mp4`, `qa-logs/assets/trucker/session-44b9a6f.log`
- **Gate status:** `done`.

### Valet execution
- **Objective parity:** ✅ Pass — parking objective progression and stage text matched host for all peers.
- **Fail/pass parity:** ✅ Pass — pass/fail outcomes emitted once across peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active valet stage without reset.
- **Late-join parity:** ✅ Pass — late join hydrated active valet stage without duplicate mission events.
- **Session build/hash:** `coopandreas-dev-2026-03-31 / 44b9a6f`
- **Evidence refs:** `qa-captures/assets/valet/objective-parity.mp4`, `qa-captures/assets/valet/fail-pass-parity.mp4`, `qa-captures/assets/valet/reconnect-parity.mp4`, `qa-captures/assets/valet/late-join-parity.mp4`, `qa-logs/assets/valet/session-44b9a6f.log`
- **Gate status:** `done`.

### Career execution
- **Objective parity:** ✅ Pass — progression objective + checkpointing matched host for all peers.
- **Fail/pass parity:** ✅ Pass — pass/fail outcomes emitted once across peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active career attempt state without reset.
- **Late-join parity:** ✅ Pass — late join hydrated active career attempt without duplicate mission events.
- **Session build/hash:** `coopandreas-dev-2026-03-31 / 44b9a6f`
- **Evidence refs:** `qa-captures/assets/career/objective-parity.mp4`, `qa-captures/assets/career/fail-pass-parity.mp4`, `qa-captures/assets/career/reconnect-parity.mp4`, `qa-captures/assets/career/late-join-parity.mp4`, `qa-logs/assets/career/session-44b9a6f.log`
- **Gate status:** `done`.
