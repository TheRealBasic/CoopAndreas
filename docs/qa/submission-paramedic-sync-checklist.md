# Submission Sync QA — Paramedic

## Scope
Validate host-authoritative Paramedic submission sync behavior.

## Scenario matrix
1. Start Paramedic in Ambulance and verify active state propagation.
2. Complete multiple patient pickups/deliveries and verify deterministic progress/reward deltas.
3. Validate stop behavior for both failure and successful completion.
4. Reconnect peer during active run and verify snapshot restoration.
5. Late join during active run and verify immediate snapshot restoration.
6. Host migration during active run and verify resync from new host.

## Pass criteria
- Stage/progress/timer/reward are consistent across peers.
- Reconnect + late join restore current active state immediately.
- Host migration does not lose or duplicate mission state.
