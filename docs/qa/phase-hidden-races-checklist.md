# Phase QA Checklist — Hidden races

## Scope
Validate co-op parity for hidden races: **BMX**, **NRG-500**, **Chiliad Challenge**.

Shared payload contract reference: `docs/qa/side-content-mission-flow-contract.md`.

## Required opcode/command coverage
- `Mission.LoadAndLaunchInternal`
- `start_checkpoint_race`
- `set_car_race_checkpoint`
- `set_timers`
- `register_mission_passed`
- `fail_current_mission`

## Mission/opcode sync mapping (implementation trace)

Hidden-race scripts (`ODDVEH` launch into missions `132`/`133`) are wired through the shared side-content mission-flow path:

- `Mission.LoadAndLaunchInternal` → `0x0417` mission start baseline emit/apply in opcode sync + mission sync state.
- `start_checkpoint_race` / `set_car_race_checkpoint` → checkpoint progression path via `checkpoint_create` (`0x06D5`) + `checkpoint_set_coords` (`0x07F3`) folded into monotonic `checkpointIndex`.
- `set_timers` → timer visibility/freeze/value path (`0x014E`, `0x014F`, `0x03C3`, `0x0396`, `0x0890`) mirrored in `MissionFlowSync`.
- `register_mission_passed` / `fail_current_mission` → deterministic terminal latch (`passFailPending=1/2`) with once-only adjudication.

Reconnect and late-join hydration source is the mission runtime snapshot + replay packet path, restoring:

- checkpoint progression (`checkpointIndex`)
- timer state (`timerVisible`, `timerFrozen`, `timerMs`)
- completion state (`passFailPending`)

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
- [BMX execution](#bmx-execution)
- [NRG-500 execution](#nrg-500-execution)
- [Chiliad Challenge execution](#chiliad-challenge-execution)

### BMX execution
- **Objective parity:** ✅ Pass — objective text + checkpoint progression aligned across host and peers.
- **Fail/pass parity:** ✅ Pass — fail and pass outcomes emitted once and matched on all peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active race checkpoint + timer state without reset.
- **Late-join parity:** ✅ Pass — late join hydrated in-progress race state without duplicate objective/pass/fail events.
- **Session build/hash:** `coopandreas-dev-2026-03-31 / 3c1a9d2`
- **Evidence refs:** `qa-captures/hidden-races/bmx/objective-parity.mp4`, `qa-captures/hidden-races/bmx/fail-pass-parity.mp4`, `qa-captures/hidden-races/bmx/reconnect-parity.mp4`, `qa-captures/hidden-races/bmx/late-join-parity.mp4`, `qa-logs/hidden-races/bmx/session-3c1a9d2.log`
- **Gate status:** `done` (all four parity gates passed).

### NRG-500 execution
- **Objective parity:** ✅ Pass — objective text + checkpoint progression aligned across host and peers.
- **Fail/pass parity:** ✅ Pass — fail and pass outcomes emitted once and matched on all peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active race checkpoint + timer state without reset.
- **Late-join parity:** ✅ Pass — late join hydrated in-progress race state without duplicate objective/pass/fail events.
- **Session build/hash:** `coopandreas-dev-2026-03-31 / 3c1a9d2`
- **Evidence refs:** `qa-captures/hidden-races/nrg-500/objective-parity.mp4`, `qa-captures/hidden-races/nrg-500/fail-pass-parity.mp4`, `qa-captures/hidden-races/nrg-500/reconnect-parity.mp4`, `qa-captures/hidden-races/nrg-500/late-join-parity.mp4`, `qa-logs/hidden-races/nrg-500/session-3c1a9d2.log`
- **Gate status:** `done` (all four parity gates passed).

### Chiliad Challenge execution
- **Objective parity:** ✅ Pass — objective text + checkpoint progression aligned across host and peers.
- **Fail/pass parity:** ✅ Pass — fail and pass outcomes emitted once and matched on all peers.
- **Reconnect parity:** ✅ Pass — reconnecting peer restored active race checkpoint + timer state without reset.
- **Late-join parity:** ✅ Pass — late join hydrated in-progress race state without duplicate objective/pass/fail events.
- **Session build/hash:** `coopandreas-dev-2026-03-31 / 3c1a9d2`
- **Evidence refs:** `qa-captures/hidden-races/chiliad/objective-parity.mp4`, `qa-captures/hidden-races/chiliad/fail-pass-parity.mp4`, `qa-captures/hidden-races/chiliad/reconnect-parity.mp4`, `qa-captures/hidden-races/chiliad/late-join-parity.mp4`, `qa-logs/hidden-races/chiliad/session-3c1a9d2.log`
- **Gate status:** `done` (all four parity gates passed).
