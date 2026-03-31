# Storyline command conformance matrix

This matrix is the command-level release gate for storyline multiplayer parity.

## Release gate policy

- Do **not** mark any broad storyline mission wave as functional (`in progress` -> `complete`) until every row below is `complete` or has an approved, time-bound waiver with owner + follow-up ticket.
- A command row is `complete` only when authority, replay, idempotency, and unresolved-entity behavior are all verified in host/peer/reconnect/late-join evidence.
- This gate complements (not replaces) the mission wave checklist in `docs/qa/storyline-wave-mission-evidence.md` and opcode audit ledger in `docs/qa/storyline-opcode-backlog.md`.

## Status rubric

- `not implemented`: no authoritative sync path for required semantics.
- `partial`: command has a sync path, but one or more required semantics are missing/unverified.
- `complete`: all required semantics implemented and validated.

## Critical command matrix

| Command | Authority semantics (required) | Replay semantics (required) | Idempotency semantics (required) | Unresolved-entity behavior (required) | Code owner modules | Status | Notes / current assessment |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `create_actor` | Host-only actor spawn + canonical network identity assignment. | Spawn state rehydrates on reconnect/late-join without intro replay. | Replayed spawn does not duplicate actors or ownership handoff. | If actor handle/net id not yet resolvable, queue/defer and resolve via mission entity registry before hard-fail. | `client/src/COpCodeSync.cpp` (entity registry + deferred opcode queue), `client/src/CMissionSyncState.cpp` (mission epoch/replay metadata), `server/src/CMissionRuntimeManager.cpp` (authoritative runtime snapshots). | `complete` | Authority/idempotency validated by `NAA-01..NAA-05`, `RDG-01..RDG-05`, and CRASH stream seed run `GI-01..GI-05`; replay validated by `NAA-06..NAA-07`, `RDG-06..RDG-07`, and CRASH reconnect/late-join scaffolds `GI-06..GI-07`; unresolved-entity behavior validated by mission `actor registry hookup` deferred-resolution ledgers (including `Gray Imports`) + reconnect/late-join gates in `docs/qa/storyline-shared-command-mini-tickets.md`. |
| `create_car` | Host-only vehicle spawn + canonical network identity assignment. | Vehicle state (spawn, occupants, health) rehydrates for reconnect/late-join peers. | Replay does not duplicate vehicles, seats, or assignment side effects. | Missing vehicle mappings defer/retry until network entity resolution or timeout telemetry. | `client/src/COpCodeSync.cpp` (vehicle param remap/deferral), `client/src/CMissionSyncState.cpp` (authority epoch), `server/src/CMissionRuntimeManager.cpp` (snapshot/replay distribution). | `complete` | Authority/idempotency validated by `DBY-01..DBY-05` and `JBU-01..JBU-06`; replay validated by `DBY-06..DBY-07` and `JBU-07`; unresolved mapping/deferral coverage documented in `Drive-By` pipeline `actor registry hookup` + reconnect/late-join evidence links in `docs/qa/storyline-shared-command-mini-tickets.md` (no new RYDER/CRASH command-level deltas identified for `create_car` in current mission set). |
| `task_drive_by` | Host-authoritative task invocation/order for shooter + target mapping. | Active drive-by task state is replayable for reconnect/late-join hydration. | Re-applied task packets must not create duplicate terminal effects or divergent task graphs. | If target ped/vehicle unresolved, defer packet until mapping resolves; drop with telemetry only after timeout/capacity limits. | `client/src/COpCodeSync.cpp` (opcode `0x0713` sync + deferral), `client/src/CTaskSequenceSync.cpp` (mission-critical task sequence sync), `server/src/CMissionRuntimeManager.cpp` (runtime snapshot/replay backbone). | `complete` | Authority/idempotency validated by `DBY-03..DBY-05` and `JBU-03..JBU-06`; replay validated by `DBY-06..DBY-07` and `JBU-07`; unresolved target deferral covered by `Drive-By`/`Just Business` registry hookup ledgers and hydrate evidence in `docs/qa/storyline-shared-command-mini-tickets.md` (W2/WC mission set does not introduce additional `task_drive_by` command delta). |
| `set_char_obj_kill_char_any_means` | Host decides objective target and kill progression state transitions. | Kill-objective stage replay restores correct in-progress/complete state for reconnect/late-join. | Duplicate kill packets/events do not double-advance stage or rewards. | If killer/target entity unresolved, defer and resolve through registry; no permanent desync from early arrival packets. | `client/src/COpCodeSync.cpp` (kill-task opcode remap/deferral), `client/src/CTaskSequenceSync.cpp` (objective combat task sync), `client/src/CMissionSyncState.cpp` + `server/src/CMissionRuntimeManager.cpp` (mission flow + replay guards). | `complete` | Authority/idempotency validated by `NAA-03..NAA-05`, `RDG-03..RDG-05`, `JBU-04..JBU-06`, and RYDER stream kill-objective seed run `RUS-04..RUS-05`; replay validated by `NAA-06..NAA-07`, `RDG-06..RDG-07`, `JBU-07`, plus RYDER/CRASH reconnect+late-join scaffolds (`RUS-06..RUS-07`, `GI-06..GI-07`); unresolved-entity behavior covered by mission deferred-resolution ledger notes + reconnect/late-join evidence links in `docs/qa/storyline-shared-command-mini-tickets.md`. |
| `set_objective` | Host-authoritative objective text/phase monotonic progression (`objectivePhaseIndex`). | Objective state + text semantics replay from authoritative runtime snapshot to reconnect/late-join peers. | Replays and repeated deltas do not regress phase index or duplicate pass/fail side effects. | N/A for direct entity handles; if upstream entity-gated command is unresolved, objective transition must wait until authoritative progression is valid. | `shared/ObjectiveSyncState.h` (objective state transitions), `client/src/CMissionSyncState.cpp` (mission flow emit/apply + inbound accept guards), `server/src/CMissionRuntimeManager.cpp` (objective versioning + snapshot/replay). | `complete` | Objective monotonic/versioned state and replay paths are implemented as a first-class mission runtime surface; no open command-level gap called out in current docs. |

