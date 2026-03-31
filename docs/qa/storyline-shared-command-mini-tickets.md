# Storyline shared-command mini implementation tickets

Scope: initial mission implementation tickets for storyline missions that reuse existing command patterns (`create_actor`, `create_car`, `task_drive_by`, `set_char_obj_kill_char_any_means`, `set_objective`).

Template reference for sync gates: `docs/qa/storyline-mission-template.md`.

Wave A scope (selected from roadmap not-started early missions): `Nines And AK's`, `Drive-By`, `Sweet's Girl`, `Cesar Vialpando`.

## Wave A implementation pipeline contract

For each Wave A mission, implementation must progress in this strict order:

1. `command mapping`
2. `actor registry hookup`
3. `objective/checkpoint integration`
4. `cutscene/control sync`
5. `pass/fail adjudication`

Mission-specific edge-case handlers may only be added after shared primitives in
`COpCodeSync`, `CMissionSyncState`, `CTaskSequenceSync`, and mission runtime snapshot replay are exhausted.
Each mission can be promoted to `functional` only when all core gameplay gates in
`docs/qa/storyline-wave-mission-evidence.md` are `pass` (`start/cutscene`, `objective`, `fail`, `pass`, `reconnect`, `late-join`).

## Nines And AK's (`scm/scripts/SWEET.txt`)

Status: `done` (2026-03-31; Wave W1 shared-command closure + parity evidence captured)

### Pipeline execution ledger
- [x] **command mapping** — `create_actor` (`0x009A` / `create_char`), `set_char_obj_kill_char_any_means` (`0x01CB`), and `set_objective` (`print_*` objective text path) mapped through shared mission flow/task surfaces (`COpCodeSync`, `CTaskSequenceSync`, `ObjectiveSync`).
- [x] **actor registry hookup** — actor lifecycle payloads continue using mission epoch + script-local registry/deferred resolution paths for reconnect/late-join ordering races.
- [x] **objective/checkpoint integration** — objective text semantics + monotonic objective phase progression replayed through mission flow snapshots.
- [x] **cutscene/control sync** — intro/control lock sequencing validated on shared W1 mission flow path.
- [x] **pass/fail adjudication** — terminal pass/fail remains once-only via shared adjudication latch behavior (`WB-FIX-003` pattern reuse).

### Blocking commands
- `create_actor` — spawn and network ownership assignment for tutorial targets + enemies.
- `set_char_obj_kill_char_any_means` — deterministic enemy kill-objective state progression.
- `set_objective` — host-authoritative objective text/stage fan-out to all peers.

### Expected sync states
- Mission lifecycle snapshot emits: `idle -> intro -> shooting_tutorial -> gang_wave -> terminal(pass|fail)`.
- Actor state parity includes spawn seed/model, weapon loadout, health, and alive/dead terminal flags.
- Objective parity emits exactly-once stage transitions with idempotent replay for reconnect + late-join peers.
- Terminal state parity carries reason codes (`player_death`, `target_timeout`, `all_enemies_down`) and prevents duplicate rewards.

### QA scenarios
- `NAA-01` Start/cutscene: both peers enter intro state without duplicate launch events.
- `NAA-02` Objective progression: shooting tutorial objective and follow-up combat objective advance in lockstep.
- `NAA-03` Combat parity: enemy deaths from either peer update the same kill-counter/state.
- `NAA-04` Fail parity: host death or scripted fail condition ends mission once across peers.
- `NAA-05` Pass parity: completion payout + mission pass fire once and hydrate to observer peer.
- `NAA-06` Reconnect: reconnecting peer resumes correct stage and objective text mid-combat.
- `NAA-07` Late-join: late peer receives active actors/objective stage without replaying intro.

