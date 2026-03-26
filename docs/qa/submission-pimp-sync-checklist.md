# Submission Sync QA — Pimp

## Scope
Validate host-authoritative Pimp submission sync behavior.

## Scenario matrix
1. Start Pimp mission and verify active state propagation.
2. Advance multiple steps and verify deterministic progress/reward deltas.
3. Verify fail/pass stop transitions clear active state.
4. Reconnect during active mission and verify snapshot replay.
5. Late join during active mission and verify instant state hydrate.
6. Host migration during active mission and verify resync continuity.

## Pass criteria
- Deterministic lifecycle/progression/reward state across peers.
- Snapshot recovery works for reconnect and late join.
- Host migration keeps state coherent.
