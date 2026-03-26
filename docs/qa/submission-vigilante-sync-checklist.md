# Submission Sync QA — Vigilante

## Scope
Validate host-authoritative Vigilante submission sync behavior.

## Scenario matrix
1. Start Vigilante in a supported law-enforcement vehicle and confirm active sync.
2. Complete multiple target eliminations and verify deterministic progress/reward deltas.
3. Exercise fail and pass outcomes and confirm stop state broadcast.
4. Reconnect peer mid-run and validate snapshot recovery.
5. Late-join peer mid-run and validate immediate snapshot hydrate.
6. Host migration mid-run and validate authoritative state continuity.

## Pass criteria
- Host-authoritative progression/payout events are identical across peers.
- No stale active state after fail/pass.
- Reconnect/late-join/host-migration restore expected state.
