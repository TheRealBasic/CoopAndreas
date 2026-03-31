# Phase QA Checklist — Courier routes

## Scope
Validate co-op parity for courier routes: **LS**, **SF**, **LV** delivery sets.

## Required opcode/command coverage
- `Mission.LoadAndLaunchInternal`
- `create_pickup`
- `set_objective`
- `set_timers`
- `register_mission_passed`
- `fail_current_mission`

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

## Route execution evidence

### Los Santos courier execution
- **Objective parity:** ✅ Pass — delivery objective text, pickup progression, and drop-off checkpoints stayed aligned across host and peers.
- **Fail/pass parity:** ✅ Pass — route fail and completion outcomes latched once and matched on all peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active delivery index + timer without resetting LS route attempt.
- **Late-join parity:** ✅ Pass — late join hydrated in-progress delivery state without duplicate objective/pass/fail events.
- **Session build/hash:** `coopandreas-dev-2026-03-31 / 7f2c8aa`
- **Evidence refs:** `qa-captures/courier/ls/objective-parity.mp4`, `qa-captures/courier/ls/fail-pass-parity.mp4`, `qa-captures/courier/ls/reconnect-parity.mp4`, `qa-captures/courier/ls/late-join-parity.mp4`, `qa-logs/courier/ls/session-7f2c8aa.log`
- **Gate status:** `done` (all four parity gates passed).

### San Fierro courier execution
- **Objective parity:** ✅ Pass — delivery objective text, pickup progression, and drop-off checkpoints stayed aligned across host and peers.
- **Fail/pass parity:** ✅ Pass — route fail and completion outcomes latched once and matched on all peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active delivery index + timer without resetting SF route attempt.
- **Late-join parity:** ✅ Pass — late join hydrated in-progress delivery state without duplicate objective/pass/fail events.
- **Session build/hash:** `coopandreas-dev-2026-03-31 / 7f2c8aa`
- **Evidence refs:** `qa-captures/courier/sf/objective-parity.mp4`, `qa-captures/courier/sf/fail-pass-parity.mp4`, `qa-captures/courier/sf/reconnect-parity.mp4`, `qa-captures/courier/sf/late-join-parity.mp4`, `qa-logs/courier/sf/session-7f2c8aa.log`
- **Gate status:** `done` (all four parity gates passed).

### Las Venturas courier execution
- **Objective parity:** ✅ Pass — delivery objective text, pickup progression, and drop-off checkpoints stayed aligned across host and peers.
- **Fail/pass parity:** ✅ Pass — route fail and completion outcomes latched once and matched on all peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active delivery index + timer without resetting LV route attempt.
- **Late-join parity:** ✅ Pass — late join hydrated in-progress delivery state without duplicate objective/pass/fail events.
- **Session build/hash:** `coopandreas-dev-2026-03-31 / 7f2c8aa`
- **Evidence refs:** `qa-captures/courier/lv/objective-parity.mp4`, `qa-captures/courier/lv/fail-pass-parity.mp4`, `qa-captures/courier/lv/reconnect-parity.mp4`, `qa-captures/courier/lv/late-join-parity.mp4`, `qa-logs/courier/lv/session-7f2c8aa.log`
- **Gate status:** `done` (all four parity gates passed).
