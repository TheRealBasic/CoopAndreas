# Gang Wars Sync Matrix (multi-peer)

## Objective
Validate complete gang-war synchronization parity across host + peers, including lifecycle ordering, authority guarantees, reconnect/late-join recovery, and territory persistence.

## Coverage requirements
- Full lifecycle events: trigger, wave progression, reinforcement logic, completion, failure, and zone ownership change.
- Host-authoritative relay only, with deterministic event order on all peers.
- Reconnect and late-join snapshot hydration during in-progress wars and after final territory updates.
- Interior/area (`currArea`) transitions and mission overlap scenarios do not suppress lifecycle propagation.

## Test topology
- Minimum peers: 1 host + 3 clients.
- Recommended labels:
  - **H** = host (authoritative event source)
  - **C1** = baseline connected client
  - **C2** = reconnect test client
  - **C3** = late-join test client
- Session requirements:
  - At least 2 turf zones tested.
  - At least 1 war ended in **win** and 1 in **loss/failure** outcome.
  - At least 1 scenario run while another mission flow is active (or recently transitioned).

## Event checklist
Track these events in sequence for each scenario:
1. `trigger/start`
2. `wave_1`
3. `wave_2+`
4. `reinforcement`
5. `completion` or `failure`
6. `territory_owner_changed` (if applicable)
7. `post_outcome_snapshot` (reconnect/late-join verification)

---

## Scenario matrix

| ID | Scenario | Steps | Expected sync result | Pass/Fail rule |
| --- | --- | --- | --- | --- |
| GW-01 | Baseline start trigger parity | Trigger war on **H** while **C1/C2** connected. | All peers enter active war state from same trigger event; no client-local start accepted. | **PASS** if all peers show identical active start state and same first event index; **FAIL** if any peer starts from local-only trigger. |
| GW-02 | Deterministic wave ordering | Progress war through at least 3 wave transitions. | **C1/C2** receive wave events in identical order/index as **H**, with no duplicate/out-of-order application. | **PASS** if event order and count match exactly across peers; **FAIL** on any skipped, duplicate, or reordered wave. |
| GW-03 | Reinforcement logic parity | Force reinforcement conditions (player leaves zone/slow clear/etc.) during active wave. | Reinforcement event and resulting phase match on **H/C1/C2**; no divergent reinforcement state. | **PASS** if reinforcement state + timing window are consistent for all peers; **FAIL** if any peer misses or independently spawns reinforcement transition. |
| GW-04 | Win completion + ownership change | Finish a war with victory that flips territory owner. | Outcome event + territory owner/color/state change are identical on all connected peers. | **PASS** if owner/color/state converge and remain stable for 60s; **FAIL** on any mismatch or rollback. |
| GW-05 | Failure outcome parity | Run a war to failure/loss conditions. | Failure event is authoritative from **H** and all peers converge to same inactive/failure state. | **PASS** if all peers settle on identical failure state; **FAIL** if any peer reports win/active mismatch. |
| GW-06 | Mid-war reconnect snapshot | Disconnect **C2** during active wave; reconnect before outcome. | **C2** hydrates current lifecycle phase (current wave/reinforcement/outcome-pending) from snapshot without waiting for next transition. | **PASS** if **C2** state matches **H** immediately on rejoin; **FAIL** if stale/default state appears until next event. |
| GW-07 | Late-join in-progress snapshot | Have **C3** join while war is active mid-wave. | **C3** receives in-progress war snapshot and renders current phase immediately. | **PASS** if **C3** matches current host phase within initial sync window; **FAIL** if it starts idle or from wrong phase. |
| GW-08 | Late-join post-outcome territory state | End war, then connect **C3** after outcome is finalized. | **C3** receives latest final lifecycle event + final territory ownership state on join. | **PASS** if final outcome + owner/color/state match **H** immediately; **FAIL** on stale pre-war territory data. |
| GW-09 | Interior/area transition resilience | During active war, move **H** and at least one client through interior/area changes (`currArea` transitions). | Lifecycle/ownership events continue flowing; no suppression due to area mismatch. | **PASS** if no missed events across transitions; **FAIL** if transition pauses/drops lifecycle propagation. |
| GW-10 | Mission overlap resilience | Start/exit another mission flow near active gang war lifecycle changes. | Gang-war sync events remain authoritative and ordered; mission events do not suppress or reorder gang-war events. | **PASS** if gang-war event stream is uninterrupted and ordered; **FAIL** if events are suppressed, delayed past order, or misapplied. |

---

## Determinism verification notes
For each scenario, capture event logs (host and clients) and compare:
- Event type sequence equality.
- Monotonic lifecycle index/sequence id equality.
- Outcome + territory state hash equality after convergence.

Minimum deterministic acceptance:
- 0 out-of-order lifecycle applications.
- 0 duplicate lifecycle applications.
- 0 client-authored lifecycle state transitions accepted by server relay path.

## Final acceptance criteria
Mark gang wars sync as complete only when all of the following are true:
1. All matrix scenarios **GW-01 ... GW-10** pass in the same build.
2. At least one reconnect and one late-join case pass for both in-progress and post-outcome states.
3. Both completion and failure outcomes are verified with consistent peer convergence.
4. Territory ownership results remain stable after reconnect and after a subsequent separate war.

## Execution record template
Use this table when executing test runs:

| Date (UTC) | Build/Commit | Scenario ID | Peers | Result (PASS/FAIL) | Notes / Evidence |
| --- | --- | --- | --- | --- | --- |
|  |  | GW-01 | H,C1,C2 |  |  |
|  |  | GW-02 | H,C1,C2 |  |  |
|  |  | GW-03 | H,C1,C2 |  |  |
|  |  | GW-04 | H,C1,C2 |  |  |
|  |  | GW-05 | H,C1,C2 |  |  |
|  |  | GW-06 | H,C1,C2 |  |  |
|  |  | GW-07 | H,C1,C3 |  |  |
|  |  | GW-08 | H,C1,C3 |  |  |
|  |  | GW-09 | H,C1,C2 |  |  |
|  |  | GW-10 | H,C1,C2 |  |  |
