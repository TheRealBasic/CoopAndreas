# Submissions Sync QA Evidence (Consolidated)

Date completed: **2026-03-26**.

References used for completion criteria:
- `docs/qa/submissions-sync-matrix.md` (completion rule: SS-02..SS-11 per submission type).
- Per-mode checklists:
  - `docs/qa/submission-taxi-sync-checklist.md`
  - `docs/qa/submission-firefighter-sync-checklist.md`
  - `docs/qa/submission-vigilante-sync-checklist.md`
  - `docs/qa/submission-paramedic-sync-checklist.md`
  - `docs/qa/submission-pimp-sync-checklist.md`
  - `docs/qa/submission-freight-train-sync-checklist.md`

## Coverage summary (SS-02..SS-11)

| Submission type | SS-02 lifecycle | SS-03/SS-04 progression+payout parity | SS-05/SS-06 outcomes | Reconnect (SS-07) | Late-join (SS-08) | Host migration (SS-09) | Anti-dup payout (SS-10) | Anti-dup snapshot replay (SS-11) | Remaining failing scenario IDs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Taxi | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | None |
| Firefighter | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | None |
| Vigilante | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | None |
| Paramedic | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | None |
| Pimp | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | None |
| Freight Train | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | None |

## Per-mode required outcomes

### Taxi
- Reconnect: **PASS** (SS-07)
- Late-join: **PASS** (SS-08)
- Host migration: **PASS** (SS-09)
- Anti-duplication payout behavior: **PASS** (SS-10, SS-11)
- Remaining failing scenario IDs: **None**

### Firefighter
- Reconnect: **PASS** (SS-07)
- Late-join: **PASS** (SS-08)
- Host migration: **PASS** (SS-09)
- Anti-duplication payout behavior: **PASS** (SS-10, SS-11)
- Remaining failing scenario IDs: **None**

### Vigilante
- Reconnect: **PASS** (SS-07)
- Late-join: **PASS** (SS-08)
- Host migration: **PASS** (SS-09)
- Anti-duplication payout behavior: **PASS** (SS-10, SS-11)
- Remaining failing scenario IDs: **None**

### Paramedic
- Reconnect: **PASS** (SS-07)
- Late-join: **PASS** (SS-08)
- Host migration: **PASS** (SS-09)
- Anti-duplication payout behavior: **PASS** (SS-10, SS-11)
- Remaining failing scenario IDs: **None**

### Pimp
- Reconnect: **PASS** (SS-07)
- Late-join: **PASS** (SS-08)
- Host migration: **PASS** (SS-09)
- Anti-duplication payout behavior: **PASS** (SS-10, SS-11)
- Remaining failing scenario IDs: **None**

### Freight Train
- Reconnect: **PASS** (SS-07)
- Late-join: **PASS** (SS-08)
- Host migration: **PASS** (SS-09)
- Anti-duplication payout behavior: **PASS** (SS-10, SS-11)
- Remaining failing scenario IDs: **None**

## Completion decision
All submission modes satisfy the matrix completion rule (`SS-02..SS-11` per submission type). The submissions sync framework can be marked complete in the roadmap.
