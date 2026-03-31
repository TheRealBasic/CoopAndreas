# Storyline Mission Sign-off Checklist

Use this checklist to sign off a storyline mission consistently before changing roadmap status from `in progress` to `done`.

## Required gates (all must pass)

- [ ] Objective parity
- [ ] Fail/pass parity
- [ ] Reconnect parity
- [ ] Late-join parity
- [ ] Host-migration stable replay
- [ ] Evidence link
- [ ] Functional sign-off
- [ ] Stage-by-stage functional sign-off (required for multi-stage finals)
- [ ] Stage terminal fail/pass idempotency evidence (required for multi-stage finals)

## Sign-off record template

| Field | Value |
| --- | --- |
| Mission |  |
| Script family |  |
| Roadmap status change | `in progress` -> `done` |
| Completion date (`YYYY-MM-DD`) |  |
| Evidence link |  |

## Usage rule

When updating `docs/ROADMAP.md` from `in progress` to `done`, include:

1. A reference to this checklist (`docs/qa/storyline-mission-signoff-checklist.md`).
2. The completion date in `YYYY-MM-DD` format.
3. The mission-specific evidence link proving all required gates passed.
4. For multi-stage final missions, evidence that each stage has independent functional + terminal-event sign-off before the aggregated storyline row is marked `done`.


## Drive-By sign-off record

### Required gates (Drive-By)

- [x] Objective parity
- [x] Fail/pass parity
- [x] Reconnect parity
- [x] Late-join parity
- [x] Host-migration stable replay
- [x] Evidence link
- [x] Functional sign-off
- [x] Stage-by-stage functional sign-off (required for multi-stage finals)
- [x] Stage terminal fail/pass idempotency evidence (required for multi-stage finals)

| Field | Value |
| --- | --- |
| Mission | Drive-By |
| Script family | SWEET (`scm/scripts/SWEET.txt`) |
| Roadmap status change | `in progress` -> `done` |
| Completion date (`YYYY-MM-DD`) | 2026-03-31 |
| Evidence link | `docs/qa/storyline-wave-mission-evidence.md` (W1 `Drive-By` row), `docs/qa/storyline-shared-command-mini-tickets.md#drive-by-scmscriptssweettxt`, `artifacts/qa/storyline/drive-by/NET-W1-2026-03-31-C/` |

Mission note: reused shared hardening fixes `WB-FIX-003` (terminal once-only adjudication) and `WB-FIX-004` (reconnect/late-join hydration epoch filter); evidence artifacts are stored under `artifacts/qa/storyline/drive-by/NET-W1-2026-03-31-C/`.

## Wave closure sign-off records (2026-03-31)

### Required gates (W2/W3/WC promotion batch)

- [x] Objective parity
- [x] Fail/pass parity
- [x] Reconnect parity
- [x] Late-join parity
- [x] Host-migration stable replay
- [x] Evidence link
- [x] Functional sign-off
- [x] Stage-by-stage functional sign-off (required for multi-stage finals)
- [x] Stage terminal fail/pass idempotency evidence (required for multi-stage finals)

| Mission | Script family | Roadmap status change | Completion date (`YYYY-MM-DD`) | Evidence link |
| --- | --- | --- | --- | --- |
| Home Invasion | RYDER (`scm/scripts/RYDER.txt`) | `in progress` -> `done` | 2026-03-31 | `docs/qa/storyline-wave-mission-evidence.md` (W2 row, `NET-W2-2026-03-31-D`) |
| Catalyst | RYDER (`scm/scripts/RYDER.txt`) | `in progress` -> `done` | 2026-03-31 | `docs/qa/storyline-wave-mission-evidence.md` (W2 row, `NET-W2-2026-03-31-E`) |
| Robbing Uncle Sam | RYDER (`scm/scripts/RYDER.txt`) | `in progress` -> `done` | 2026-03-31 | `docs/qa/storyline-wave-mission-evidence.md` (W2 row, `NET-W2-2026-03-31-F`) |
| OG Loc | SMOKE (`scm/scripts/SMOKE.txt`) | `in progress` -> `done` | 2026-03-31 | `docs/qa/storyline-wave-mission-evidence.md` (W3 row, `NET-W3-2026-03-31-A`) |
| Wrong Side Of The Tracks | SMOKE (`scm/scripts/SMOKE.txt`) | `in progress` -> `done` | 2026-03-31 | `docs/qa/storyline-wave-mission-evidence.md` (W3 row, `NET-W3-2026-03-31-B`) |
| Burning Desire | CRASH (`scm/scripts/CRASH.txt`) | `in progress` -> `done` | 2026-03-31 | `docs/qa/storyline-wave-mission-evidence.md` (WC row, `NET-WC-2026-03-31-A`) |
| Gray Imports | CRASH (`scm/scripts/CRASH.txt`) | `in progress` -> `done` | 2026-03-31 | `docs/qa/storyline-wave-mission-evidence.md` (WC row, `NET-WC-2026-03-31-B`) |
