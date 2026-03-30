#!/usr/bin/env python3
import argparse
import json
import re
import sys
from pathlib import Path
from collections import Counter, defaultdict
from datetime import date

ROOT = Path(__file__).resolve().parent.parent
ROADMAP = ROOT / 'docs/ROADMAP.md'
OPSYNC = ROOT / 'client/src/COpCodeSync.cpp'
SA_JSON = ROOT / 'sdk/Sanny Builder 4/data/sa_sbl_coopandreas/sa_coop.json'
OUT_MD = ROOT / 'docs/qa/storyline-opcode-backlog.md'

SCRIPT_REF_RE = re.compile(r'`(scm/scripts/[A-Z0-9_]+\.txt)`')
NUM_OPCODE_RE = re.compile(r'\b([0-9A-Fa-f]{4}):')
DOT_CALL_RE = re.compile(r'\b([A-Za-z_][A-Za-z0-9_]*)\.([A-Za-z_][A-Za-z0-9_]*)\s*(?:\(|$)')
SYNCED_HEX_RE = re.compile(r'\{\s*0x([0-9A-Fa-f]{4})')
SYNCED_MACRO_RE = re.compile(r'\{\s*COMMAND_([A-Z0-9_]+)')


WAVE_FAMILIES = [
    ('W1', 'SWEET', 'Mission Sync Pod (Sweet stream)'),
    ('W2', 'RYDER', 'Mission Sync Pod (Ryder stream)'),
    ('W3', 'SMOKE', 'Mission Sync Pod (Smoke stream)'),
]


def wave_missing_rows(triaged):
    rows = []
    for wave_id, family, owner in WAVE_FAMILIES:
        family_suffix = f"{family}.txt"
        family_missing = [item for item in triaged if any(f == family_suffix for f in item['files'])]
        new_gaps = ', '.join(f"`0x{item['opcode']:04X}`" for item in family_missing) or 'none'
        rows.append((wave_id, family, owner, len(family_missing), new_gaps))
    return rows


def load_targets():
    text = ROADMAP.read_text(encoding='utf-8')
    targets = sorted(set(SCRIPT_REF_RE.findall(text)))
    return [ROOT / p for p in targets]


def load_opcode_index():
    data = json.loads(SA_JSON.read_text(encoding='utf-8'))
    by_class_member = {}
    by_name = {}
    for ext in data.get('extensions', []):
        for cmd in ext.get('commands', []):
            cmd_id = int(cmd['id'], 16)
            by_name[cmd.get('name', '').upper()] = cmd_id
            cls = cmd.get('class')
            member = cmd.get('member')
            if cls and member:
                by_class_member[(cls.lower(), member.lower())] = cmd_id
    return by_class_member, by_name


def load_synced_set(by_name):
    text = OPSYNC.read_text(encoding='utf-8')
    synced = {int(v, 16) for v in SYNCED_HEX_RE.findall(text)}
    for macro in SYNCED_MACRO_RE.findall(text):
        if macro in by_name:
            synced.add(by_name[macro])
    return synced


def opcode_desc(cmd):
    cls = cmd.get('class')
    mem = cmd.get('member')
    if cls and mem:
        return f"{cls}.{mem}"
    return cmd.get('name', '')


def category_for(cmd):
    cls = (cmd.get('class') or '').lower()
    mem = (cmd.get('member') or '').lower()
    name = cmd.get('name', '').lower()
    full = f"{cls}.{mem} {name}"

    if cls == 'camera' or 'cutscene' in full or 'widescreen' in full or 'fade' in full:
        return 'cutscene/camera'
    if cls in {'task', 'char', 'car', 'vehicle', 'ped', 'player', 'train', 'heli', 'plane', 'boat'} or 'task_' in name:
        return 'task/ped/vehicle control'
    if cls in {'text', 'audio', 'conversation'} or 'text' in full or 'audio' in full:
        return 'mission text/audio'
    if cls in {'world', 'zone', 'game', 'garage', 'clock', 'weather', 'mission'} or 'mission' in full:
        return 'world state toggles'
    return 'other'


def parse_args():
    parser = argparse.ArgumentParser(
        description='Audit storyline opcodes against syncedOpcodes coverage.',
    )
    parser.add_argument(
        '--output',
        type=Path,
        default=OUT_MD,
        help='Path to write the generated markdown report.',
    )
    parser.add_argument(
        '--max-missing',
        type=int,
        default=None,
        help='Fail with exit code 1 if missing opcode count is greater than this value.',
    )
    return parser.parse_args()


