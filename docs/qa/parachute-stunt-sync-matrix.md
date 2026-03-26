# QA Matrix — Parachute + Stunt Sync

Validate host-authoritative parachute and stunt synchronization with reconnect/late-join parity, reward idempotency, and high-latency resilience.

## Scope
- Parachute lifecycle replication: `deploy` → `glide` → `cut` → (`landing` | `fail`).
- Stunt event trigger replication and per-player scoring isolation.
- Stunt collection/checkpoint progress replication.
- Reconnect and late-join hydration without duplicated rewards or progress resets.
- Simultaneous multi-player stunts under normal and high-latency conditions.

## Test topology
- **H** = host (authoritative mission/stunt state emitter)
- **C1** = active stunt participant
- **C2** = second stunt participant (simultaneous coverage)
- **C3** = reconnect/late-join verifier

Recommended: 4 clients + server, with optional network shaping (`tc/netem` or equivalent) enabled on C1/C2.

## Preconditions
1. Enable debug logging for mission/parachute/stunt deltas on host and peers.
2. Use a route that includes high-altitude parachute access and at least one stunt collection/checkpoint loop.
3. Start with all players in the same `currArea` and synchronized world/weather.

## Test cases

| ID | Scenario | Steps | Expected result |
| --- | --- | --- | --- |
| PS-01 | Parachute lifecycle ordered sync | H performs full parachute run with explicit `deploy`, sustained `glide`, manual `cut`, then safe ground contact. | C1/C2 observe matching ordered transitions without missing/interleaved states. |
| PS-02 | Parachute fail branch | H deploys then impacts terrain/water to force fail branch. | Peers receive `fail` terminal state (not `landing`), and no stale glide state remains active. |
| PS-03 | Late-join mid-glide | C3 joins while H is in `glide`. | C3 snapshot hydrates current parachute state immediately; no default/on-foot flashback before next delta. |
| PS-04 | Reconnect during transition | Disconnect C1 after `deploy`; reconnect during `glide` or immediately after `cut`. | C1 resumes authoritative in-progress state and terminal outcome; no duplicate lifecycle replay side effects. |
| PS-05 | Simultaneous stunt triggers | C1 and C2 trigger distinct stunt events within 2s window. | Event feeds remain per-player; neither player's score/slow-motion state overwrites the other. |
| PS-06 | Per-player score isolation | Run 3–5 stunt completions alternating C1/C2. | Score totals update only for triggering player; no bleed-through to other clients. |
| PS-07 | Checkpoint/collection progress replication | C1 advances checkpoint chain while C2 watches; then C2 advances independent chain. | Progress counters and completed markers replicate correctly per player and persist in snapshots. |
| PS-08 | Reward idempotency on reconnect | Force reconnect right after reward grant edge (checkpoint completion or stunt completion). | Replayed snapshot does not re-grant reward cash/points; totals stay unchanged after reconnect. |
| PS-09 | Late-join after reward grant | C3 joins immediately after a completed stunt reward. | C3 sees final score/progress state only; no extra reward trigger fires. |
| PS-10 | High-latency concurrent stunts | Apply 150–250ms latency + 2–5% packet loss on C1/C2; run simultaneous stunts. | Host-authoritative ordering prevents duplicate rewards and invalid rollbacks; eventual state convergence across peers. |
| PS-11 | Host migration guardrail (if supported) | During in-progress stunt, migrate host or restart authoritative peer path. | New authority resumes latest committed state, preserving reward idempotency and checkpoint progress. |

## Pass criteria
- Lifecycle ordering is deterministic and host-authoritative for all observed parachute runs.
- No duplicated stunt rewards after reconnect/late-join snapshot replay.
- Per-player scoring and checkpoint/collection progress remain isolated under concurrent play.
- High-latency tests converge to identical authoritative state within expected interpolation windows.

## Failure signals
- Replayed reward grants on reconnect/late-join.
- Any player inheriting another player’s stunt score/progress.
- Global slow-motion side effects affecting non-participating players.
- Terminal parachute mismatch (`landing` vs `fail`) across peers.
