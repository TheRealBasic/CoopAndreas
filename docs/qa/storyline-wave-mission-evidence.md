# Storyline wave mission QA evidence

This checklist is the promotion gate for storyline-wave statuses in `docs/ROADMAP.md`.

## Status promotion policy

- A mission may remain `not started` with blank validation notes.
- A mission may move to `in progress` **only after** initial notes are captured for all six validation gates: `start/cutscene`, `objective`, `fail`, `pass`, `reconnect`, and `late-join`.
- A mission may move to `in progress` only if its notes/evidence explicitly reference `docs/qa/storyline-mission-template.md` (mission onboarding template).
- A mission may move to `done` **only after** all six validation gates are marked `pass` with concrete evidence references (session id, log, clip, or trace).
- A mission may move to `functional` **only after** all six core gameplay gates are `pass`; `functional` should be treated as equivalent to parity-complete gameplay readiness for wave promotion.
- If reconnect behavior regresses, mission status must roll back to `in progress` until reconnect notes are refreshed.
- Final storyline missions (high-transition combat/chase/cutscene arcs) must explicitly carry forward hardened systems from prior waves (`WB-FIX-001..WB-FIX-004`) in their evidence notes.
- Multi-stage final missions must record terminal handling per stage (fail/pass) and prove stage-local terminal idempotency.
- Final storyline rows may be promoted to `done` only after stage-by-stage functional sign-off plus stable replay evidence under reconnect **and** host migration.
- A wave may not be promoted to `complete` until `python3 tools/opcode_audit.py --output docs/qa/storyline-opcode-backlog.md` has been run at both wave start and wave end, and the review entries are recorded in `docs/qa/storyline-opcode-backlog.md`.

## Evidence notation

- `not started`: no run completed yet.
- `in progress`: run started; partial evidence collected.
- `pass`: validation gate confirmed and evidence linked.
- `fail`: validation gate tested and currently failing.

---

## W1 — SWEET family (`scm/scripts/SWEET.txt`)

Owner: **Mission Sync Pod (Sweet stream)**  
Wave status: `in progress`

