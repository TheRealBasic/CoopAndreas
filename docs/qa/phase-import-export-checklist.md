# Phase QA Checklist — Import/Export

## Scope
Validate co-op parity for **Import/Export** wanted lists and delivery turn-ins.

## Required opcode/command coverage
- `Mission.LoadAndLaunchInternal`
- `create_car`
- `set_objective`
- `set_player_money`
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

## Import/Export phase evidence
| Mode | Objective parity | Fail/pass parity | Reconnect parity | Late-join parity | Evidence (clips/logs) | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Import/Export wanted lists + turn-ins | ✅ | ✅ | ✅ | ✅ | `qa-captures/import-export/objective-parity.mp4`, `qa-captures/import-export/fail-pass-parity.mp4`, `qa-captures/import-export/reconnect-parity.mp4`, `qa-captures/import-export/late-join-parity.mp4`, `qa-logs/import-export/session-91dc43e.log` | Delivery state (`currentList`, `turnInIndex`, payout latch) now persists through reconnect and hydrates for late join under host authority. |
