# QA Traceability Tags

Use a lightweight tag so roadmap items are easy to connect to implementation and validation evidence.

## Tag format

- Prefix: `PARITY-ID:`
- Value: uppercase kebab-case `<AREA>-<ITEM>`
- Example: `PARITY-ID: STADIUM-8-TRACK`

## Where to use it

- PR title/body.
- Commit messages.
- Optional code comments near the replicated mission/sync logic.

## Index generation

Run:

```bash
python3 tools/qa_trace.py --output docs/qa/roadmap-trace-index.json --check-complete
```

This generates a machine-readable index that links `docs/ROADMAP.md` progress rows to:

- QA evidence documents (phase checklist/matrix files)
- key module references
- recent commits tagged with matching `PARITY-ID`

The `--check-complete` gate fails if any roadmap row with status `done` is missing evidence docs or module references.
