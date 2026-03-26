# CoopAndreas Roadmap

Detailed backlog extracted from `README.md` sections: **Current Tasks**, **TODO Launcher**, **TODO Missions**, and **TODO Other Scripts**.

Legend:
- **Priority:** `P0` = critical for playable co-op baseline, `P1` = important parity/quality, `P2` = optional or long-tail.
- **Effort:** `S` (small), `M` (medium), `L` (large).

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
- [~] Gang wars sync (host-authoritative lifecycle + snapshots). **[P2][L]** 🚧 In progress (2026-03-26).
  - **Acceptance criteria:** host emits authoritative lifecycle events for start trigger, wave progression, and win/loss outcome.
  - **Acceptance criteria:** territory ownership updates remain synchronized with existing gang-zone owner/color propagation paths.
  - **Acceptance criteria:** reconnecting and late-join peers receive lifecycle snapshot + latest ownership state without manual refresh.
  - **Status update (2026-03-26):** added `GANG_WAR_LIFECYCLE_EVENT` replication path and late-join snapshot replay on server; QA checklist coverage added for start/finish, reconnect parity, and ownership persistence.
- [ ] Parachute jump sync. **[P2][M]**
- [ ] Stunt systems (`collecting`, per-player slow motion). **[P2][M]**
- [ ] Chat gamepad support with on-screen keyboard. **[P2][M]**
- [ ] Minor ideas: sync laser sniper red dot, moon-sniper easter egg shot size effect. **[P2][S]**

## Milestone: Mission Parity

### P1
- [ ] Storyline mission parity checklist (`Cleaning The Hood` → `End Of The Line`, excluding already complete: `Big Smoke`, `Ryder`, `Tagging Up Turf`). **[P1][L]**

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

#### SWEET mission wave 1 implementation notes (selected from first two `not started` rows)

- **Cleaning The Hood** (`Mission.LoadAndLaunchInternal(14)` in `SWEET.txt`)
  - Blocking opcodes: `Mission.LoadAndLaunchInternal`, `create_actor`, `set_char_obj_kill_char_any_means`.
  - Mission state transitions covered in sync flow:
    - **start:** mission launch + cutscene start trigger propagation.
    - **active objectives:** objective/help text propagation (`print_big`, `print_now`, `print_help`).
    - **fail:** `fail_current_mission` propagation.
    - **pass:** `register_mission_passed` propagation.
    - **reconnect resume:** server snapshot replay of mission flag + latest mission flow event.
- **Drive-Thru** (`Mission.LoadAndLaunchInternal(15)` in `SWEET.txt`)
  - Blocking opcodes: `create_car`, `task_drive_by`, `set_char_obj_destroy_car`.
  - Mission state transitions covered in sync flow:
    - **start:** mission launch + cutscene start trigger propagation.
    - **active objectives:** objective/help text propagation (`print_big`, `print_now`, `print_help`).
    - **fail:** `fail_current_mission` propagation.
    - **pass:** `register_mission_passed` propagation.
    - **reconnect resume:** server snapshot replay of mission flag + latest mission flow event.

## Milestone: Launcher UX

### P1
- [x] Start/control server directly from launcher. **[P1][M]** ✅ Completed (2026-03-26).

## Milestone: Optional Content

### P2
- [ ] Property purchase sync. **[P2][M]**
- [ ] Submissions: `Taxi`, `Firefighter`, `Vigilante`, `Paramedic`, `Pimp`, `Freight Train`. **[P2][L]**
- [ ] Hidden races: `BMX`, `NRG-500`, `Chiliad Challenge`. **[P2][M]**
- [ ] Stadium events: `8-Track`, `Blood Bowl`, `Dirt Track`, `Kick Start`. **[P2][M]**
- [ ] Ammu-Nation challenge. **[P2][S]**
- [ ] Schools: `Driving`, `Flight`, `Bike`, `Boat`. **[P2][L]**
- [ ] Asset missions: `Trucker`, `Valet`, `Career`. **[P2][L]**
- [ ] Courier sets: LS/SF/LV routes. **[P2][M]**
- [ ] Street Racing set (22 races). **[P2][L]**
- [ ] Import / Export. **[P2][M]**
