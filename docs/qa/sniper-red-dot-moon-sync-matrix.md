# Sniper red-dot + moon-sniper sync QA matrix

Date: 2026-03-26  
Scope: host-authoritative sniper aim marker replication + deterministic moon-sniper shot-size payload parity.

## Environment
- Host (H), Client 1 (C1), Client 2 (C2) in the same co-op session.
- One sniper rifle available for H and C1.

## Scenarios

| ID | Scenario | Steps | Expected result | Status |
| --- | --- | --- | --- | --- |
| SN-01 | 2-peer baseline sniper red-dot alignment | 1) H hosts and C1 joins. 2) H equips sniper and sweeps aim over static world geometry. 3) Compare H vs C1 red-dot world placement over 30s. | C1 sees red-dot aligned to host-replicated marker trajectory, with no independent simulation drift. | PASS |
| SN-02 | Late-join during active sniper aim | 1) H keeps sniper aim active. 2) C2 joins mid-aim. 3) Observe C2 immediately after join. | C2 hydrates active marker state from snapshot/replay path and renders current marker without waiting for a new trigger. | PASS |
| SN-03 | Moon-sniper active vs inactive shot-size parity | 1) Fire a standard sniper shot (inactive condition). 2) Fire a moon-sniper-condition shot. 3) Compare replicated shot-size behavior on C1/C2. | Inactive shot uses multiplier `1.0`; active shot uses replicated multiplier (>1.0) consistently across peers. | PASS |
| SN-04 | Reconnect consistency | 1) Keep H aiming sniper. 2) Disconnect C1 and reconnect. 3) Repeat with moon-sniper active/inactive shots. | Reconnected client restores active marker state and receives consistent shot-size parity (`1.0` default when inactive). | PASS |

## Pass criteria
- All SN-01..SN-04 scenarios remain `PASS`.
