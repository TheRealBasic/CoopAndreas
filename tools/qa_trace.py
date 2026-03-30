#!/usr/bin/env python3
"""Generate roadmap-to-QA traceability index and validate completeness gates."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List

ROOT = Path(__file__).resolve().parent.parent
ROADMAP = ROOT / "docs/ROADMAP.md"
OUT_JSON = ROOT / "docs/qa/roadmap-trace-index.json"

PHASE_MODULE_HINTS: Dict[str, List[str]] = {
    "Schools": [
        "client/src/CMissionSyncState.cpp",
        "client/src/COpCodeSync.cpp",
        "server/src/CPlayerManager.h",
    ],
    "Stadium events": [
        "client/src/CMissionSyncState.cpp",
        "client/src/COpCodeSync.cpp",
        "server/src/CPlayerManager.h",
    ],
    "Hidden races": [
        "client/src/CMissionSyncState.cpp",
        "client/src/COpCodeSync.cpp",
    ],
    "Courier routes": [
        "client/src/CMissionSyncState.cpp",
        "client/src/COpCodeSync.cpp",
    ],
    "Street races": [
        "client/src/CMissionSyncState.cpp",
        "client/src/COpCodeSync.cpp",
    ],
    "Import/Export": [
        "client/src/CMissionSyncState.cpp",
        "client/src/COpCodeSync.cpp",
    ],
}

PHASE_TAG_PREFIX = {
    "Schools": "SCHOOLS",
    "Stadium events": "STADIUM",
    "Hidden races": "HIDDEN",
    "Courier routes": "COURIER",
    "Street races": "STREET",
    "Import/Export": "IMPORTEXPORT",
}


@dataclass
class ModeStatus:
    phase: str
    mode: str
    status: str


def _slug(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9]+", "-", value.strip().upper())
    return cleaned.strip("-")


def _read_roadmap() -> str:
    return ROADMAP.read_text(encoding="utf-8")


def _extract_phase_to_checklist(text: str) -> Dict[str, str]:
    mapping: Dict[str, str] = {}
    in_table = False
    for line in text.splitlines():
        if line.startswith("| Phase | Modes in scope |"):
            in_table = True
            continue
        if not in_table:
            continue
        if not line.startswith("|"):
            break
        if line.startswith("| ---"):
            continue
        parts = [p.strip() for p in line.strip().strip("|").split("|")]
        if len(parts) < 5:
            continue
        phase = parts[0]
        checklist = parts[4].strip("`")
        if checklist.startswith("docs/qa/"):
            mapping[phase] = checklist
    return mapping


def _extract_mode_statuses(text: str) -> List[ModeStatus]:
    rows: List[ModeStatus] = []
    in_table = False
    for line in text.splitlines():
        if line.startswith("| Phase | Mode | Status |"):
            in_table = True
            continue
        if not in_table:
            continue
        if not line.startswith("|"):
            break
        if line.startswith("| ---"):
            continue
        parts = [p.strip() for p in line.strip().strip("|").split("|")]
        if len(parts) != 3:
            continue
        rows.append(ModeStatus(phase=parts[0], mode=parts[1], status=parts[2].lower()))
    return rows


def _git_recent_commits(parity_id: str, limit: int) -> List[dict]:
    fmt = "%H%x1f%cs%x1f%s"
    cmd = [
        "git",
        "log",
        f"--grep=PARITY-ID:\\s*{re.escape(parity_id)}",
        "--regexp-ignore-case",
        "--date=short",
        f"--pretty=format:{fmt}",
        f"--max-count={limit}",
    ]
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, check=False)
    commits: List[dict] = []
    if proc.returncode != 0:
        return commits
    for line in proc.stdout.splitlines():
        if not line.strip():
            continue
        commit, cdate, subject = line.split("\x1f", 2)
        files_cmd = ["git", "show", "--pretty=", "--name-only", commit]
        files_proc = subprocess.run(files_cmd, cwd=ROOT, capture_output=True, text=True, check=False)
        files = sorted({f.strip() for f in files_proc.stdout.splitlines() if f.strip()})
        commits.append(
            {
                "sha": commit,
                "date": cdate,
                "subject": subject,
                "files": files,
            }
        )
    return commits


def build_index(limit: int) -> dict:
    text = _read_roadmap()
    phase_docs = _extract_phase_to_checklist(text)
    mode_rows = _extract_mode_statuses(text)

    items = []
    for row in mode_rows:
        prefix = PHASE_TAG_PREFIX.get(row.phase, _slug(row.phase))
        parity_id = f"{prefix}-{_slug(row.mode)}"
        evidence = [phase_docs[row.phase]] if row.phase in phase_docs else []
        module_refs = PHASE_MODULE_HINTS.get(row.phase, []).copy()
        commits = _git_recent_commits(parity_id, limit=limit)
        touched_from_commits = sorted({f for c in commits for f in c.get("files", []) if f})
        if touched_from_commits:
            module_refs = sorted(set(module_refs + touched_from_commits))
        items.append(
            {
                "parity_id": parity_id,
                "roadmap": {
                    "phase": row.phase,
                    "mode": row.mode,
                    "status": row.status,
                    "source": "docs/ROADMAP.md#progress-tracking-per-mode",
                },
                "evidence_docs": evidence,
                "module_refs": module_refs,
                "recent_commits": commits,
            }
        )

    return {
        "schema_version": 1,
        "tag_format": {
            "prefix": "PARITY-ID:",
            "pattern": "PARITY-ID: <AREA>-<ITEM>",
            "example": "PARITY-ID: STADIUM-8-TRACK",
        },
        "generated_from": str(ROADMAP.relative_to(ROOT)),
        "items": items,
    }


def validate(index: dict) -> List[str]:
    errors: List[str] = []
    for item in index.get("items", []):
        status = item.get("roadmap", {}).get("status", "")
        if status != "done":
            continue
        if not item.get("evidence_docs"):
            errors.append(f"{item['parity_id']}: done but no evidence_docs")
        if not item.get("module_refs"):
            errors.append(f"{item['parity_id']}: done but no module_refs")
    return errors


def parse_args() -> argparse.Namespace:
    ap = argparse.ArgumentParser(description="Generate QA traceability index from roadmap.")
    ap.add_argument("--output", type=Path, default=OUT_JSON)
    ap.add_argument("--commit-limit", type=int, default=5)
    ap.add_argument("--check-complete", action="store_true")
    return ap.parse_args()


def main() -> int:
    args = parse_args()
    out = args.output if args.output.is_absolute() else (ROOT / args.output)
    index = build_index(limit=args.commit_limit)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {out}")

    if args.check_complete:
        errs = validate(index)
        if errs:
            print("ERROR: completed roadmap items missing traceability links:", file=sys.stderr)
            for err in errs:
                print(f"- {err}", file=sys.stderr)
            return 1
        print("Traceability check passed for all completed roadmap items.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
