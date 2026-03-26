# Submissions Sync Matrix

Validate host-authoritative submissions sync behavior for `Taxi`, `Firefighter`, `Vigilante`, `Paramedic`, `Pimp`, and `Freight Train`.

## Coverage goals
- Start/stop state convergence across all peers.
- Progression convergence (level/stage/progress/timer/score/reward/outcome/participants).
- Reconnect + late-join snapshot hydration correctness.
- Host migration continuity.
- Anti-duplication safeguards for reward/progress deltas.

## Roles
- **H**: current host.
- **P1/P2**: already connected peers.
- **J**: joining peer (late-join / reconnect case).

## Scenario matrix

| ID | Mode(s) | Scenario | Steps | Expected result |
|---|---|---|---|---|
| SS-01 | Taxi | Simultaneous start attempt | 1) P1 and P2 both enter Taxi-class vehicles. 2) P1 (host) starts Taxi run while P2 attempts to start at near-same time. | Host state is authoritative; all peers converge on a single active Taxi submission state (same mode, level/stage/progress/timer). |
| SS-02 | All | Start -> stop lifecycle | 1) Host starts a submission run. 2) Host exits/invalidates mode vehicle to stop run. | `active` transitions to `1` then `0` exactly once per transition on all peers; no stale active state after stop. |
| SS-03 | Taxi | Progress + payout flow | 1) Host starts Taxi run. 2) Complete multiple fares that trigger reward updates. | Peers replicate matching `progress`, `score`, `reward`, `participantCount`, and monotonic `stateVersion`. |
| SS-04 | Firefighter/Vigilante/Paramedic/Pimp/Freight | Adapter parity | Repeat SS-03 in each non-Taxi submission mode. | Same host-authoritative progression and payout behavior as Taxi with mode-specific vehicle adapter only. |
| SS-05 | All | Pass outcome | 1) Run submission to success path. 2) Observe final outcome state. | `outcome=pass` (`1`) and `active=0` converge for all peers with no duplicate completion packets. |
| SS-06 | All | Fail outcome | 1) Run submission to fail path. 2) Observe final outcome state. | `outcome=fail` (`2`) and `active=0` converge for all peers with no duplicate failure packets. |
| SS-07 | Taxi + one other | Reconnect hydrate | 1) During active run, disconnect P2. 2) Reconnect as same user/session. | Reconnected peer receives snapshot immediately and renders current state without waiting for next delta. |
| SS-08 | Taxi + one other | Late-join hydrate | 1) Keep run active. 2) New peer J joins. | J receives full snapshot (`begin/entry/end`) and renders exact active state instantly. |
| SS-09 | All | Host migration continuity | 1) Keep run active. 2) Current host leaves; new host elected. | New host rebroadcasts cached submission snapshot; peers converge without reset or duplicated payouts. |
| SS-10 | All | Anti-duplication reward checks | 1) Trigger rapid reward-producing events. 2) Compare each peer's score/reward totals over same interval. | Totals remain identical across peers; no duplicate reward increments from replay/resync. |
| SS-11 | All | Anti-duplication snapshot replay | 1) Force resync (reconnect/host migration). 2) Ensure no extra progression/reward increments occur post-snapshot. | Snapshot hydration updates visual state only; progression/reward does not double-apply. |

## Completion criteria per submission type
A submission type is **done** when SS-02 through SS-11 pass for that type in multiplayer validation logs.
