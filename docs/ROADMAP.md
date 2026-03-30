# CoopAndreas Roadmap

Detailed backlog extracted from `README.md` sections: **Current Tasks**, **TODO Launcher**, **TODO Missions**, and **TODO Other Scripts**.

Legend:
- **Priority:** `P0` = critical for playable co-op baseline, `P1` = important parity/quality, `P2` = optional or long-tail.
- **Effort:** `S` (small), `M` (medium), `L` (large).

## Traceability Tag Standard

Use a lightweight parity tag in implementation and QA artifacts to keep roadmap work items traceable end-to-end:

- **Tag format:** `PARITY-ID: <AREA>-<ITEM>`
- **Pattern:** uppercase kebab-case tokens, e.g. `PARITY-ID: STADIUM-8-TRACK`, `PARITY-ID: SCHOOLS-BOAT-SCHOOL`.
- **Where to add tags:** PR titles/descriptions, commit messages, and (optionally) focused code comments near critical sync logic.
- **Index generator:** `python3 tools/qa_trace.py` writes `docs/qa/roadmap-trace-index.json`.

## Milestone: Core Sync Stability

### P0 (must-have)
- [x] **Pickups sync** — collectibles, static pickups, and dropped pickups (`money`, `weapons`). **[P0][L]** ✅ Completed (2026-03-26).
  - **Acceptance criteria:** pickup state changes persist across reconnects for all connected peers and late joiners.
  - **Acceptance criteria:** collectible categories (graffiti/horseshoes/snapshots/oysters) do not duplicate or respawn incorrectly after host migration/rejoin.
  - **Acceptance criteria:** dropped money/weapons appear, can be collected once, and are removed consistently across at least 2 peers.
- [x] **Wanted level sync**. **[P0][M]** ✅ Completed (2026-03-25).
  - **Acceptance criteria:** wanted level changes are mirrored and verified across 2+ peers in free roam and mission contexts.
  - **Acceptance criteria:** reconnecting peer receives current wanted level without requiring a manual refresh.
- [x] **Passenger sync completion** (`gamepad support`, `radio sync`). **[P0][M]** ✅ Completed (2026-03-25).
  - **Acceptance criteria:** controller inputs map correctly for non-driver passenger actions on all clients.
  - **Acceptance criteria:** radio station selection/state stays consistent for driver + passengers across 2+ peers.
- [x] **Vehicle sync completion** (`force hydraulics sync`, `trailer sync`). **[P0][L]** ✅ Completed (2026-03-26).
  - **Acceptance criteria:** hydraulic state transitions replicate reliably without visual divergence.
  - **Acceptance criteria:** trailer attach/detach and trailer physics state remain consistent across 2+ peers over 5+ minutes.
- [x] **NPC sync completion** (`in vehicle: horn/siren`, `aim`, `shots`, `task sync`). **[P0][L]** ✅ Completed (2026-03-25).
  - **Acceptance criteria:** NPC in-vehicle horn/siren state changes replicate in real time for all peers.
  - **Acceptance criteria:** NPC aiming and firing targets/impact events match across 2+ peers.
  - **Acceptance criteria:** task/state machine replication avoids stuck or desynced behavior over mission-length sessions.

### P1 (important)
- [x] Jetpack pickup interaction (depends on pickup sync). **[P1][S]**
- [x] Player map sync: `Areas/GangZones`, map-pin proportion fix. **[P1][M]**
- [x] Stats sync: `breath level bar`, `max hp sync`. **[P1][M]**
- [x] Fire sync. **[P1][M]**
- [x] Cheat code sync. **[P1][M]**
- [x] Animation sync: `idle anims`, special `TAB+NUM4/NUM6` anims. **[P1][M]**
- [x] Gang groups sync. **[P1][M]**
- [x] Widescreen fix. **[P1][S]**
- [x] Smooth interpolation: movement + rotation. **[P1][M]**

### P2 (nice-to-have)
- [ ] Player voice commands. **[P2][S]**
- [ ] Chat reactions (`LD_CHAT.txd`). **[P2][S]**
- [x] Gang wars sync (host-authoritative lifecycle + snapshots). **[P2][L]** ✅ Completed (2026-03-26).
  - **Acceptance criteria (passed):** host emits authoritative lifecycle events for trigger/start, wave progression, reinforcement transitions, and win/loss outcomes in deterministic order.
  - **Acceptance criteria (passed):** territory ownership updates remain synchronized with gang-zone owner/color propagation and persist through subsequent wars.
  - **Acceptance criteria (passed):** reconnecting and late-join peers receive in-progress lifecycle snapshot plus final ownership state without manual refresh.
  - **Acceptance criteria (passed):** interior/area transitions and mission-overlap situations do not suppress gang-war lifecycle propagation.
  - **Validation artifact:** `docs/qa/gang-wars-sync-matrix.md` multi-peer matrix with pass/fail criteria for GW-01..GW-10.
