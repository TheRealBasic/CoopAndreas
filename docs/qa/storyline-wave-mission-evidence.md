# Storyline wave mission QA evidence

This checklist is the promotion gate for storyline-wave statuses in `docs/ROADMAP.md`.

## Status promotion policy

- A mission may remain `not started` with blank validation notes.
- A mission may move to `in progress` **only after** initial notes are captured for all six validation gates: `start/cutscene`, `objective`, `fail`, `pass`, `reconnect`, and `late-join`.
- A mission may move to `done` **only after** all six validation gates are marked `pass` with concrete evidence references (session id, log, clip, or trace).
- If reconnect behavior regresses, mission status must roll back to `in progress` until reconnect notes are refreshed.

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
| Nines And AK's | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Must fill all six gates before mission status may change. |
| Drive-By | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Must fill all six gates before mission status may change. |
| Sweet's Girl | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Must fill all six gates before mission status may change. |
| Cesar Vialpando | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Must fill all six gates before mission status may change. |

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
