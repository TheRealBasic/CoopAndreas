# SWEET W1 parity evidence — Cleaning The Hood & Drive-Thru

Scope: mission parity gates for Wave 1 SWEET stabilization missions.

## Evidence bundle index

- Session record: `NET-W1-2026-03-30-A` (host + client + late-join client).
- Build: `coopandreas-dev` @ `2026-03-30` nightly validation build.
- Capture artifacts:
  - `logs/net/NET-W1-2026-03-30-A-host.log`
  - `logs/net/NET-W1-2026-03-30-A-clientA.log`
  - `logs/net/NET-W1-2026-03-30-A-clientB-latejoin.log`
  - `captures/NET-W1-2026-03-30-A/`

## Cleaning The Hood parity scenarios

| Scenario ID | Parity gate | Observed outcome | Evidence refs | Result |
| --- | --- | --- | --- | --- |
| CTH-START-01 | Start/cutscene sync | Host mission launch and opening cutscene transition replicated to both remote peers within the same mission tick window; no double-start event observed. | `NET-W1-2026-03-30-A` host/client logs (`mission_start`, `cutscene_begin`) + clip `cth-start-sync.mp4`. | `pass` |
| CTH-OBJ-01 | Objective propagation | Stage objectives (`reach crack house` → `clear crack dealers`) propagated to peers in-order with matching objective hash and HUD help text. | Host/client objective trace + clip `cth-objective-propagation.mp4`. | `pass` |
| CTH-TERM-FAIL-01 | Fail/pass parity (fail path) | Forced player death on host produced a single authoritative fail broadcast; clients entered fail state once with no repeated mission-end opcodes. | Host fail trace + both client mission terminal logs. | `pass` |
| CTH-TERM-PASS-01 | Fail/pass parity (pass path) | Successful completion emitted one pass terminal payload; reward and mission-complete UI arrived once per peer with aligned mission timestamp. | Host terminal event trace + reward sync logs + clip `cth-pass-terminal.mp4`. | `pass` |
| CTH-RECON-01 | Reconnect restore | Mid-mission disconnect/reconnect restored current stage + objective + combat-phase flags for reconnecting client without mission restart. | Reconnect client log (`snapshot_apply`, `mission_stage_restore`) + clip `cth-reconnect-restore.mp4`. | `pass` |
| CTH-LATE-01 | Late-join hydration | Late-joining peer hydrated into active mission stage with correct objective and actor-state snapshot; no replayed start/cutscene events. | Late-join client log (`late_join_snapshot`, `mission_stage`) + clip `cth-late-join-hydration.mp4`. | `pass` |

## Drive-Thru parity scenarios

| Scenario ID | Parity gate | Observed outcome | Evidence refs | Result |
| --- | --- | --- | --- | --- |
| DTH-START-01 | Start/cutscene sync | Mission start and pre-drive setup sequence synchronized from host to remote peers with one authoritative mission-start packet. | `NET-W1-2026-03-30-A` mission-start traces + clip `dth-start-sync.mp4`. | `pass` |
| DTH-OBJ-01 | Objective propagation | Objective transitions (`board vehicle` → `follow target route` → `eliminate Ballas`) propagated in deterministic order to all peers. | Objective trace lines in host/client logs + clip `dth-objective-propagation.mp4`. | `pass` |
| DTH-TERM-FAIL-01 | Fail/pass parity (fail path) | Vehicle destruction fail condition produced one fail terminal event and synchronized rollback to post-fail state for all peers. | Host/client terminal traces + clip `dth-fail-terminal.mp4`. | `pass` |
| DTH-TERM-PASS-01 | Fail/pass parity (pass path) | Mission completion emitted a single pass event with identical completion tick and no duplicate reward payout replication. | Completion + reward logs + clip `dth-pass-terminal.mp4`. | `pass` |
| DTH-RECON-01 | Reconnect restore | Reconnecting client restored active drive-by stage, target convoy metadata, and objective pointer without forcing mission restart. | Reconnect snapshot logs + clip `dth-reconnect-restore.mp4`. | `pass` |
| DTH-LATE-01 | Late-join hydration | Late-join client hydrated into in-progress drive-by phase with objective state and vehicle roster without re-triggering opening sequence. | Late-join hydration logs + clip `dth-late-join-hydration.mp4`. | `pass` |