- [x] Parachute jump sync. **[P2][M]** ✅ Completed (2026-03-26).
  - **Acceptance criteria (passed):** host-authoritative parachute lifecycle transitions replicate deterministically (`deploy`, `glide`, `cut`, `landing`, `fail`).
  - **Acceptance criteria (passed):** reconnect and late-join peers hydrate in-progress lifecycle state without duplicate terminal outcomes or reward side effects.
- [x] Stunt systems (`collecting`, per-player slow motion). **[P2][M]** ✅ Completed (2026-03-26).
  - **Acceptance criteria (passed):** stunt triggers and scoring replicate per player without cross-player slow-motion bleed.
  - **Acceptance criteria (passed):** stunt collection/checkpoint progress remains authoritative and persists through reconnect and late join.
  - **Acceptance criteria (passed):** reward replay is idempotent (no duplicate grants) across reconnect/late-join snapshot hydration.
  - **Validation artifact:** `docs/qa/parachute-stunt-sync-matrix.md` multi-peer and high-latency matrix (PS-01..PS-11).
- [ ] Chat gamepad support with on-screen keyboard. **[P2][M]**
- [x] Sniper laser red-dot sync. **[P2][S]** ✅ Completed (2026-03-26).
  - **Acceptance criteria (passed):** host-authoritative sniper aim marker payload replicates source/direction/range + visibility + tick and late-join peers hydrate the active marker without waiting for a new shot.
  - **Acceptance criteria (passed):** non-host clients render remote sniper red-dot using replicated marker state only, with smoothing that avoids visible jitter under ordinary packet jitter.
  - **Validation artifact:** `docs/qa/sniper-red-dot-moon-sync-matrix.md` (SN-01, SN-02, SN-04).
- [x] Moon-sniper easter egg shot-size effect. **[P2][S]** ✅ Completed (2026-03-26).
  - **Acceptance criteria (passed):** server computes deterministic moon-sniper activation and replicates `moonSniperActive` + `shotSizeMultiplier` in the authoritative shot payload.
  - **Acceptance criteria (passed):** shot-size multiplier defaults to `1.0` when moon-sniper is inactive, with no local-only RNG divergence.
  - **Validation artifact:** `docs/qa/sniper-red-dot-moon-sync-matrix.md` (SN-03).

## Milestone: Mission Parity

### P1
- [ ] Storyline mission parity checklist (`Cleaning The Hood` → `End Of The Line`, excluding already complete: `Big Smoke`, `Ryder`, `Tagging Up Turf`). **[P1][L]**

### Storyline parity waves (script-family rollout)

Wave execution rule: advance one wave at a time; each wave keeps a named owner, explicit status, and fixed acceptance checkpoints.

| Wave | Script family scope | Owner | Wave status | Opcode audit status | Acceptance checkpoints |
| --- | --- | --- | --- | --- | --- |
| W1 | `SWEET` | Mission Sync Pod (Sweet stream) | `in progress` | `clear` (0 missing on 2026-03-30) | `C1` owner assigned, `C2` mission rows created in QA evidence doc, `C3` start/objective/fail/pass/reconnect notes captured per mission before promotion, `C4` reconnect replay validated on 2+ peers, `C5` `tools/opcode_audit.py` rerun and delta logged in storyline backlog, `C6` wave sign-off attached. |
| W2 | `RYDER` | Mission Sync Pod (Ryder stream) | `not started` | `clear` (0 missing on 2026-03-30) | Same checkpoints `C1..C6`; may not move to `in progress` until QA evidence rows exist for all W2 missions. |
| W3 | `SMOKE` | Mission Sync Pod (Smoke stream) | `not started` | `clear` (0 missing on 2026-03-30) | Same checkpoints `C1..C6`; may not move to `in progress` until QA evidence rows exist for all W3 missions. |

Wave evidence + status gate source of truth: `docs/qa/storyline-wave-mission-evidence.md`.

Opcode gap tracking rule per wave:
1. Run `python3 tools/opcode_audit.py --output docs/qa/storyline-opcode-backlog.md` at wave start and wave end.
2. Update the matching wave ledger row in `docs/qa/storyline-opcode-backlog.md` with any newly discovered gaps (or explicit `none`).
3. Do not mark a wave `done` until wave-end audit is recorded.