| Mission | Status | Start/cutscene validation | Objective validation | Fail validation | Pass validation | Reconnect validation | Late-join validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Cleaning The Hood | `done` | `pass` — `CTH-START-01` start + cutscene sync validated in `NET-W1-2026-03-30-A`. | `pass` — `CTH-OBJ-01` objective propagation matched host ordering. | `pass` — `CTH-TERM-FAIL-01` fail terminal event emitted once across peers. | `pass` — `CTH-TERM-PASS-01` pass terminal event + reward parity validated. | `pass` — `CTH-RECON-01` reconnect restore resumed active stage without restart. | `pass` — `CTH-LATE-01` late-join hydration restored in-progress stage without opening replay. | Evidence: `docs/qa/sweet-w1-cleaning-drive-thru-parity.md` (CTH scenarios). |
| Drive-Thru | `done` | `pass` — `DTH-START-01` mission start/setup sync validated across peers. | `pass` — `DTH-OBJ-01` objective propagation remained deterministic for drive-by phases. | `pass` — `DTH-TERM-FAIL-01` fail terminal parity validated on vehicle-destroyed condition. | `pass` — `DTH-TERM-PASS-01` pass terminal parity validated with single reward payout. | `pass` — `DTH-RECON-01` reconnect restore resumed active drive-by state. | `pass` — `DTH-LATE-01` late-join hydration restored active phase without intro replay. | Evidence: `docs/qa/sweet-w1-cleaning-drive-thru-parity.md` (DTH scenarios). |
| Nines And AK's | `done` | `pass` — `NAA-01` intro/start handshake synchronized once in `NET-W1-2026-03-31-B` (no duplicate launch/cutscene edges). | `pass` — `NAA-02` tutorial -> combat objective progression stayed monotonic via objective text token + phase index sync. | `pass` — `NAA-04` fail branch emitted exactly once (`player_death` path) with shared terminal adjudication latch. | `pass` — `NAA-05` completion pass + reward emitted once (`all_enemies_down`) with idempotent terminal replay guard. | `pass` — `NAA-06` reconnect restored in-progress combat stage/objective without intro replay. | `pass` — `NAA-07` late-join hydrated active actors + objective phase with no duplicate terminal side effects. | Completed on 2026-03-31 using template `docs/qa/storyline-mission-template.md`; shared hardening reused: `WB-FIX-001`, `WB-FIX-003`, `WB-FIX-004`. |
| Drive-By | `done` | `pass` — `DBY-01` mission start + vehicle attach sync validated in `NET-W1-2026-03-31-C` with single launch/cutscene handshake. | `pass` — `DBY-02` route/wave objective transitions stayed monotonic and matched host stage order across peers. | `pass` — `DBY-04` vehicle-destroyed fail branch emitted one terminal fail outcome with shared adjudication latch. | `pass` — `DBY-05` all-enemies-cleared pass branch emitted once with reward idempotency preserved. | `pass` — `DBY-06` reconnect restored active route node, wave index, and vehicle health/occupants without intro replay. | `pass` — `DBY-07` late-join hydration restored in-progress wave + kill quota counters without duplicate terminal side effects. | Completed on 2026-03-31 using template `docs/qa/storyline-mission-template.md`; evidence: `artifacts/qa/storyline/drive-by/NET-W1-2026-03-31-C/` (`drive-by-route-wave-sync.log`, `drive-by-terminal-parity.log`, `drive-by-reconnect-restore.log`, `drive-by-late-join-hydration.log`, `drive-by-terminals.mp4`); shared hardening reused: `WB-FIX-003`, `WB-FIX-004`. |
| Sweet's Girl | `done` | `pass` — `SGR-01` start/cutscene/control-lock sync verified in `NET-WB-2026-03-30-A`. | `pass` — `SGR-02` escort/flee objective transitions matched host stage order. | `pass` — `SGR-04` protected-actor fail branch emitted one terminal fail across peers. | `pass` — `SGR-05` pass branch payout and completion emitted once with replay guard. | `pass` — `SGR-06` reconnect restored objective phase and seat bindings mid-attempt. | `pass` — `SGR-07` late-join hydration restored active state without intro replay. | Wave B completion on 2026-03-30 using template `docs/qa/storyline-mission-template.md`; blocking fix refs: `WB-FIX-001`, `WB-FIX-002`. |
| Cesar Vialpando | `done` | `pass` — `CVP-01` launch/cutscene sync verified in `NET-WB-2026-03-30-B`. | `pass` — `CVP-02` lowrider objective/checkpoint cadence remained deterministic across peers. | `pass` — `CVP-04` fail branch emitted once and cleared mission runtime snapshot safely. | `pass` — `CVP-05` pass branch reward emitted once with terminal latch parity. | `pass` — `CVP-06` reconnect restored active scoring phase without restarting mission attempt. | `pass` — `CVP-07` late-join hydration restored objective and control-handoff state. | Wave B completion on 2026-03-30 using template `docs/qa/storyline-mission-template.md`; blocking fix refs: `WB-FIX-002`, `WB-FIX-003`. |

## W2 — RYDER family (`scm/scripts/RYDER.txt`)

Owner: **Mission Sync Pod (Ryder stream)**  
Wave status: `done` (2026-03-31)

W2 execution order (implementation + validation): `Home Invasion` -> `Catalyst` -> `Robbing Uncle Sam`.

Promotion hold (W2): all three missions remain `in progress` until all six gates are `pass` and `docs/qa/storyline-mission-signoff-checklist.md` is complete; then promote to `done` with completion date + checklist reference.

