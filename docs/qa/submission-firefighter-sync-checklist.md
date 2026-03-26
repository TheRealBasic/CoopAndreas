# Submission Sync QA — Firefighter

## Scope
Validate host-authoritative Firefighter submission sync for start, progression, fail/pass, reconnect, and host migration.

## Scenario matrix
1. Start Firefighter and confirm active state propagation.
2. Extinguish multiple targets and verify deterministic `progress` + reward accumulation on all peers.
3. Validate both fail and pass stop paths clear active state.
4. Reconnect a client mid-run and confirm snapshot restoration.
5. Add a late-join peer mid-run and confirm immediate active state reconstruction.
6. Perform host migration while active and verify cached state resync from the new host.

## Pass criteria
- No divergence in stage/progress/timer/reward state.
- Snapshot replay works for reconnect and late join.
- Host migration preserves mission continuity.
