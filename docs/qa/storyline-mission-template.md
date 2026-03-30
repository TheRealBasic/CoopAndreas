# Storyline mission onboarding template

Use this template when onboarding any new storyline mission into co-op sync parity.

Baseline: this flow is the same mission-flow plumbing already validated by **Cleaning The Hood** and **Drive-Thru** in W1 (`docs/qa/sweet-w1-cleaning-drive-thru-parity.md`).

## 1) Required common mission-flow hooks (client/server)

These hooks must be present and exercised for each storyline mission:

1. **Start hook**
   - Host emits mission-flow state on `0x0417` (`Mission.LoadAndLaunchInternal` / `start_mission`) via `CMissionSyncState::EmitMissionFlowOpcode`.
   - Mission identity (`missionId`) and attempt state reset are propagated via `CPackets::MissionFlowSync`.

2. **Objective text hook**
   - Host updates mission objective state on text opcodes `0x00BA` (`print_big`), `0x00BC` (`print_now`), `0x03E5` (`print_help`).
   - Objective replay text is emitted via `CMissionSyncState::EmitMissionFlowText` (`MISSION_FLOW_EVENT_OBJECTIVE`).

3. **Fail hook**
   - Host emits deterministic fail on `0x045C` (`fail_current_mission`) and latches `passFailPending=2`.
   - Replay path re-applies fail text event with `MISSION_FLOW_EVENT_FAIL`.

4. **Pass hook**
   - Host emits deterministic pass on `0x0318` (`register_mission_passed`) and latches `passFailPending=1`.
   - Replay path re-applies pass text event with `MISSION_FLOW_EVENT_PASS`.

5. **Resume snapshot hook (reconnect + late-join hydration)**
   - Server caches latest mission-flow payload (`ms_lastMissionFlowSync`) and replays it with `replay=1` in `SendMissionStateSnapshot`.
   - Client consumes replay in `CMissionSyncState::HandleMissionFlowSync` and restores mission attempt state (`missionId`, `objective`, timer/checkpoint, terminal latch).

## 2) Required sync events

Every onboarded storyline mission must verify all event types below through at least one host + peer capture:

- `MISSION_FLOW_EVENT_STATE_UPDATE` (start/progression/timer/checkpoint/control updates)
- `MISSION_FLOW_EVENT_OBJECTIVE` (objective/help text)
- `MISSION_FLOW_EVENT_FAIL`
- `MISSION_FLOW_EVENT_PASS`
- `replay=1` `MISSION_FLOW_SYNC` snapshot on reconnect/late-join

## 3) Required opcodes/commands

At minimum, onboarding evidence must show opcode handling for the mission-flow contract below:

- **Start/identity:** `0x0417` (`Mission.LoadAndLaunchInternal` / `start_mission`)
- **Objective text:** `0x00BA` (`print_big`), `0x00BC` (`print_now`), `0x03E5` (`print_help`)
- **Terminal outcome:** `0x045C` (`fail_current_mission`), `0x0318` (`register_mission_passed`)
- **Resume state carriers used by mission scripts as needed:**
  - `0x01B4` (`set_player_control`)
  - `0x06D5` / `0x07F3` (checkpoint create/move)
  - `0x014E`, `0x03C3`, `0x014F`, `0x0396`, `0x0890` (timer visibility/freeze/countdown)

## 4) QA gates (must be filled before promotion)

For each mission row in `docs/qa/storyline-wave-mission-evidence.md`, capture and link evidence for:

1. Start/cutscene validation
2. Objective validation
3. Fail validation
4. Pass validation
5. Reconnect validation
6. Late-join validation

## 5) Promotion rule (mandatory)

Before a storyline mission status changes from `not started` to `in progress`, the mission owner **must**:

- link this template (`docs/qa/storyline-mission-template.md`) in the mission note/evidence row, and
- confirm all six QA gate placeholders are initialized for that mission.

If either condition is missing, status promotion is blocked.


## 6) Implementation anchors (where to verify in code)

- Client mission-flow emit + replay handling: `client/src/CMissionSyncState.cpp`
- Host opcode capture feeding mission-flow events: `client/src/COpCodeSync.cpp`
- Packet contract + event enum: `client/src/CPackets.h`
- Server host-authoritative filter + mission-flow fan-out/cache: `server/src/CPlayerManager.h`
- Reconnect/late-join snapshot replay dispatch: `server/src/CPlayerManager.h` (`SendMissionStateSnapshot`)