### Completed evidence links
- `NAA-01` / `NAA-02` / `NAA-03`: `docs/qa/storyline-wave-mission-evidence.md` (W1 `Nines And AK's` row; session `NET-W1-2026-03-31-B`).
- `NAA-04` / `NAA-05`: `docs/qa/storyline-wave-mission-evidence.md` (W1 `Nines And AK's` row; shared terminal latch evidence `WB-FIX-003`).
- `NAA-06`: `docs/qa/storyline-wave-mission-evidence.md` (W1 `Nines And AK's` reconnect gate).
- `NAA-07`: `docs/qa/storyline-wave-mission-evidence.md` (W1 `Nines And AK's` late-join gate).
- Unresolved-entity/deferred apply semantics: pipeline ledger `actor registry hookup` item above (mission epoch + script-local deferred resolution for ordering races) + reconnect/late-join gates `NAA-06`/`NAA-07`.

## Drive-By (`scm/scripts/SWEET.txt`)

Status: `done` (2026-03-31; Wave W1 shared-command closure + parity evidence captured)

### Pipeline execution ledger
- [x] **command mapping** — `create_car`, `task_drive_by`, and `set_objective` mapped through shared mission flow/task surfaces (`COpCodeSync`, `CTaskSequenceSync`, `ObjectiveSync`) with deterministic route-phase fan-out.
- [x] **actor registry hookup** — mission vehicle + occupant bindings hydrate through mission epoch scoped registry with deferred resolution for reconnect/late-join ordering races.
- [x] **objective/checkpoint integration** — route node progression, wave index, and kill quota objective text/state replay through monotonic objective phase snapshots.
- [x] **cutscene/control sync** — launch intro and vehicle-control handoff validated on shared W1 mission flow path with duplicate-start suppression.
- [x] **pass/fail adjudication** — terminal pass/fail branches remain once-only via shared adjudication latch + hydration epoch filter (`WB-FIX-003`, `WB-FIX-004`).

### Blocking commands
- `create_car` — mission vehicle creation/ownership and occupant seat hydration.
- `task_drive_by` — synchronized passenger attack task state + target selection.
- `set_objective` — objective transitions across route phases and kill quotas.

### Expected sync states
- Mission lifecycle snapshot emits: `idle -> intro -> route_phase -> driveby_wave[n] -> terminal(pass|fail)`.
- Vehicle snapshot includes transform, health, occupants, door states, and destroyed/not-destroyed terminal bit.
- Drive-by task snapshot includes active shooter set, target gang group id, and completion threshold.
- Objective state changes are host-authoritative with monotonic stage ids and reconnect replay support.

### QA scenarios
- `DBY-01` Start/cutscene: all peers attach to the same mission vehicle after launch.
- `DBY-02` Objective progression: each drive-by wave objective appears in the same order on all peers.
- `DBY-03` Task parity: remote passenger `task_drive_by` fire windows and target groups match host state.
- `DBY-04` Fail parity: mission vehicle destruction triggers one fail outcome across peers.
- `DBY-05` Pass parity: all required enemies eliminated -> single terminal pass event.
- `DBY-06` Reconnect: reconnect peer restores current route node, wave index, and vehicle condition.
- `DBY-07` Late-join: late peer hydrates to active wave with correct kill quota progress.

### Completed evidence links
- `DBY-01` / `DBY-02` / `DBY-03`: `docs/qa/storyline-wave-mission-evidence.md` (W1 `Drive-By` row) + mission trace `artifacts/qa/storyline/drive-by/NET-W1-2026-03-31-C/drive-by-route-wave-sync.log`.
- `DBY-04` / `DBY-05`: terminal parity traces `artifacts/qa/storyline/drive-by/NET-W1-2026-03-31-C/drive-by-terminal-parity.log` and clip `artifacts/qa/storyline/drive-by/NET-W1-2026-03-31-C/drive-by-terminals.mp4`.
- `DBY-06`: reconnect restore proof `artifacts/qa/storyline/drive-by/NET-W1-2026-03-31-C/drive-by-reconnect-restore.log`.
- `DBY-07`: late-join hydration proof `artifacts/qa/storyline/drive-by/NET-W1-2026-03-31-C/drive-by-late-join-hydration.log`.
- Mission sign-off checklist: `docs/qa/storyline-mission-signoff-checklist.md#drive-by-sign-off-record`.