| Mission | Status | Start/cutscene validation | Objective validation | Fail validation | Pass validation | Reconnect validation | Late-join validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Home Invasion | `done` | `pass` — `HIN-01` start/cutscene handshake remained single-fire in `NET-W2-2026-03-31-D`. | `pass` — `HIN-02` stealth-objective progression (`create_pickup` + `task_go_straight_to_coord` + timer edges) stayed monotonic under host authority. | `pass` — `HIN-04` fail paths (`noise/alert`, `player_death`) emitted one terminal fail across peers. | `pass` — `HIN-05` pass branch emitted once with reward idempotency preserved. | `pass` — `HIN-06` reconnect restored burglary phase + timer state without intro replay. | `pass` — `HIN-07` late-join hydrated active objective phase and pickup state without duplicate terminals. | Completed on 2026-03-31 (checklist: `docs/qa/storyline-mission-signoff-checklist.md`, evidence session `NET-W2-2026-03-31-D`); shared hardening reused: `WB-FIX-001`, `WB-FIX-002`, `WB-FIX-003`, `WB-FIX-004`. |
| Catalyst | `done` | `pass` — `CAT-01` launch/cutscene sync remained deterministic in `NET-W2-2026-03-31-E`. | `pass` — `CAT-02` train-chase/jump objective chain (`create_train` + `task_jump_from_car` + mission audio position) preserved host stage ordering. | `pass` — `CAT-04` fail paths (`target_escape`, `player_death`) produced one terminal fail outcome. | `pass` — `CAT-05` pass branch remained once-only with terminal replay guard. | `pass` — `CAT-06` reconnect restored mid-chase mission phase + target state deterministically. | `pass` — `CAT-07` late-join hydration restored active chase stage without cutscene replay. | Completed on 2026-03-31 (checklist: `docs/qa/storyline-mission-signoff-checklist.md`, evidence session `NET-W2-2026-03-31-E`); shared hardening reused: `WB-FIX-001`, `WB-FIX-002`, `WB-FIX-003`, `WB-FIX-004`. |
| Robbing Uncle Sam | `done` | `pass` — `RUS-01` start/cutscene sync validated as single-launch in `NET-W2-2026-03-31-F`. | `pass` — `RUS-02` crate lifecycle/attach objective chain (`create_object` + `attach_object_to_car`) stayed monotonic and authoritative. | `pass` — `RUS-04` fail paths (`crate_loss`, `player_death`) emitted one terminal fail event. | `pass` — `RUS-05` pass + reward remained once-only with shared terminal latch. | `pass` — `RUS-06` reconnect restored crate-load stage + scripted entity bindings. | `pass` — `RUS-07` late-join hydrated active crate phase and guards without duplicate terminal side effects. | Completed on 2026-03-31 (checklist: `docs/qa/storyline-mission-signoff-checklist.md`, evidence session `NET-W2-2026-03-31-F`); shared hardening reused: `WB-FIX-001`, `WB-FIX-002`, `WB-FIX-003`, `WB-FIX-004`. |

## W3 — SMOKE family (`scm/scripts/SMOKE.txt`)

Owner: **Mission Sync Pod (Smoke stream)**  
Wave status: `done` (2026-03-31)

| Mission | Status | Start/cutscene validation | Objective validation | Fail validation | Pass validation | Reconnect validation | Late-join validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| OG Loc | `done` | `pass` — `OGL-01` start/cutscene handshake remained deterministic in `NET-W3-2026-03-31-A`. | `pass` — `OGL-02` chase objective chain (`task_follow_nav_mesh_to_coord` -> `task_bike_flee_point` -> `set_objective`) stayed monotonic. | `pass` — `OGL-04` fail paths (`target_escape`, `player_death`) emitted once-only terminal fail across peers. | `pass` — `OGL-05` pass branch + reward remained idempotent. | `pass` — `OGL-06` reconnect restored chase phase and target lifecycle state without intro replay. | `pass` — `OGL-07` late-join hydration restored active chase stage without duplicate outcomes. | Completed on 2026-03-31 (checklist: `docs/qa/storyline-mission-signoff-checklist.md`, evidence session `NET-W3-2026-03-31-A`); shared hardening reused: `WB-FIX-001`, `WB-FIX-003`, `WB-FIX-004`. |
| Running Dog | `done` | `pass` — `RDG-01` actor spawn/cutscene sync verified in `NET-WB-2026-03-30-C`. | `pass` — `RDG-02` chase-to-recovery objective transitions stayed monotonic and host-authoritative. | `pass` — `RDG-04` fail branch produced one terminal event under player death and abort branches. | `pass` — `RDG-05` package recovery pass branch emitted once with idempotent reward replay. | `pass` — `RDG-06` reconnect restored chase/recovery stage and actor bindings without reset. | `pass` — `RDG-07` late-join hydration replayed active actor/objective state without intro rerun. | Wave B completion on 2026-03-30; template: `docs/qa/storyline-mission-template.md`; blocking fix refs: `WB-FIX-001`, `WB-FIX-004`. |
| Wrong Side Of The Tracks | `done` | `pass` — `WST-01` start/cutscene + train setup synced deterministically in `NET-W3-2026-03-31-B`. | `pass` — `WST-02` train chase + drive-by objective transitions (`create_train`, `task_drive_by`, `task_car_mission`) remained host-authoritative. | `pass` — `WST-04` fail paths (`train_lost`, `player_death`) emitted one terminal fail outcome. | `pass` — `WST-05` pass branch emitted once with reward idempotency preserved. | `pass` — `WST-06` reconnect restored train entity hydration + pursuit phase resume. | `pass` — `WST-07` late-join hydration restored active pursuit state without intro replay or duplicate terminal events. | Completed on 2026-03-31 (checklist: `docs/qa/storyline-mission-signoff-checklist.md`, evidence session `NET-W3-2026-03-31-B`); shared hardening reused: `WB-FIX-001`, `WB-FIX-003`, `WB-FIX-004`. |
| Just Business | `done` | `pass` — `JBU-01` intro-to-action phase transition synchronized in `NET-WB-2026-03-30-D`. | `pass` — `JBU-02` interior/escape/chase objective chain matched host ordering under jitter. | `pass` — `JBU-05` fail branch emitted one terminal fail on death and vehicle-destroyed conditions. | `pass` — `JBU-06` pass branch emitted one completion + reward packet across peers. | `pass` — `JBU-07` reconnect restored phase, wave index, and pursuit vehicle health budget. | `pass` — `JBU-07` late-join hydration restored active wave state and objective text without duplication. | Wave B completion on 2026-03-30; template: `docs/qa/storyline-mission-template.md`; blocking fix refs: `WB-FIX-003`, `WB-FIX-004`. |

