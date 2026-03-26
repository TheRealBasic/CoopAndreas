# Animation Sync QA Matrix (2 Peers)

## Scope
- Verify idle animation loop parity and release parity between host + one client peer.
- Verify special `TAB+NUM4` / `TAB+NUM6` trigger parity.
- Verify sequence playback does not fail when anim-task targets are unresolved during network task-sequence processing.

## Setup
1. Launch one host and one non-host client in the same session.
2. Put both players in the same area and in clear line of sight.
3. Enable network debug logs for opcode/task-sequence output (if available).
4. Use the same ped model class on both peers to minimize animation dictionary differences.

## Matrix

| ID | Scenario | Initiator | Steps | Expected on remote peer | Expected on initiator |
|---|---|---|---|---|---|
| A1 | Idle loop enter | Host | Start an idle loop animation (non-cutscene) and keep it running for \>= 5s. | Remote sees same idle anim enter quickly and remain looped. | No duplicate/replayed self-trigger from host relay. |
| A2 | Idle loop exit | Host | Cancel or interrupt the idle loop via normal gameplay/task change. | Remote exits idle anim in parity window (\<= 500ms drift). | Local state remains authoritative and stable. |
| A3 | Idle loop enter | Client | Start same idle loop from non-host peer. | Host sees matching loop start with correct ped target. | Client keeps ownership behavior without immediate rollback. |
| A4 | Special trigger `TAB+NUM4` | Host | Trigger animation bound to `TAB+NUM4` three times with short spacing. | Remote sees each trigger once (no drops, no doubles). | Host does not replay its own authoritative trigger from broadcast. |
| A5 | Special trigger `TAB+NUM6` | Client | Trigger animation bound to `TAB+NUM6` three times with short spacing. | Host sees each trigger once, with proper anim dictionary resolution. | Client sees no duplicate local restart from echo packet. |
| A6 | Sequence anim + missing target | Host | Run a task sequence containing an anim task where target ped becomes unresolved/destroyed before replay. | Remote sequence continues without global `failed` latch; unresolved anim step is skipped safely. | Host remains stable; no repeated sequence resend storm. |
| A7 | Sequence anim dictionary miss | Client | Run sequence anim using an anim dict not loaded yet on remote. | Remote queues/deferred-load behavior works; sequence only fails when target is valid and loading is required. | Initiator remains in sync after deferred replay. |

## Pass Criteria
- No hard desync caused by `task_play_anim*` variants.
- No host self-replay for authoritative local anim triggers.
- `TAB+NUM4` / `TAB+NUM6` visual parity across both peers.
- Task-sequence processing does not set a sticky failure flag solely because an anim target is unresolved.