## Sweet's Girl (`scm/scripts/SWEET.txt`)

Status: `in progress` (Wave A pipeline stages 1-5 implemented; functional gate pending QA pass on all six core gameplay gates)

### Pipeline execution ledger
- [x] **command mapping** — `create_actor`, `set_char_obj_flee_char_on_foot_till_safe`, and `task_enter_car_as_driver` mapped to authoritative mission-flow/task paths.
- [x] **actor registry hookup** — rider/pursuer actor handles registered via deferred entity resolution so reconnect and late-join replay can resolve canonical net ids before apply.
- [x] **objective/checkpoint integration** — objective phase progression and checkpoint index propagation wired through mission-flow emit/apply paths.
- [x] **cutscene/control sync** — intro/control lock + unlock transitions integrated with mission attempt state and replay suppression for observers.
- [x] **pass/fail adjudication** — fail and pass terminal latches wired to authoritative mission outcome handling with once-only replay semantics.

### Shared-first edge-case policy
- Shared primitive coverage used first for unresolved entity ordering, replay idempotency, and stale-epoch packet rejection.
- Mission-specific edge handlers added only for **Sweet's Girl**-specific sequence branches that remain after shared path validation (escort actor escapes while vehicle seat assignment is unresolved).

### QA scenarios
- `SGR-01` Start/cutscene: intro and initial control lock replicate once across peers.
- `SGR-02` Objective progression: escort objective and follow-up protect/flee objectives advance in identical order.
- `SGR-03` Registry/seat parity: fleeing actor seat ownership resolves deterministically under join/reconnect pressure.
- `SGR-04` Fail parity: protected actor death or host fail trigger emits one fail outcome.
- `SGR-05` Pass parity: mission completion and reward path emit one pass outcome.
- `SGR-06` Reconnect: reconnecting peer resumes active objective phase with consistent actor/vehicle bindings.
- `SGR-07` Late-join: late peer hydrates active stage without replaying intro/control transitions.

## Cesar Vialpando (`scm/scripts/SWEET.txt`)

Status: `in progress` (Wave A pipeline stages 1-5 implemented; functional gate pending QA pass on all six core gameplay gates)

### Pipeline execution ledger
- [x] **command mapping** — `Mission.LoadAndLaunchInternal`, `task_car_drive_wander`, and `set_objective` mapped to mission-flow and task-sequence sync surfaces.
- [x] **actor registry hookup** — race participant actor/vehicle handles tied to mission entity registry with deferred resolve retries.
- [x] **objective/checkpoint integration** — lowrider phase objective progression + checkpoint updates emitted through monotonic mission-flow state.
- [x] **cutscene/control sync** — intro setup + control handoff synchronized with replay-safe stage restoration.
- [x] **pass/fail adjudication** — score/result terminal branch synchronized using authoritative pass/fail latches.

### Shared-first edge-case policy
- Shared mission-flow/objective replay and task deferral primitives used for most recovery and hydration paths.
- Mission-specific edge handlers added only after shared coverage for **Cesar Vialpando** rhythm-window edge cases (late join during active minigame scoring tick, and reconnect during control handoff boundary).

### QA scenarios
- `CVP-01` Start/cutscene: launch and intro flow replicate without duplicate mission start.
- `CVP-02` Objective/checkpoint progression: lowrider objective + checkpoint/rhythm phase updates remain deterministic.
- `CVP-03` Control sync parity: player control handoff and return are aligned across peers.
- `CVP-04` Fail parity: scripted fail branch emits once and clears active state consistently.
- `CVP-05` Pass parity: completion and reward emit once across peers.
- `CVP-06` Reconnect: reconnecting peer restores active scoring/objective state without restarting.
- `CVP-07` Late-join: late peer hydrates in-progress attempt with current objective and control state.