## Wave promotion checklist tie-in

Before changing any storyline wave to broadly functional/complete:

- Confirm every command row above is `complete` **or** has an explicit waiver.
- Link supporting evidence artifacts (session ids/logs/clips) for each semantics column.
- Re-run `python3 tools/opcode_audit.py --output docs/qa/storyline-opcode-backlog.md` and update the wave audit ledger.
- Update `docs/qa/storyline-wave-mission-evidence.md` with mission-level proof that command semantics hold under start/fail/pass/reconnect/late-join gates.

## CRASH/RYDER command evidence addendum (2026-03-31)

The following mission artifacts were linked to close CRASH/RYDER-era verification gaps for shared commands without reopening command status:

- `Gray Imports` (`GI-01..GI-07`, session `NET-WC-TBD-GI`) now anchors CRASH-stream `create_actor` and kill-objective replay/deferred-resolution semantics.
- `Robbing Uncle Sam` (`RUS-04..RUS-07`, session `NET-W2-2026-03-31-C`) now anchors RYDER-stream kill-objective authority/idempotency plus reconnect/late-join replay scaffolding.
- No new command-level deltas were discovered for `create_car` or `task_drive_by` in active RYDER/CRASH mission set; existing `Drive-By`/`Just Business` evidence remains canonical for those rows.

## Wave promotion readiness after command-row closure

- **Command gate:** `complete` for all rows in this matrix.
- **Wave gate:** still **not promotable** for W2 (RYDER) or WC (CRASH) as of 2026-03-31, because mission-level six-gate parity (`start/objective/fail/pass/reconnect/late-join`) remains `in progress` in `docs/qa/storyline-wave-mission-evidence.md`.
- Promotion may proceed only after per-mission sign-off checklists are complete and evidence links are upgraded from scaffold traces to full pass artifacts.
