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
Wave status: `in progress`

| Mission | Status | Start/cutscene validation | Objective validation | Fail validation | Pass validation | Reconnect validation | Late-join validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Home Invasion | `in progress` | `in progress` — `HIN-01` start/cutscene handshake capture plan logged for next Ryder wave run. | `in progress` — `HIN-02` stealth-objective progression trace checklist prepared. | `in progress` — `HIN-04` fail branch (`noise/alert` + `player_death`) validation queued. | `in progress` — `HIN-05` pass branch + reward idempotency validation queued. | `in progress` — `HIN-06` reconnect restore gate defined for active burglary stage replay. | `in progress` — `HIN-07` late-join hydration gate defined for active objective stage. | Owner: Mission Sync Pod (Ryder stream). Onboarding template: `docs/qa/storyline-mission-template.md` (C3). |
| Catalyst | `in progress` | `in progress` — `CAT-01` launch/cutscene sync notes seeded for train-setup entry. | `in progress` — `CAT-02` chase/jump objective propagation checklist prepared. | `in progress` — `CAT-04` fail branch (`target_escape` + `player_death`) validation queued. | `in progress` — `CAT-05` pass branch + terminal once-only replay guard validation queued. | `in progress` — `CAT-06` reconnect restore gate defined for mid-chase mission phase. | `in progress` — `CAT-07` late-join hydration gate defined for active chase phase. | Owner: Mission Sync Pod (Ryder stream). Onboarding template: `docs/qa/storyline-mission-template.md` (C3). |
| Robbing Uncle Sam | `in progress` | `in progress` — `RUS-01` start/cutscene sync capture checklist prepared. | `in progress` — `RUS-02` forklift/crate objective propagation trace checklist prepared. | `in progress` — `RUS-04` fail branch (`crate_loss` + `player_death`) validation queued. | `in progress` — `RUS-05` pass branch + reward idempotency validation queued. | `in progress` — `RUS-06` reconnect restore gate defined for crate-load stage replay. | `in progress` — `RUS-07` late-join hydration gate defined for active crate phase. | Owner: Mission Sync Pod (Ryder stream). Onboarding template: `docs/qa/storyline-mission-template.md` (C3). |

## W3 — SMOKE family (`scm/scripts/SMOKE.txt`)

Owner: **Mission Sync Pod (Smoke stream)**  
Wave status: `in progress`

| Mission | Status | Start/cutscene validation | Objective validation | Fail validation | Pass validation | Reconnect validation | Late-join validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| OG Loc | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W3 may move to `in progress`. |
| Running Dog | `done` | `pass` — `RDG-01` actor spawn/cutscene sync verified in `NET-WB-2026-03-30-C`. | `pass` — `RDG-02` chase-to-recovery objective transitions stayed monotonic and host-authoritative. | `pass` — `RDG-04` fail branch produced one terminal event under player death and abort branches. | `pass` — `RDG-05` package recovery pass branch emitted once with idempotent reward replay. | `pass` — `RDG-06` reconnect restored chase/recovery stage and actor bindings without reset. | `pass` — `RDG-07` late-join hydration replayed active actor/objective state without intro rerun. | Wave B completion on 2026-03-30; template: `docs/qa/storyline-mission-template.md`; blocking fix refs: `WB-FIX-001`, `WB-FIX-004`. |
| Wrong Side Of The Tracks | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W3 may move to `in progress`. |
| Just Business | `done` | `pass` — `JBU-01` intro-to-action phase transition synchronized in `NET-WB-2026-03-30-D`. | `pass` — `JBU-02` interior/escape/chase objective chain matched host ordering under jitter. | `pass` — `JBU-05` fail branch emitted one terminal fail on death and vehicle-destroyed conditions. | `pass` — `JBU-06` pass branch emitted one completion + reward packet across peers. | `pass` — `JBU-07` reconnect restored phase, wave index, and pursuit vehicle health budget. | `pass` — `JBU-07` late-join hydration restored active wave state and objective text without duplication. | Wave B completion on 2026-03-30; template: `docs/qa/storyline-mission-template.md`; blocking fix refs: `WB-FIX-003`, `WB-FIX-004`. |

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