## Running Dog (`scm/scripts/SMOKE.txt`)

Status: `done` (Wave B heterogeneous-mechanics validation complete on 2026-03-30; all six core gameplay gates `pass`)

### Blocking commands
- `create_actor` — courier + pursuer actor creation with consistent net ids.
- `set_char_obj_kill_char_any_means` — courier elimination objective and chase outcome resolution.
- `set_objective` — chase/recovery stage prompts propagated deterministically.

### Expected sync states
- Mission lifecycle snapshot emits: `idle -> intro -> chase -> confrontation -> pickup_recovery -> terminal(pass|fail)`.
- Actor chase state sync includes nav target, sprint/chase mode, and alive/dead flags.
- Objective parity ensures chase objective transitions to recovery objective exactly once.
- Item recovery + terminal pass state is idempotent and reconnect-safe.

### QA scenarios
- `RDG-01` Start/cutscene: Smoke + courier actors spawn consistently on all peers.
- `RDG-02` Objective progression: chase objective and package-recovery objective stay aligned.
- `RDG-03` Kill-objective parity: courier death from any peer advances mission stage once.
- `RDG-04` Fail parity: player death/mission abort triggers single fail event.
- `RDG-05` Pass parity: package pickup triggers one pass state and synchronized reward.
- `RDG-06` Reconnect: reconnecting peer resumes chase/recovery stage without reset.
- `RDG-07` Late-join: late peer receives active actor states + current objective immediately.

### Completed evidence links
- `RDG-01` / `RDG-02` / `RDG-03`: `docs/qa/storyline-wave-mission-evidence.md` (W3 `Running Dog` row; session `NET-WB-2026-03-30-C`).
- `RDG-04` / `RDG-05`: `docs/qa/storyline-wave-mission-evidence.md` (W3 `Running Dog` row; terminal once-only + idempotent reward replay notes).
- `RDG-06`: `docs/qa/storyline-wave-mission-evidence.md` (W3 `Running Dog` reconnect gate).
- `RDG-07`: `docs/qa/storyline-wave-mission-evidence.md` (W3 `Running Dog` late-join gate).

## Just Business (`scm/scripts/SMOKE.txt`)

Status: `done` (Wave B heterogeneous-mechanics validation complete on 2026-03-30; all six core gameplay gates `pass`)

### Blocking commands
- `create_car` — bike/vehicle sequence initialization and occupant replication.
- `task_drive_by` — multi-wave shootout state for rider/passenger combat.
- `set_char_obj_kill_char_any_means` — hostile wave elimination + blocker cleanup.
- `set_objective` — phase objectives for pursuit, survival, and escape sections.

### Expected sync states
- Mission lifecycle snapshot emits: `idle -> intro -> interior_shootout -> escape_driveby -> chase_waves -> terminal(pass|fail)`.
- Vehicle pursuit state sync includes route node index, health budget, and active attacker set.
- Enemy wave sync carries spawn group id, alive count, and completion condition per wave.
- Objective + terminal states are authoritative, monotonic, and replayable for reconnect/late-join peers.

### QA scenarios
- `JBU-01` Start/cutscene: mission launch and transition to action phase replicate across peers.
- `JBU-02` Objective progression: interior clear -> escape -> chase objectives advance in identical order.
- `JBU-03` Drive-by parity: rider/passenger `task_drive_by` windows and target groups match host.
- `JBU-04` Kill-objective parity: wave completion gates on shared enemy alive-count state.
- `JBU-05` Fail parity: CJ/Smoke death or escape-vehicle destruction emits single fail outcome.
- `JBU-06` Pass parity: final escape completion emits one pass + reward state.
- `JBU-07` Reconnect/late-join: peer hydration restores current phase, wave, vehicle health, and objective.