## WC — CRASH family (`scm/scripts/CRASH.txt`)

Owner: **Mission Sync Pod (CRASH stream)**  
Wave status: `done` (2026-03-31)  
Kickoff date: `2026-03-31`

| Mission | Status | Start/cutscene validation | Objective validation | Fail validation | Pass validation | Reconnect validation | Late-join validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Burning Desire | `done` | `pass` — `BD-01` launch/cutscene sync validated in `NET-WC-2026-03-31-A`. | `pass` — `BD-02` fire lifecycle + forced-exit objective transitions (`create_fire`, forced leave-car path) remained monotonic and authoritative. | `pass` — `BD-04` fail paths (`player_death`, fire timeout) emitted one terminal fail with shared latch. | `pass` — `BD-05` pass branch (Denise extraction) completed once with reward idempotency. | `pass` — `BD-06` reconnect restored active burn phase + scripted entity bindings. | `pass` — `BD-07` late-join hydration restored in-progress fire phase without duplicate terminals. | Completed on 2026-03-31 (checklist: `docs/qa/storyline-mission-signoff-checklist.md`, evidence session `NET-WC-2026-03-31-A`); blocker closure: `BD-BLK-01..03`; shared hardening reused: `WB-FIX-001`, `WB-FIX-002`, `WB-FIX-003`, `WB-FIX-004`. |
| Gray Imports | `done` | `pass` — `GI-01` launch/cutscene handshake validated in `NET-WC-2026-03-31-B`. | `pass` — `GI-02` actor spawn + kill objective + detonation transition chain (`create_actor`, `set_char_obj_kill_char_any_means`, `blow_up_car`) stayed deterministic. | `pass` — `GI-04` fail paths (`player_death`, target loss) emitted one terminal fail outcome. | `pass` — `GI-05` pass branch (warehouse clear + detonation) remained once-only. | `pass` — `GI-06` reconnect restored combat-to-detonation state with objective monotonicity preserved. | `pass` — `GI-07` late-join hydrated active phase across kill + blast transitions without duplicate side effects. | Completed on 2026-03-31 (checklist: `docs/qa/storyline-mission-signoff-checklist.md`, evidence session `NET-WC-2026-03-31-B`); blocker closure: `GI-BLK-01..03`; shared hardening reused: `WB-FIX-001`, `WB-FIX-002`, `WB-FIX-003`, `WB-FIX-004`. |

## W4 — RIOT/Endgame finale family (`scm/scripts/RIOT.txt`)

Owner: **Mission Sync Pod (Finale stream)**  
Wave status: `not started`

| Mission | Status | Stage map | Start/cutscene validation | Objective validation | Stage terminal validation (fail/pass per stage) | Reconnect validation | Host-migration validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Riot | `not started` | `single-stage` | `not started` | `not started` | `not started` | `not started` | `not started` | Endgame target mission: apply `WB-FIX-001..WB-FIX-004` before mission-local exceptions. |
| Los Desperados | `not started` | `single-stage` | `not started` | `not started` | `not started` | `not started` | `not started` | Endgame target mission: heavy combat transitions; require replay idempotency evidence. |
| End Of The Line (1) | `not started` | `stage-1` | `not started` | `not started` | `not started` | `not started` | `not started` | Multi-stage finale handling required: stage-local terminal fail/pass evidence + stable replay under reconnect/migration. |
| End Of The Line (2) | `not started` | `stage-2` | `not started` | `not started` | `not started` | `not started` | `not started` | Multi-stage finale handling required: stage-local terminal fail/pass evidence + stable replay under reconnect/migration. |
| End Of The Line (3) | `not started` | `stage-3` | `not started` | `not started` | `not started` | `not started` | `not started` | Multi-stage finale handling required: stage-local terminal fail/pass evidence + stable replay under reconnect/migration. |
