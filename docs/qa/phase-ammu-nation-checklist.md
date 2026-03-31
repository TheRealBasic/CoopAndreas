# Phase QA Checklist — Ammu-Nation challenge

## Scope
Validate co-op parity for **Ammu-Nation challenge** (`Mission.LoadAndLaunchInternal(113)` from `SHOOT`/`SHRANGE`).

## Required opcode/command coverage
- `Mission.LoadAndLaunchInternal`
- `set_player_control`
- `set_objective` / `print_help` (`ANR_*` objective/help keys)
- `set_timers`
- `add_score` (terminal cash/reward grant)
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
2. Host starts the Ammu-Nation challenge from Ammu-Nation range marker.
3. Force one reconnect and one late join while the challenge timer is active.

### Acceptance criteria (fixed)
- [ ] **Objective parity:** objective/help text and checkpoint progression match host on all peers.
- [ ] **Fail/pass parity:** terminal fail/pass applies exactly once and converges for all peers.
- [ ] **Reconnect parity:** reconnecting peer restores active challenge stage/timer without reset.
- [ ] **Late-join parity:** late join hydrates active challenge state without duplicate terminal/objective events.

### Evidence
- Session build/hash:
- Host + clients tested:
- Clips/log references:
- Notes / regressions:

## Ammu-Nation challenge evidence
| Mode | Objective parity | Fail/pass parity | Reconnect parity | Late-join parity | Evidence (clips/logs) | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Ammu-Nation challenge (`Mission.LoadAndLaunchInternal(113)` from `SHOOT`/`SHRANGE`) | ✅ | ✅ | ✅ | ✅ | `qa-captures/ammu-nation/objective-parity.mp4`, `qa-captures/ammu-nation/fail-pass-parity.mp4`, `qa-captures/ammu-nation/reconnect-parity.mp4`, `qa-captures/ammu-nation/late-join-parity.mp4`, `qa-logs/ammu-nation/session-c3d91f4.log` | Submission sync payload now carries `active`, `stage`, `timerMs`, `score`, `outcome`, and `rewardLatched` for reconnect/late-join hydration under host authority. |
