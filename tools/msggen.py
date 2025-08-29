#!/usr/bin/env python3
import os
import sys
from pathlib import Path

# tools/msggen.py -> uORB -> libraries -> project_root
ROOT = Path(__file__).resolve().parents[3]
MSG_DIR = ROOT / 'libraries/uORB/msg'
INC_DIR = ROOT / 'libraries/uORB/inc/topics'
META_DIR = ROOT / 'libraries/uORB/src/metadata'

BASIC_TYPES = {
    'int8': 'int8_t', 'uint8': 'uint8_t',
    'int16': 'int16_t', 'uint16': 'uint16_t',
    'int32': 'int32_t', 'uint32': 'uint32_t',
    'int64': 'int64_t', 'uint64': 'uint64_t',
    'float': 'float', 'double': 'double',
}

def parse_meta(line):
    s = line.strip()
    if not s.startswith('%'):
        return None
    s = s[1:].strip()
    parts = s.split()
    if len(parts) != 2:
        return None
    key, val = parts[0].lower(), parts[1]
    try:
        ival = int(val, 0)
    except Exception:
        return None
    if key in ('queue', 'instances'):
        return (key, ival)
    return None

def parse_field(line):
    line = line.strip()
    if not line or line.startswith('#') or line.startswith('%'):
        return None
    # support: "type name" or "type[n] name"
    parts = line.split()
    if len(parts) < 2:
        return None
    t = parts[0].strip()
    name = parts[1].strip()
    arr = None
    if '[' in t and t.endswith(']'):
        base = t[:t.index('[')]
        try:
            arr = int(t[t.index('[')+1:-1])
            if arr <= 0:
                raise ValueError('array size must be > 0')
        except Exception:
            raise ValueError(f'invalid array definition: {line}')
        t = base
    t_c = BASIC_TYPES.get(t, t)
    return (t_c, name, arr)

def gen_header(topic, fields, meta):
    tu = topic.upper()
    guard = f'__TOPIC_{tu}_H__'
    lines = []
    lines.append(f'#ifndef {guard}')
    lines.append(f'#define {guard}')
    lines.append('')
    lines.append('#include "uORB.h"')
    lines.append('#include <stdint.h>')
    lines.append('')
    # meta macros
    q = meta.get('queue')
    m = meta.get('instances')
    if q is not None:
        lines.append(f'#define ORB_{tu}_QUEUE_LENGTH {q}')
    if m is not None:
        lines.append(f'#define ORB_{tu}_MAX_INSTANCES {m}')
    if q is not None or m is not None:
        lines.append('')
    lines.append('#ifdef __cplusplus')
    lines.append('extern "C" {')
    lines.append('#endif')
    lines.append('')
    struct_name = f'{topic}_s'
    lines.append(f'struct {struct_name} {{')
    for (t, n, arr) in fields:
        if arr:
            lines.append(f'    {t} {n}[{arr}];')
        else:
            lines.append(f'    {t} {n};')
    lines.append('};')
    lines.append('')
    lines.append(f'ORB_DECLARE({topic});')
    lines.append('')
    lines.append('#ifdef __cplusplus')
    lines.append('}')
    lines.append('#endif')
    lines.append('')
    lines.append(f'#endif /* {guard} */')
    return '\n'.join(lines) + '\n'

def fields_signature(fields):
    items = []
    for (t, n, arr) in fields:
        base = t
        if base == 'int8_t': base = 'int8'
        if base == 'uint8_t': base = 'uint8'
        if base == 'int16_t': base = 'int16'
        if base == 'uint16_t': base = 'uint16'
        if base == 'int32_t': base = 'int32'
        if base == 'uint32_t': base = 'uint32'
        if base == 'int64_t': base = 'int64'
        if base == 'uint64_t': base = 'uint64'
        if arr:
            items.append(f'{base}[{arr}] {n};')
        else:
            items.append(f'{base} {n};')
    return ''.join(items)

def gen_metadata(topic, fields, orb_id_enum, meta=None):
    lines = []
    lines.append(f'#include "topics/{topic}.h"')
    lines.append('')
    struct_name = f'struct {topic}_s'
    sig = fields_signature(fields)
    # embed meta tokens for runtime parsing
    if meta:
        q = meta.get('queue')
        m = meta.get('instances')
        if q is not None:
            sig = sig + f'@queue={q};'
        if m is not None:
            sig = sig + f'@instances={m};'
    lines.append(f'ORB_DEFINE({topic}, {struct_name}, sizeof({struct_name}), "{sig}", {orb_id_enum});')
    lines.append('')
    return '\n'.join(lines)

def ensure_dirs():
    INC_DIR.mkdir(parents=True, exist_ok=True)
    META_DIR.mkdir(parents=True, exist_ok=True)

def validate_fields(topic, fields):
    # fields: list of tuples (ctype, name, arr)
    info = {name: (ctype, arr) for (ctype, name, arr) in fields}
    if 'timestamp' not in info:
        print(f"[uorb-msggen] ERROR: '{topic}.msg' missing required field 'timestamp'", file=sys.stderr)
        return False
    ctype, arr = info['timestamp']
    if ctype != 'uint64_t' or arr is not None:
        print(f"[uorb-msggen] ERROR: '{topic}.msg' field 'timestamp' must be uint64 (uint64_t) and scalar", file=sys.stderr)
        return False
    return True

def main():
    ensure_dirs()
    if not MSG_DIR.exists():
        print(f'[uorb-msggen] WARN: msg dir not found: {MSG_DIR}', file=sys.stderr)
        return 0
    topics = []
    for p in sorted(MSG_DIR.glob('*.msg')):
        topic = p.stem
        meta = {}
        fields = []
        with p.open('r', encoding='utf-8') as f:
            for idx, line in enumerate(f.readlines(), start=1):
                m = parse_meta(line)
                if m:
                    meta[m[0]] = m[1]
                    continue
                try:
                    r = parse_field(line)
                except Exception as e:
                    print(f"[uorb-msggen] ERROR: {p.name}:{idx}: {e}", file=sys.stderr)
                    return 1
                if r:
                    fields.append(r)
        if not validate_fields(topic, fields):
            print(f"[uorb-msggen] ERROR: invalid fields in {p.name}", file=sys.stderr)
            return 1
        topics.append((topic, fields, meta))
    for idx, (topic, fields, meta) in enumerate(topics):
        h = gen_header(topic, fields, meta)
        (INC_DIR / f'{topic}.h').write_text(h, encoding='utf-8')
        c = gen_metadata(topic, fields, idx, meta)
        (META_DIR / f'{topic}_metadata.c').write_text(c, encoding='utf-8')
    print(f'[uorb-msggen] generated {len(topics)} topics into:')
    print(f'  headers:   {INC_DIR}')
    print(f'  metadata:  {META_DIR}')
    return 0

if __name__ == '__main__':
    sys.exit(main()) 