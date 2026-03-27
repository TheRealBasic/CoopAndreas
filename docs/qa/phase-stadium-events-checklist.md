# Phase QA Checklist — Stadium events

## Scope
Validate co-op parity for stadium events: **8-Track**, **Blood Bowl**, **Dirt Track**, **Kick Start**.

## Required opcode/command coverage
- `Mission.LoadAndLaunchInternal`
- `start_car_race`
- `set_car_race_checkpoint`
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

## Mode-specific execution sections
- [8-Track execution](#8-track-execution)
- [Blood Bowl execution](#blood-bowl-execution)
- [Dirt Track execution](#dirt-track-execution)
- [Kick Start execution](#kick-start-execution)

### 8-Track execution
- **Objective parity:** ✅ Pass — objective text + checkpoint progression aligned across host and peers.
- **Fail/pass parity:** ✅ Pass — fail and pass outcomes emitted once and matched on all peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active race state/timers without reset.
- **Late-join parity:** ✅ Pass — late join hydrated active race state without duplicate events.
- **Session build/hash:** `coopandreas-dev-2026-03-27 / 8f4e9a1`
- **Evidence refs:** `qa-captures/stadium/8-track/objective-parity.mp4`, `qa-captures/stadium/8-track/fail-pass-parity.mp4`, `qa-captures/stadium/8-track/reconnect-parity.mp4`, `qa-captures/stadium/8-track/late-join-parity.mp4`, `qa-logs/stadium/8-track/session-8f4e9a1.log`
- **Gate status:** `done` (all four parity gates passed).

### Blood Bowl execution
- **Objective parity:** ✅ Pass — objective text + checkpoint progression aligned across host and peers.
- **Fail/pass parity:** ✅ Pass — fail and pass outcomes emitted once and matched on all peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active race state/timers without reset.
- **Late-join parity:** ✅ Pass — late join hydrated active race state without duplicate events.
- **Session build/hash:** `coopandreas-dev-2026-03-27 / 8f4e9a1`
- **Evidence refs:** `qa-captures/stadium/blood-bowl/objective-parity.mp4`, `qa-captures/stadium/blood-bowl/fail-pass-parity.mp4`, `qa-captures/stadium/blood-bowl/reconnect-parity.mp4`, `qa-captures/stadium/blood-bowl/late-join-parity.mp4`, `qa-logs/stadium/blood-bowl/session-8f4e9a1.log`
- **Gate status:** `done` (all four parity gates passed).

### Dirt Track execution
- **Objective parity:** ✅ Pass — objective text + checkpoint progression aligned across host and peers.
- **Fail/pass parity:** ✅ Pass — fail and pass outcomes emitted once and matched on all peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active race state/timers without reset.
- **Late-join parity:** ✅ Pass — late join hydrated active race state without duplicate events.
- **Session build/hash:** `coopandreas-dev-2026-03-27 / 8f4e9a1`
- **Evidence refs:** `qa-captures/stadium/dirt-track/objective-parity.mp4`, `qa-captures/stadium/dirt-track/fail-pass-parity.mp4`, `qa-captures/stadium/dirt-track/reconnect-parity.mp4`, `qa-captures/stadium/dirt-track/late-join-parity.mp4`, `qa-logs/stadium/dirt-track/session-8f4e9a1.log`
- **Gate status:** `done` (all four parity gates passed).

### Kick Start execution
- **Objective parity:** ✅ Pass — objective text + checkpoint progression aligned across host and peers.
- **Fail/pass parity:** ✅ Pass — fail and pass outcomes emitted once and matched on all peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active race state/timers without reset.
- **Late-join parity:** ✅ Pass — late join hydrated active race state without duplicate events.
- **Session build/hash:** `coopandreas-dev-2026-03-27 / 8f4e9a1`
- **Evidence refs:** `qa-captures/stadium/kick-start/objective-parity.mp4`, `qa-captures/stadium/kick-start/fail-pass-parity.mp4`, `qa-captures/stadium/kick-start/reconnect-parity.mp4`, `qa-captures/stadium/kick-start/late-join-parity.mp4`, `qa-logs/stadium/kick-start/session-8f4e9a1.log`
- **Gate status:** `done` (all four parity gates passed).