def main():
    args = parse_args()
    out_md = args.output
    if not out_md.is_absolute():
        out_md = ROOT / out_md

    targets = load_targets()
    by_class_member, by_name = load_opcode_index()
    data = json.loads(SA_JSON.read_text(encoding='utf-8'))
    command_by_id = {}
    for ext in data.get('extensions', []):
        for cmd in ext.get('commands', []):
            command_by_id[int(cmd['id'], 16)] = cmd

    synced = load_synced_set(by_name)

    used = Counter()
    seen_in_files = defaultdict(set)

    for path in targets:
        if not path.exists():
            continue
        txt = path.read_text(encoding='utf-8', errors='ignore')
        for m in NUM_OPCODE_RE.findall(txt):
            op = int(m, 16) & 0x7FFF
            used[op] += 1
            seen_in_files[op].add(path.name)

        for cls, member in DOT_CALL_RE.findall(txt):
            key = (cls.lower(), member.lower())
            op = by_class_member.get(key)
            if op is None:
                continue
            used[op] += 1
            seen_in_files[op].add(path.name)

    missing = [op for op in used if op not in synced]

    triaged = []
    for op in sorted(missing):
        cmd = command_by_id.get(op, {'name': 'UNKNOWN'})
        triaged.append({
            'opcode': op,
            'name': cmd.get('name', 'UNKNOWN'),
            'display': opcode_desc(cmd),
            'category': category_for(cmd),
            'count': used[op],
            'files': sorted(seen_in_files[op]),
        })

    must_add = [0x00BC, 0x00BF, 0x00DF, 0x00FE, 0x00FF, 0x0256, 0x03EE, 0x0417]

    existing_review_log = None
    if out_md.exists():
        previous_text = out_md.read_text(encoding='utf-8')
        marker = '\n## Wave audit review log\n'
        if marker in previous_text:
            existing_review_log = previous_text.split(marker, 1)[1].strip()

    lines = [
        '# Storyline opcode parity backlog',
        '',
        'Generated by `python tools/opcode_audit.py` from storyline targets referenced in `docs/ROADMAP.md`.',
        '',
        f'- Target script files scanned: {len(targets)}',
        f'- Unique opcodes observed in target scripts: {len(used)}',
        f'- Missing vs `syncedOpcodes`: {len(missing)}',
        '',
        '## Missing opcodes triage',
        '',
        '| Opcode | Command | Category | Hits | Example scripts |',
        '| --- | --- | --- | ---: | --- |',
    ]

    for item in sorted(triaged, key=lambda x: (-x['count'], x['opcode'])):
        files = ', '.join(item['files'][:3])
        lines.append(f"| `0x{item['opcode']:04X}` | `{item['display']}` | {item['category']} | {item['count']} | {files} |")

    lines += [
        '',
        '## Must-add before parity (initial backlog)',
        '',
        '| Opcode | Command | Why now |',
        '| --- | --- | --- |',
    ]

    triage_map = {t['opcode']: t for t in triaged}
    for op in must_add:
        item = triage_map.get(op)
        if not item:
            continue
        lines.append(
            f"| `0x{op:04X}` | `{item['display']}` | {item['category']}; used {item['count']} times across {len(item['files'])} target scripts. |"
        )

    lines += [
        '',
        '## Wave audit ledger (update per implementation wave)',
        '',
        f'- Last generated: {date.today().isoformat()}',
        '- Command: `python3 tools/opcode_audit.py --output docs/qa/storyline-opcode-backlog.md`',
        '- Workflow: rerun at wave start and wave end; commit the refreshed backlog output with each wave progress update.',
        '- Review gate: do not promote any wave status to `complete` until the latest start+end audit output has been reviewed and recorded here.',
        '- Triage rule: if new commands/opcodes appear, append a short triage note with why the command is needed, where it is used, and which mission list is blocked pending support.',
        '',
        '| Wave | Script family | Owner | Missing opcode count | Newly discovered command coverage gaps |',
        '| --- | --- | --- | ---: | --- |',
    ]

    for wave_id, family, owner, count, gaps in wave_missing_rows(triaged):
        lines.append(f"| {wave_id} | `{family}` | {owner} | {count} | {gaps} |")

    lines += [
        '',
        '## Wave audit review log',
        '',
    ]

    if existing_review_log:
        lines.append(existing_review_log)
    else:
        lines += [
            'Record one entry at wave start and one at wave end.',
            '',
            '| Date | Wave | Audit checkpoint | Missing opcode count | Review result | Notes / triage refs |',
            '| --- | --- | --- | ---: | --- | --- |',
        ]

    out_md.parent.mkdir(parents=True, exist_ok=True)
    out_md.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(f'Wrote {out_md}')
    print(f'Missing vs syncedOpcodes: {len(missing)}')

    if args.max_missing is not None and len(missing) > args.max_missing:
        print(
            f'ERROR: missing opcode count {len(missing)} exceeds threshold {args.max_missing}.',
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
