# Submission Sync QA — Freight Train

## Scope
Validate host-authoritative Freight Train submission sync behavior.

## Scenario matrix
1. Start Freight Train mission and verify active state propagation.
2. Advance checkpoints and verify deterministic progress/reward deltas.
3. Verify fail/pass stop transitions clear active state.
4. Reconnect during active mission and verify snapshot replay.
5. Late join during active mission and verify immediate snapshot hydrate.
6. Host migration during active mission and verify state continuity from new host.

## Pass criteria
- Lifecycle/progression/timer/reward parity across peers.
- Reconnect + late join restore active state immediately.
- Host migration does not desync mission state.