| Mission | Script | Status | Blocking dependencies (opcodes/commands) | Quick acceptance criteria |
| --- | --- | --- | --- | --- |
| Cleaning The Hood | `scm/scripts/SWEET.txt` | `in progress` | `Mission.LoadAndLaunchInternal`, `create_actor`, `set_char_obj_kill_char_any_means` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Drive-Thru | `scm/scripts/SWEET.txt` | `in progress` | `create_car`, `task_drive_by`, `set_char_obj_destroy_car` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Nines And AK's | `scm/scripts/SWEET.txt` | `not started` | `create_pickup`, `task_go_to_coord_any_means`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Drive-By | `scm/scripts/SWEET.txt` | `not started` | `task_drive_by`, `set_char_obj_destroy_car`, `set_mission_audio_position` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Sweet's Girl | `scm/scripts/SWEET.txt` | `not started` | `create_actor`, `set_char_obj_flee_char_on_foot_till_safe`, `task_enter_car_as_driver` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Cesar Vialpando | `scm/scripts/SWEET.txt` | `not started` | `Mission.LoadAndLaunchInternal`, `task_car_drive_wander`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Burning Desire | `scm/scripts/CRASH.txt` | `not started` | `create_fire`, `remove_char_from_car_maintain_position`, `set_char_obj_leave_car` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Gray Imports | `scm/scripts/CRASH.txt` | `not started` | `create_actor`, `set_char_obj_kill_char_any_means`, `blow_up_car` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Home Invasion | `scm/scripts/RYDER.txt` | `not started` | `create_pickup`, `task_go_straight_to_coord`, `set_timers` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Catalyst | `scm/scripts/RYDER.txt` | `not started` | `create_train`, `task_jump_from_car`, `set_mission_audio_position` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Robbing Uncle Sam | `scm/scripts/RYDER.txt` | `not started` | `create_object`, `attach_object_to_car`, `set_char_obj_kill_char_any_means` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| OG Loc | `scm/scripts/SMOKE.txt` | `not started` | `task_follow_nav_mesh_to_coord`, `task_bike_flee_point`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Running Dog | `scm/scripts/SMOKE.txt` | `not started` | `create_actor`, `task_sprint_to_coord`, `set_char_obj_kill_char_any_means` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Wrong Side Of The Tracks | `scm/scripts/SMOKE.txt` | `not started` | `create_train`, `task_drive_by`, `task_car_mission` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Just Business | `scm/scripts/SMOKE.txt` | `not started` | `create_car`, `task_drive_point_route`, `task_shoot_at_char` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Life's A Beach | `scm/scripts/STRAP.txt` | `not started` | `start_mini_game`, `task_play_anim_non_interruptable`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Madd Dogg's Rhymes | `scm/scripts/STRAP.txt` | `not started` | `task_sneak_at_coord`, `set_char_obj_steal_any_car`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Management Issues | `scm/scripts/STRAP.txt` | `not started` | `task_enter_car_as_driver`, `set_car_driving_style`, `set_char_obj_destroy_car` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| House Party | `scm/scripts/STRAP.txt` | `not started` | `create_actor`, `task_kill_char_on_foot`, `set_mission_audio_position` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| High Stakes, Low-Rider | `scm/scripts/CESAR.txt` | `not started` | `start_car_race`, `set_car_race_checkpoint`, `task_car_drive_to_coord` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Reuniting The Families | `scm/scripts/LA1FIN.txt` | `not started` | `task_go_to_coord_any_means`, `set_char_accuracy`, `task_kill_char_on_foot` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| The Green Sabre | `scm/scripts/LA1FIN.txt` | `not started` | `create_car`, `set_char_obj_flee_char_on_foot_till_safe`, `set_fade_colour` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Badlands | `scm/scripts/BCRASH.txt` | `not started` | `task_follow_char_in_car`, `set_char_obj_kill_char_any_means`, `store_car_char_is_in_no_save` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| First Date | `scm/scripts/CAT.txt` | `not started` | `create_actor`, `task_go_to_coord_any_means`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Tanker Commander | `scm/scripts/CAT.txt` | `not started` | `create_car`, `attach_trailer_to_cab`, `task_car_drive_to_coord` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Against All Odds | `scm/scripts/CAT.txt` | `not started` | `create_object`, `add_explosion`, `set_char_obj_escape_car` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Local Liquor Store | `scm/scripts/CAT.txt` | `not started` | `create_actor`, `task_kill_char_on_foot`, `task_enter_car_as_driver` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Small Town Bank | `scm/scripts/CAT.txt` | `not started` | `task_go_to_coord_any_means`, `set_char_obj_flee_char_on_foot_till_safe`, `create_pickup` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| King In Exile | `scm/scripts/CAT.txt` | `not started` | `start_cutscene`, `set_player_control`, `Mission.LoadAndLaunchInternal` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Wu Zi Mu / Farewell, My Love... | `scm/scripts/BCESAR.txt` | `not started` | `start_car_race`, `set_car_race_checkpoint`, `task_car_drive_to_coord` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Wear Flowers In Your Hair | `scm/scripts/GARAGE.txt` | `not started` | `create_car`, `task_enter_car_as_driver`, `task_car_drive_to_coord` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| 555 WE TIP | `scm/scripts/SCRASH.txt` | `not started` | `create_actor`, `task_enter_car_as_driver`, `set_char_obj_steal_any_car` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Deconstruction | `scm/scripts/GARAGE.txt` | `not started` | `create_actor`, `add_explosion`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Photo Opportunity | `scm/scripts/SYND.txt` | `not started` | `create_camera`, `task_car_drive_to_coord`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Jizzy | `scm/scripts/SYND.txt` | `not started` | `task_enter_car_as_driver`, `task_car_drive_to_coord`, `set_mission_audio_position` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| T-Bone Mendez | `scm/scripts/SYND.txt` | `not started` | `task_bike_mission`, `set_char_obj_kill_char_any_means`, `create_pickup` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Mike Toreno | `scm/scripts/SYND.txt` | `not started` | `task_car_drive_to_coord`, `set_char_obj_kill_char_any_means`, `set_timers` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Outrider | `scm/scripts/SYND.txt` | `not started` | `create_object`, `add_explosion`, `task_car_drive_to_coord` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Snail Trail | `scm/scripts/SCRASH.txt` | `not started` | `task_follow_char_in_car`, `task_follow_nav_mesh_to_coord`, `set_char_obj_kill_char_any_means` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Ice Cold Killa | `scm/scripts/SYND.txt` | `not started` | `create_actor`, `task_kill_char_on_foot`, `set_char_obj_flee_char_on_foot_till_safe` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Pier 69 | `scm/scripts/SYND.txt` | `not started` | `task_sniper_mode`, `task_kill_char_on_foot`, `create_boat` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Toreno's Last Flight | `scm/scripts/SYND.txt` | `not started` | `create_heli`, `set_char_obj_destroy_car`, `task_aim_gun_at_char` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Yay Ka-Boom-Boom | `scm/scripts/SYND.txt` | `not started` | `create_car`, `add_explosion`, `set_char_obj_kill_char_any_means` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Mountain Cloud Boys | `scm/scripts/WUZI.txt` | `not started` | `task_go_to_coord_any_means`, `task_kill_char_on_foot`, `set_char_accuracy` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Ran Fa Li | `scm/scripts/WUZI.txt` | `not started` | `task_car_drive_to_coord`, `set_char_obj_destroy_car`, `create_pickup` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Lure | `scm/scripts/WUZI.txt` | `not started` | `task_car_drive_wander`, `set_char_obj_destroy_car`, `set_mission_audio_position` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Amphibious Assault | `scm/scripts/WUZI.txt` | `not started` | `task_swim_to_coord`, `task_stealth_kill_char`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| The Da Nang Thang | `scm/scripts/WUZI.txt` | `not started` | `create_boat`, `task_kill_char_on_foot`, `set_char_obj_escape_car` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Air Raid | `scm/scripts/ZERO.txt` | `not started` | `start_mini_game`, `create_object`, `set_timers` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Supply Lines... | `scm/scripts/ZERO.txt` | `not started` | `start_mini_game`, `task_plane_mission`, `set_timers` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| New Model Army | `scm/scripts/ZERO.txt` | `not started` | `start_mini_game`, `create_object`, `task_rc_mission` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Monster | `scm/scripts/DESERT.txt` | `not started` | `start_checkpoint_race`, `set_car_race_checkpoint`, `set_timers` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Highjack | `scm/scripts/DESERT.txt` | `not started` | `task_jump_from_car`, `task_enter_car_as_driver`, `attach_trailer_to_cab` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Interdiction | `scm/scripts/DESERT.txt` | `not started` | `create_heli`, `create_pickup`, `set_char_obj_kill_char_any_means` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Verdant Meadows | `scm/scripts/DESERT.txt` | `not started` | `set_player_money`, `set_objective`, `set_garage_respray_free` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| N.O.E. | `scm/scripts/DESERT.txt` | `not started` | `task_plane_mission`, `set_timers`, `set_mission_audio_position` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Stowaway | `scm/scripts/DESERT.txt` | `not started` | `task_jump_from_car`, `create_object`, `add_explosion` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Black Project | `scm/scripts/DESERT.txt` | `not started` | `task_sneak_at_coord`, `task_use_jetpack`, `create_pickup` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Green Goo | `scm/scripts/DESERT.txt` | `not started` | `create_train`, `task_use_jetpack`, `create_pickup` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Fender Ketchup | `scm/scripts/CASINO.txt` | `not started` | `task_enter_car_as_driver`, `set_timers`, `set_char_obj_flee_char_on_foot_till_safe` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Explosive Situation | `scm/scripts/CASINO.txt` | `not started` | `create_pickup`, `task_kill_char_on_foot`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| You've Had Your Chips | `scm/scripts/CASINO.txt` | `not started` | `create_object`, `add_explosion`, `task_kill_char_on_foot` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Don Peyote | `scm/scripts/CASINO.txt` | `not started` | `create_car`, `task_car_drive_to_coord`, `task_enter_car_as_driver` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Intensive Care | `scm/scripts/CASINO.txt` | `not started` | `create_car`, `task_car_mission`, `set_char_obj_destroy_car` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| The Meat Business | `scm/scripts/CASINO.txt` | `not started` | `create_actor`, `task_kill_char_on_foot`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Fish In A Barrel | `scm/scripts/CASINO.txt` | `not started` | `start_cutscene`, `set_player_control`, `Mission.LoadAndLaunchInternal` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Freefall | `scm/scripts/CASINO.txt` | `not started` | `create_plane`, `task_plane_mission`, `set_char_obj_kill_char_any_means` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Saint Mark's Bistro | `scm/scripts/CASINO.txt` | `not started` | `task_kill_char_on_foot`, `set_char_accuracy`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Architectural Espionage | `scm/scripts/HEIST.txt` | `not started` | `task_go_to_coord_any_means`, `create_pickup`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Key To Her Heart | `scm/scripts/HEIST.txt` | `not started` | `task_follow_char_in_car`, `task_enter_car_as_driver`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Dam And Blast | `scm/scripts/HEIST.txt` | `not started` | `task_parachute_to_target`, `create_object`, `add_explosion` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Cop Wheels | `scm/scripts/HEIST.txt` | `not started` | `set_char_obj_steal_any_car`, `create_car`, `set_timers` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Up, Up And Away! | `scm/scripts/HEIST.txt` | `not started` | `create_heli`, `task_heli_mission`, `attach_object_to_car` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Breaking The Bank At Caligula's | `scm/scripts/HEIST.txt` | `not started` | `task_go_to_coord_any_means`, `create_pickup`, `set_char_obj_kill_char_any_means` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Madd Dogg | `scm/scripts/DOC.txt` | `not started` | `task_follow_nav_mesh_to_coord`, `set_char_obj_kill_char_any_means`, `set_timers` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Misappropriation | `scm/scripts/VCRASH.txt` | `not started` | `task_follow_char_in_car`, `create_pickup`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| High Noon | `scm/scripts/VCRASH.txt` | `not started` | `create_actor`, `task_duel`, `set_char_obj_kill_char_any_means` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| A Home In The Hills | `scm/scripts/MANSION.txt` | `not started` | `task_enter_car_as_driver`, `task_kill_char_on_foot`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Vertical Bird | `scm/scripts/MANSION.txt` | `not started` | `create_heli`, `task_swim_to_coord`, `task_heli_mission` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Home Coming | `scm/scripts/MANSION.txt` | `not started` | `create_actor`, `task_go_to_coord_any_means`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Cut Throat Business | `scm/scripts/MANSION.txt` | `not started` | `task_car_chase`, `set_timers`, `task_kill_char_on_foot` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Beat Down On B Dup | `scm/scripts/GROVE.txt` | `not started` | `task_go_to_coord_any_means`, `task_kill_char_on_foot`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Grove 4 Life | `scm/scripts/GROVE.txt` | `not started` | `task_gang_war`, `set_zone_gang_strength`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Riot | `scm/scripts/RIOT.txt` | `not started` | `create_actor`, `set_riot_mode`, `task_kill_char_on_foot` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| Los Desperados | `scm/scripts/RIOT.txt` | `not started` | `create_actor`, `task_kill_char_on_foot`, `set_char_accuracy` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| End Of The Line (1) | `scm/scripts/RIOT.txt` | `not started` | `task_go_to_coord_any_means`, `task_kill_char_on_foot`, `set_objective` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| End Of The Line (2) | `scm/scripts/RIOT.txt` | `not started` | `create_car`, `task_car_chase`, `set_char_obj_destroy_car` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |
| End Of The Line (3) | `scm/scripts/RIOT.txt` | `not started` | `start_cutscene`, `task_kill_char_on_foot`, `Mission.LoadAndLaunchInternal` | Cutscene sync, objective sync, fail/pass sync, reconnect resumes mission state. |

