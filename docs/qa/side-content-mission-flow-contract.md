# Side-content mission flow shared payload contract

This document defines the shared mission/sync payload used by side-content handlers (schools, races, courier-like modes, and similar scripted activities) so each stadium mode can plug into the same replication path with minimal custom logic.

## Opcode/event coverage on the shared path

- **Launch/start propagation**
  - `Mission.LoadAndLaunchInternal` (`0x0417`) initializes a new attempt baseline.
  - Race starts (for example `start_car_race` flows that launch mission scripts) are expected to enter through the same start baseline path.
- **Checkpoint progression**
  - `checkpoint_create` (`0x06D5`) and `checkpoint_set_coords` (`0x07F3`) advance `checkpointIndex`.
  - Race checkpoint logic (for example `set_car_race_checkpoint`) maps to the same monotonic index behavior.
- **Timer sync**
  - `display_onscreen_timer` (`0x014E`)
  - `display_onscreen_timer_with_string` (`0x03C3`)
  - `clear_onscreen_timer` (`0x014F`)
  - `freeze_onscreen_timer` (`0x0396`)
  - `set_timer_beep_countdown_time` (`0x0890`)
- **Deterministic pass/fail**
  - `register_mission_passed` (`0x0318`) sets `passFailPending = 1`.
  - `fail_current_mission` (`0x045C`) sets `passFailPending = 2`.
  - Terminal outcome is latched once per attempt to prevent duplicate pass/fail replay.

## Shared payload/state fields (`MissionFlowSync`)

- `missionId`: start identity for the current attempt.
- `objective`: latest objective/help GXT key.
- `checkpointIndex`: monotonic progression index for objectives/checkpoints.
- `timerVisible`, `timerFrozen`, `timerMs`: timer HUD state + value.
- `passFailPending`: terminal latch (`0 none`, `1 pass`, `2 fail`).
- `playerControlState`: mirrored player-control state from scripts.
- `sequence`: authoritative ordering for state/event updates.
- `replay`: server-set flag for reconnect/late-join hydration packets.

## Reconnect + late-join hydration rule

The server caches the latest mission-flow packet and replays it (`replay = 1`) to reconnecting/late-joining peers. Because each packet carries full side-content attempt state (not only the triggering opcode), peers hydrate current launch/progression/timer/outcome state immediately without waiting for another script edge.
