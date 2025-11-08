import re
import yaml
from pathlib import Path

C_TO_KSY_MAP = {
    "uint8_t": "u1",
    "uint16_t": "u2",
    "uint32_t": "u4",
    "uint64_t": "u8",
}

def parse_c_struct(c_file_path):
    text = Path(c_file_path).read_text()
    # Extract fields inside the struct
    pattern = re.compile(r"typedef struct\s*\{([^}]*)\}")
    match = pattern.search(text)
    if not match:
        raise ValueError("Could not find struct definition")

    struct_body = match.group(1)
    fields = []
    for line in struct_body.splitlines():
        line = line.strip().rstrip(";")
        if not line or line.startswith("//"):
            continue
        type_name, field_name = line.split()[:2]
        fields.append((field_name, type_name))
    return fields

def parse_ksy(ksy_file_path):
    data = yaml.safe_load(Path(ksy_file_path).read_text())
    seq = data["seq"]
    fields = [(f["id"], f["type"]) for f in seq]
    return fields

def test_ksy_matches_c_struct():
    c_fields = parse_c_struct("beacon_stats.h")
    ksy_fields = parse_ksy("beacon_task.ksy")

    # Filter only the fields shared between both
    common_c = [(name, C_TO_KSY_MAP.get(t)) for name, t in c_fields if t in C_TO_KSY_MAP]

    assert len(ksy_fields) == len(common_c), f"Field count mismatch: {len(ksy_fields)} vs {len(common_c)}"

    for (ksy_name, ksy_type), (c_name, c_type) in zip(ksy_fields, common_c):
        assert ksy_name == c_name, f"Field name mismatch: {ksy_name} != {c_name}"
        assert ksy_type == c_type, f"Field type mismatch for {ksy_name}: {ksy_type} != {c_type}"