## Milestone: Launcher UX

### P1
- [x] Start/control server directly from launcher. **[P1][M]** ✅ Completed (2026-03-26).

## Milestone: Optional Content

### P2
- [x] Property purchase sync. **[P2][M]** ✅ Completed (2026-03-30).
  - **Completion note (2026-03-30):** all four parity gates are now explicitly represented in the QA matrix (`PP-01..PP-04`) with expected/observed evidence sections covering purchase flow, reconnect restore, late-join hydration, and post-host-migration consistency.
  - **Validation artifact:** `docs/qa/property-purchase-sync-checklist.md`.
- [x] Submissions sync framework (`Taxi`, `Firefighter`, `Vigilante`, `Paramedic`, `Pimp`, `Freight Train`). **[P2][L]** ✅ Completed (2026-03-26).
  - **Shared sync layer:** ✅ host-authoritative generic submission state module now tracks mode, level/stage, timer, progress, score/reward, pass/fail outcome, and participant count; supports reconnect/late-join snapshot hydration.
  - **Taxi:** ✅ SS-02..SS-11 multiplayer QA complete.
  - **Firefighter:** ✅ SS-02..SS-11 multiplayer QA complete.
  - **Vigilante:** ✅ SS-02..SS-11 multiplayer QA complete.
  - **Paramedic:** ✅ SS-02..SS-11 multiplayer QA complete.
  - **Pimp:** ✅ SS-02..SS-11 multiplayer QA complete.
  - **Freight Train:** ✅ SS-02..SS-11 multiplayer QA complete.
  - **Validation artifacts:** `docs/qa/submissions-sync-matrix.md`, `docs/qa/submissions-sync-evidence.md`.
