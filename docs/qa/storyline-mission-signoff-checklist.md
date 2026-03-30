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
