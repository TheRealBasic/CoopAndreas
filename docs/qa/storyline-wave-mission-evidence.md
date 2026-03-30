# Storyline wave mission QA evidence

This checklist is the promotion gate for storyline-wave statuses in `docs/ROADMAP.md`.

## Status promotion policy

- A mission may remain `not started` with blank validation notes.
- A mission may move to `in progress` **only after** initial notes are captured for all six validation gates: `start/cutscene`, `objective`, `fail`, `pass`, `reconnect`, and `late-join`.
- A mission may move to `in progress` only if its notes/evidence explicitly reference `docs/qa/storyline-mission-template.md` (mission onboarding template).
- A mission may move to `done` **only after** all six validation gates are marked `pass` with concrete evidence references (session id, log, clip, or trace).
- A mission may move to `functional` **only after** all six core gameplay gates are `pass`; `functional` should be treated as equivalent to parity-complete gameplay readiness for wave promotion.
- If reconnect behavior regresses, mission status must roll back to `in progress` until reconnect notes are refreshed.
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
| Nines And AK's | `in progress` | `in progress` — `NAA-01` pipeline-ready; start/cutscene sync run queued for `NET-W1-2026-03-30-B`. | `in progress` — `NAA-02` objective progression trace checklist prepared from template. | `in progress` — `NAA-04` fail adjudication instrumentation bound to terminal latch logs. | `in progress` — `NAA-05` pass adjudication instrumentation bound to reward replay guard logs. | `in progress` — `NAA-06` reconnect restore capture steps prepared. | `in progress` — `NAA-07` late-join hydration capture steps prepared. | Wave A pipeline complete through implementation; **not functional** until all six gates are `pass`. Template: `docs/qa/storyline-mission-template.md`. |
| Drive-By | `in progress` | `in progress` — `DBY-01` mission start/vehicle attach sync run queued. | `in progress` — `DBY-02` route/wave objective gate trace checklist prepared. | `in progress` — `DBY-04` fail branch (vehicle destroyed) validation queued. | `in progress` — `DBY-05` pass branch + reward idempotency validation queued. | `in progress` — `DBY-06` reconnect route/wave hydration run queued. | `in progress` — `DBY-07` late-join wave hydration run queued. | Wave A pipeline complete through implementation; **not functional** until all six gates are `pass`. Template: `docs/qa/storyline-mission-template.md`. |
| Sweet's Girl | `in progress` | `in progress` — `SGR-01` start/cutscene/control-lock sync validation queued. | `in progress` — `SGR-02` escort/flee objective stage trace prepared. | `in progress` — `SGR-04` fail branch adjudication checks prepared. | `in progress` — `SGR-05` pass branch adjudication checks prepared. | `in progress` — `SGR-06` reconnect restore scenario prepared. | `in progress` — `SGR-07` late-join hydration scenario prepared. | Wave A pipeline complete through implementation; mission-specific edge handling added only after shared primitives. **Not functional** until all six gates are `pass`. Template: `docs/qa/storyline-mission-template.md`. |
| Cesar Vialpando | `in progress` | `in progress` — `CVP-01` launch/cutscene sync validation queued. | `in progress` — `CVP-02` lowrider objective/checkpoint validation queued. | `in progress` — `CVP-04` fail branch validation queued. | `in progress` — `CVP-05` pass branch validation queued. | `in progress` — `CVP-06` reconnect restore during scoring phase queued. | `in progress` — `CVP-07` late-join hydration during active attempt queued. | Wave A pipeline complete through implementation; mission-specific edge handling added only after shared primitives. **Not functional** until all six gates are `pass`. Template: `docs/qa/storyline-mission-template.md`. |

## W2 — RYDER family (`scm/scripts/RYDER.txt`)

Owner: **Mission Sync Pod (Ryder stream)**  
Wave status: `not started`

| Mission | Status | Start/cutscene validation | Objective validation | Fail validation | Pass validation | Reconnect validation | Late-join validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Home Invasion | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W2 may move to `in progress`. |
| Catalyst | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W2 may move to `in progress`. |
| Robbing Uncle Sam | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W2 may move to `in progress`. |

## W3 — SMOKE family (`scm/scripts/SMOKE.txt`)

Owner: **Mission Sync Pod (Smoke stream)**  
Wave status: `not started`

| Mission | Status | Start/cutscene validation | Objective validation | Fail validation | Pass validation | Reconnect validation | Late-join validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| OG Loc | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W3 may move to `in progress`. |
| Running Dog | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W3 may move to `in progress`. |
| Wrong Side Of The Tracks | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W3 may move to `in progress`. |
| Just Business | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W3 may move to `in progress`. |
