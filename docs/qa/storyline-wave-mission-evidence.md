# Storyline wave mission QA evidence

This checklist is the promotion gate for storyline-wave statuses in `docs/ROADMAP.md`.

## Status promotion policy

- A mission may remain `not started` with blank validation notes.
- A mission may move to `in progress` **only after** initial notes are captured for all five validation gates: `start`, `objective`, `fail`, `pass`, and `reconnect`.
- A mission may move to `done` **only after** all five validation gates are marked `pass` with concrete evidence references (session id, log, clip, or trace).
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

| Mission | Status | Start validation | Objective validation | Fail validation | Pass validation | Reconnect validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Cleaning The Hood | `in progress` | `in progress` — Host launch/cutscene trigger mirrored to clients in 2-peer run (2026-03-30). | `in progress` — objective/help text mirrored on mission stage transition. | `not started` | `not started` | `in progress` — reconnecting client resumes active mission flag + latest mission-flow event. | Pending formal capture IDs and clips before promotion to `done`. |
| Drive-Thru | `in progress` | `in progress` — mission start replicated with vehicle setup state. | `in progress` — objective sequence updates mirrored while drive-by phase active. | `not started` | `not started` | `in progress` — reconnect hydration restores current stage without restart. | Pending formal capture IDs and clips before promotion to `done`. |
| Nines And AK's | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Must fill all five gates before mission status may change. |
| Drive-By | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Must fill all five gates before mission status may change. |
| Sweet's Girl | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Must fill all five gates before mission status may change. |
| Cesar Vialpando | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Must fill all five gates before mission status may change. |

## W2 — RYDER family (`scm/scripts/RYDER.txt`)

Owner: **Mission Sync Pod (Ryder stream)**  
Wave status: `not started`

| Mission | Status | Start validation | Objective validation | Fail validation | Pass validation | Reconnect validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Home Invasion | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W2 may move to `in progress`. |
| Catalyst | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W2 may move to `in progress`. |
| Robbing Uncle Sam | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W2 may move to `in progress`. |

## W3 — SMOKE family (`scm/scripts/SMOKE.txt`)

Owner: **Mission Sync Pod (Smoke stream)**  
Wave status: `not started`

| Mission | Status | Start validation | Objective validation | Fail validation | Pass validation | Reconnect validation | Notes / evidence refs |
| --- | --- | --- | --- | --- | --- | --- | --- |
| OG Loc | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W3 may move to `in progress`. |
| Running Dog | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W3 may move to `in progress`. |
| Wrong Side Of The Tracks | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W3 may move to `in progress`. |
| Just Business | `not started` | `not started` | `not started` | `not started` | `not started` | `not started` | Required before W3 may move to `in progress`. |
