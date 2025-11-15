#!/usr/bin/env python3
"""
Simple consistency check between beacon_stats C struct and Kaitai .ksy
- extracts field names from C header `src/tasks/beacon/beacon_stats.h`
- extracts seq ids from `ground_station/samwise.ksy`
- verifies the ordered fields from `reboot_counter` through `device_status`
  match exactly.

Exit code 0 on success, 1 on failure.
"""

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
C_HEADER = REPO_ROOT / 'src' / 'tasks' / 'beacon' / 'beacon_stats.h'
KSY_FILE = REPO_ROOT / 'ground_station' / 'samwise.ksy'

if not C_HEADER.exists():
    print(f"Missing C header: {C_HEADER}")
    sys.exit(2)
if not KSY_FILE.exists():
    print(f"Missing KSY file: {KSY_FILE}")
    sys.exit(2)

# parse C header for field names inside the struct
c_text = C_HEADER.read_text()
# find the struct body between first '{' after typedef and the closing '};'
m = re.search(r"typedef\s+struct\s*\{(.*?)\}\s*__attribute__", c_text, re.S)
if not m:
    print("Failed to parse C struct in", C_HEADER)
    sys.exit(2)
struct_body = m.group(1)
# find lines like: uint32_t reboot_counter;
fields = []
for line in struct_body.splitlines():
    line = line.strip()
    if not line or line.startswith('//'):
        continue
    mm = re.match(r"(?:uint32_t|uint64_t|uint16_t|uint8_t|int32_t|int16_t|int64_t)\s+([a-zA-Z0-9_]+)\s*;", line)
    if mm:
        fields.append(mm.group(1))

if not fields:
    print("No fields found in C struct")
    sys.exit(2)

# parse ksy for seq ids
ksy_text = KSY_FILE.read_text()
# collect '- id: name' occurrences in order
ksy_ids = re.findall(r"-\s+id:\s*([a-zA-Z0-9_]+)", ksy_text)
if not ksy_ids:
    print("No seq ids found in ksy")
    sys.exit(2)

# find the range in ksy corresponding to the beacon_stats fields: reboot_counter .. device_status
try:
    start = ksy_ids.index('reboot_counter')
    end = ksy_ids.index('device_status')
except ValueError as e:
    print("KSY missing required fields:", e)
    sys.exit(2)

ksy_sub = ksy_ids[start:end+1]

if ksy_sub == fields:
    print("OK: beacon_stats fields match samwise.ksy segment")
    sys.exit(0)
else:
    print("Mismatch between C struct fields and ksy seq segment")
    print("C fields:")
    print(fields)
    print("KSY fields:")
    print(ksy_sub)
    # show diffs
    for i, (c, k) in enumerate(zip(fields, ksy_sub)):
        if c != k:
            print(f" Index {i}: C={c} KSY={k}")
    # extra items
    if len(fields) > len(ksy_sub):
        print("Extra in C:", fields[len(ksy_sub):])
    if len(ksy_sub) > len(fields):
        print("Extra in KSY:", ksy_sub[len(fields):])
    sys.exit(1)