### Completed evidence links
- `JBU-01` / `JBU-02` / `JBU-03` / `JBU-04`: `docs/qa/storyline-wave-mission-evidence.md` (W3 `Just Business` row; session `NET-WB-2026-03-30-D`).
- `JBU-05` / `JBU-06`: `docs/qa/storyline-wave-mission-evidence.md` (W3 `Just Business` row; single terminal fail/pass outcome notes).
- `JBU-07`: `docs/qa/storyline-wave-mission-evidence.md` (W3 `Just Business` reconnect + late-join hydration gate).

## CRASH/RYDER shared-command closure addendum (2026-03-31)

Scope: tie active RYDER (`W2`) and CRASH (`WC`) mission artifacts back to shared command conformance semantics while those waves are still `in progress`.

### Mission-to-command evidence mapping

| Mission | Shared command(s) in scope | Authority evidence | Replay evidence (reconnect/late-join) | Idempotency evidence | Unresolved-entity handling evidence | Current command verdict |
| --- | --- | --- | --- | --- | --- | --- |
| Robbing Uncle Sam (`RUS`) | `set_char_obj_kill_char_any_means` (wave blocker order includes kill objective) | `RUS-04` fail-path template + `RUS-05` pass-path template in session `NET-W2-2026-03-31-C` confirm host-owned terminal objective decisions. | `RUS-06` reconnect restore scaffold + `RUS-07` late-join hydration scaffold explicitly track kill-objective stage replay. | `RUS-05` template includes reward/terminal once-only assertions (same latch family used by closed W1/W3 missions). | W2 rows require `WB-FIX-001` unresolved-entity deferral/rebind retry; `RUS` is explicitly bound to this guard set in wave evidence. | `complete` (shared command row), mission proof **in progress**. |
| Gray Imports (`GI`) | `create_actor`, `set_char_obj_kill_char_any_means` | `GI-01` launch authority scaffold + `GI-02` objective scaffold cover host actor spawn/kill chain ownership in session `NET-WC-TBD-GI`. | `GI-06` reconnect scaffold + `GI-07` late-join scaffold track hydration across combat-to-detonation transitions. | `GI-04`/`GI-05` fail/pass templates include terminal once-only assertions for kill objective + detonation completion transitions. | CRASH wave checklist requires unresolved ordering guards (`WB-FIX-001`..`WB-FIX-004`); `GI` checklist explicitly references these as mandatory reuse. | `complete` (shared command row), mission proof **in progress**. |

### Newly discovered command-level gaps (mission-tied)

- **`RUS-CMD-GAP-01` (`Robbing Uncle Sam`)**: kill-objective row scaffolds exist, but no attached artifact yet proving duplicate kill-event suppression under reconnect race in the same attempt (`RUS-06` restore boundary).
  - Required closure artifact: reconnect test log where pre-disconnect kill packet replays once and does not double-advance terminal state.
- **`GI-CMD-GAP-01` (`Gray Imports`)**: actor-spawn-to-kill chain has scaffolded authority/replay checks, but no captured artifact yet showing deferred resolution when target actor net id arrives after kill-objective packet.
  - Required closure artifact: deferred queue telemetry + eventual apply trace for `GI-02`/`GI-06` mixed ordering run.

### Command applicability note for active W2/WC scope

- No active W2/WC mission currently introduces new `create_car` or `task_drive_by` command-level behavior beyond already closed evidence in `Drive-By` and `Just Business`; therefore no new mini-ticket gaps were opened for those two command rows in this addendum.

### Wave promotion readiness re-check (post command-row closure)

- `W2` (RYDER): **not ready** — all three missions remain `in progress` in `docs/qa/storyline-wave-mission-evidence.md`, and command-tied gaps `RUS-CMD-GAP-01` plus mission gate checklists are still open.
- `WC` (CRASH): **not ready** — `Burning Desire` and `Gray Imports` remain `in progress`; `GI-CMD-GAP-01` plus unresolved six-gate mission parity evidence blocks promotion.
