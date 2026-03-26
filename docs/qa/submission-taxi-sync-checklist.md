# Submission Sync QA — Taxi

## Scope
Validate host-authoritative Taxi submission synchronization for lifecycle, progression, timer/reward deltas, reconnect, and host migration.

## Scenario matrix
1. **Start sync**
   - Host enters Taxi/Cabbie and starts Taxi submission.
   - Verify all clients receive active state (`active=1`, `stage>=1`) within one network tick.
2. **In-progress progression**
   - Complete at least 2 fares.
   - Verify `progress` increments deterministically and reward accumulation is identical on all peers.
3. **Fail / pass stop path**
   - Fail one run (timeout/abort) and finish one run successfully.
   - Verify mission deactivates (`active=0`) with no stale stage/progress after stop.
4. **Reconnect snapshot**
   - During an active Taxi run, disconnect a client and reconnect.
   - Verify reconnecting peer receives immediate snapshot with current active stage/progress/timer/reward values.
5. **Late-join snapshot**
   - Start Taxi run, then join a new peer.
   - Verify new peer immediately renders active Taxi submission state (no waiting for next reward event).
6. **Host migration**
   - During active Taxi run, migrate host (old host leaves).
   - Verify new host rebroadcasts cached submission snapshot and all peers converge to the same state.

## Pass criteria
- All peers keep identical Taxi mission active/stage/progress/timer/reward state after each transition.
- Reconnect and late join always restore in-progress state from snapshot.
- Host migration does not reset or duplicate reward/progress deltas.
