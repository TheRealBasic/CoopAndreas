# Mission parity opcode/task tracing note

This note links the opcode/task sync helpers to a quick debugging workflow for mission parity regressions.

## Where to start

- `COpCodeSync::ShouldDeferNetworkOpcodeExecution(...)`
  - Wraps the pre-execution mission gate (`CMissionSyncState::HandleOpCodePreExecute`).
  - Use this when an incoming opcode appears to stall or get deferred before execution.
- `COpCodeSync::ShouldSkipNetworkWidescreenOpcode(...)`
  - Wraps the `OP_SWITCH_WIDESCREEN` network special case for non-host peers.
  - Use this when widescreen changes seem to be swallowed or replayed unexpectedly.

## Task sequence map

- `CTaskSequenceSync::m_syncedTasks[]`
  - Grouped by task category (movement, orientation/look/aim, animation, vehicle exit, cleanup).
  - Use these group labels to verify whether a missing behavior belongs in sequence sync or in standalone opcode sync.

## Suggested parity workflow

1. **Confirm pre-execute gate behavior**
   - Check whether `ShouldDeferNetworkOpcodeExecution` returns true for the opcode under test.
2. **Check widescreen branch (if opcode is `OP_SWITCH_WIDESCREEN`)**
   - Validate host/non-host role and `scriptParamCount` assumptions in `ShouldSkipNetworkWidescreenOpcode`.
3. **Trace task opcodes through sequence sync**
   - Locate the opcode in `m_syncedTasks[]` by category.
   - If present, follow `CTaskSequenceSync::AddNewTask` serialization path.
   - If absent, validate whether it should remain only in `syncedOpcodes[]` (regular opcode replication).
4. **Compare both pathways**
   - Mission flow bugs often come from choosing the wrong transport path (task-sequence packet vs. generic opcode packet).

Keeping these checkpoints aligned makes it faster to isolate mission/script parity mismatches during co-op debugging.