- [ ] Hidden races: `BMX`, `NRG-500`, `Chiliad Challenge`. **[P2][M]**
- [x] Stadium events: `8-Track`, `Blood Bowl`, `Dirt Track`, `Kick Start`. **[P2][M]** ✅ Completed (2026-03-27).
  - **Acceptance criteria (passed):** objective parity for all four stadium modes using the fixed stadium opcode/state-transition contract.
  - **Acceptance criteria (passed):** fail/pass parity applies terminal outcomes once across peers for each stadium mode.
  - **Acceptance criteria (passed):** reconnect parity restores active attempts with checkpoint/timer state without restarting the mode.
  - **Acceptance criteria (passed):** late-join parity hydrates active stadium attempts without duplicate objective/pass/fail events.
  - **Validation artifact:** `docs/qa/phase-stadium-events-checklist.md`.
  - **Completion note (2026-03-27):** each stadium mode is marked `done` only after passing all four parity gates (objective parity, fail/pass parity, reconnect parity, late-join parity).
    - Evidence index: `docs/qa/phase-stadium-events-checklist.md`.
    - Per-mode execution sections: [8-Track](docs/qa/phase-stadium-events-checklist.md#8-track-execution), [Blood Bowl](docs/qa/phase-stadium-events-checklist.md#blood-bowl-execution), [Dirt Track](docs/qa/phase-stadium-events-checklist.md#dirt-track-execution), [Kick Start](docs/qa/phase-stadium-events-checklist.md#kick-start-execution).
- [ ] Ammu-Nation challenge. **[P2][S]**
- [x] Schools: `Driving`, `Flight`, `Bike`, `Boat`. **[P2][L]** ✅ Completed (2026-03-27).
  - **Acceptance criteria (passed):** objective parity for all four school modes using the fixed schools opcode/state-transition contract.
  - **Acceptance criteria (passed):** fail/pass parity applies terminal outcomes once across peers for each school mode.
  - **Acceptance criteria (passed):** reconnect parity restores active attempts with checkpoint/timer state without restarting tests.
  - **Acceptance criteria (passed):** late-join parity hydrates active school attempts without duplicate objective/pass/fail events.
  - **Validation artifact:** `docs/qa/phase-schools-checklist.md`.
  - **Completion note (2026-03-27):** all schools modes are `done` after passing the fixed parity gates (objective parity, fail/pass parity, reconnect parity, late-join parity).
    - Evidence index: `docs/qa/phase-schools-checklist.md`.
    - Per-mode sign-off logs/clips: [Driving school evidence](docs/qa/phase-schools-checklist.md#driving-school-evidence), [Flight school evidence](docs/qa/phase-schools-checklist.md#flight-school-evidence), [Bike school evidence](docs/qa/phase-schools-checklist.md#bike-school-evidence), [Boat school evidence](docs/qa/phase-schools-checklist.md#boat-school-evidence).
- [ ] Asset missions: `Trucker`, `Valet`, `Career`. **[P2][L]**
- [ ] Courier sets: LS/SF/LV routes. **[P2][M]**
- [ ] Street Racing set (22 races). **[P2][L]**
- [ ] Import / Export. **[P2][M]**


## Optional Content Phased Rollout Plan

Execution rule: implement **one phase at a time**. A phase is only marked `done` once all included modes pass the fixed acceptance criteria:
- objective parity
- fail/pass parity
- reconnect parity
- late-join parity

### Phase grouping (open content)

| Phase | Modes in scope | Required opcode/command focus | Required state transitions | QA checklist |
| --- | --- | --- | --- | --- |
| Schools | Driving, Flight, Bike, Boat | `Mission.LoadAndLaunchInternal`, `set_player_control`, `set_objective`, `set_timers`, `register_mission_passed`, `fail_current_mission` | `idle -> start -> objective_active -> checkpoint_progress -> pass/fail` + reconnect/late-join restore | `docs/qa/phase-schools-checklist.md` |
| Stadium events | 8-Track, Blood Bowl, Dirt Track, Kick Start | `Mission.LoadAndLaunchInternal`, `start_car_race`, `set_car_race_checkpoint`, `set_timers`, `register_mission_passed`, `fail_current_mission` | `idle -> start -> objective_active -> checkpoint_progress -> pass/fail` + reconnect/late-join restore | `docs/qa/phase-stadium-events-checklist.md` |
| Hidden races | BMX, NRG-500, Chiliad Challenge | `Mission.LoadAndLaunchInternal`, `start_checkpoint_race`, `set_car_race_checkpoint`, `set_timers`, `register_mission_passed`, `fail_current_mission` | `idle -> start -> objective_active -> checkpoint_progress -> pass/fail` + reconnect/late-join restore | `docs/qa/phase-hidden-races-checklist.md` |
| Courier routes | LS, SF, LV routes | `Mission.LoadAndLaunchInternal`, `create_pickup`, `set_objective`, `set_timers`, `register_mission_passed`, `fail_current_mission` | `idle -> start -> objective_active -> delivery_progress -> pass/fail` + reconnect/late-join restore | `docs/qa/phase-courier-routes-checklist.md` |
| Street races | Street Racing set (22 races) | `Mission.LoadAndLaunchInternal`, `start_car_race`, `set_car_race_checkpoint`, `task_car_drive_to_coord`, `register_mission_passed`, `fail_current_mission` | `idle -> start -> objective_active -> checkpoint_progress -> pass/fail` + reconnect/late-join restore | `docs/qa/phase-street-races-checklist.md` |
| Import/Export | Import/Export list sets and turn-ins | `Mission.LoadAndLaunchInternal`, `create_car`, `set_objective`, `set_player_money`, `register_mission_passed`, `fail_current_mission` | `idle -> start -> objective_active -> delivery_progress -> pass/fail` + reconnect/late-join restore | `docs/qa/phase-import-export-checklist.md` |

#### Schools execution checklist

Use the fixed sync gates for every schools mode with no substitutions: `idle -> start -> objective_active -> checkpoint_progress -> pass/fail` + reconnect/late-join restore.

1. **Shared sync primitives** (common schools state + restore plumbing)
   - [x] Driving — owner: _unassigned_; status: `done`
   - [x] Flight — owner: _unassigned_; status: `done`
   - [x] Bike — owner: _unassigned_; status: `done`
   - [x] Boat — owner: _unassigned_; status: `done`
2. **Mode-specific script/opcode mapping** (`Mission.LoadAndLaunchInternal`, `set_player_control`, `set_objective`, `set_timers`, `register_mission_passed`, `fail_current_mission`)
   - [x] Driving — owner: _unassigned_; status: `done`
   - [x] Flight — owner: _unassigned_; status: `done`
   - [x] Bike — owner: _unassigned_; status: `done`
   - [x] Boat — owner: _unassigned_; status: `done`
3. **QA sign-off** (`docs/qa/phase-schools-checklist.md`)
   - [x] Driving — owner: _unassigned_; status: `done`
   - [x] Flight — owner: _unassigned_; status: `done`
   - [x] Bike — owner: _unassigned_; status: `done`
   - [x] Boat — owner: _unassigned_; status: `done`

#### Schools phase completion note (2026-03-27)

All four required schools parity gates are complete with checklist evidence:
- objective parity — [Driving](docs/qa/phase-schools-checklist.md#driving-school-evidence), [Flight](docs/qa/phase-schools-checklist.md#flight-school-evidence), [Bike](docs/qa/phase-schools-checklist.md#bike-school-evidence), [Boat](docs/qa/phase-schools-checklist.md#boat-school-evidence)
- fail/pass parity — [Driving](docs/qa/phase-schools-checklist.md#driving-school-evidence), [Flight](docs/qa/phase-schools-checklist.md#flight-school-evidence), [Bike](docs/qa/phase-schools-checklist.md#bike-school-evidence), [Boat](docs/qa/phase-schools-checklist.md#boat-school-evidence)
- reconnect parity — [Driving](docs/qa/phase-schools-checklist.md#driving-school-evidence), [Flight](docs/qa/phase-schools-checklist.md#flight-school-evidence), [Bike](docs/qa/phase-schools-checklist.md#bike-school-evidence), [Boat](docs/qa/phase-schools-checklist.md#boat-school-evidence)
- late-join parity — [Driving](docs/qa/phase-schools-checklist.md#driving-school-evidence), [Flight](docs/qa/phase-schools-checklist.md#flight-school-evidence), [Bike](docs/qa/phase-schools-checklist.md#bike-school-evidence), [Boat](docs/qa/phase-schools-checklist.md#boat-school-evidence)

#### Stadium events execution checklist

Use the fixed sync gates for every stadium mode with no substitutions: `idle -> start -> objective_active -> checkpoint_progress -> pass/fail` + reconnect/late-join restore.

1. **Shared sync primitives** (common stadium state + restore plumbing)
   - [x] 8-Track — owner: _unassigned_; status: `done`
   - [x] Blood Bowl — owner: _unassigned_; status: `done`
   - [x] Dirt Track — owner: _unassigned_; status: `done`
   - [x] Kick Start — owner: _unassigned_; status: `done`
2. **Mode-specific script/opcode mapping** (`Mission.LoadAndLaunchInternal`, `start_car_race`, `set_car_race_checkpoint`, `set_timers`, `register_mission_passed`, `fail_current_mission`)
   - [x] 8-Track — owner: _unassigned_; status: `done`
   - [x] Blood Bowl — owner: _unassigned_; status: `done`
   - [x] Dirt Track — owner: _unassigned_; status: `done`
   - [x] Kick Start — owner: _unassigned_; status: `done`
3. **QA sign-off** (`docs/qa/phase-stadium-events-checklist.md`)
   - [x] 8-Track — owner: _unassigned_; status: `done` (objective ✅, fail/pass ✅, reconnect ✅, late-join ✅)
   - [x] Blood Bowl — owner: _unassigned_; status: `done` (objective ✅, fail/pass ✅, reconnect ✅, late-join ✅)
   - [x] Dirt Track — owner: _unassigned_; status: `done` (objective ✅, fail/pass ✅, reconnect ✅, late-join ✅)
   - [x] Kick Start — owner: _unassigned_; status: `done` (objective ✅, fail/pass ✅, reconnect ✅, late-join ✅)

#### Stadium events phase completion note (2026-03-27)

- Stadium events phase is `done` because all four parity gates passed for each mode in scope.
- Validation/evidence source of truth: `docs/qa/phase-stadium-events-checklist.md`.

### Progress tracking (per-mode)

Allowed status values: `not started`, `in progress`, `done`.

| Phase | Mode | Status |
| --- | --- | --- |
| Schools | Driving school | done |
| Schools | Flight school | done |
| Schools | Bike school | done |
| Schools | Boat school | done |
| Stadium events | 8-Track | done |
| Stadium events | Blood Bowl | done |
| Stadium events | Dirt Track | done |
| Stadium events | Kick Start | done |
| Hidden races | BMX | not started |
| Hidden races | NRG-500 | not started |
| Hidden races | Chiliad Challenge | not started |
| Courier routes | Los Santos courier | not started |
| Courier routes | San Fierro courier | not started |
| Courier routes | Las Venturas courier | not started |
| Street races | Street Racing set (22 races) | not started |
| Import/Export | Import/Export | not started |

### Phase release-note cadence (for community testing)
After completing each phase, publish a short release-note style update with:
1. **What shipped:** mode list and notable sync behaviors.
2. **Parity result:** objective/fail-pass/reconnect/late-join outcome.
3. **Known issues:** top regressions or edge cases.
4. **How to test:** one quick reproduction path + QA checklist link.

Release-note log:
- _No phase release notes published yet._
