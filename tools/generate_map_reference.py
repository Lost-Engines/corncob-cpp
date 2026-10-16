#!/usr/bin/env python3
"""Record unchanged REL3D depth capping for the original map projection."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_SI
from generate_native_reference import load, run, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    native = load(exe)
    native.mem_write(DATA_ADDRESS + 0xb05, bytes(12))
    native.mem_write(DATA_ADDRESS + 0x1efa, struct.pack('<H', 35))
    rng = random.Random(0x4d415050)
    output = bytearray(b'CORNMAP1' + struct.pack('<I', 512))
    for i in range(512):
        difference = (rng.randrange(-16000000, 16000000), rng.randrange(-16000000, 16000000), rng.choice((-1, 1)) * rng.randrange(30000000))
        native.mem_write(DATA_ADDRESS + 0xf800, struct.pack('<3i', *difference))
        native.reg_write(UC_X86_REG_SI, 0xf800)
        run(native, 0x6ff7)
        output += struct.pack('<3i', *difference) + native.mem_read(DATA_ADDRESS + 0xe7e, 12)
    (ROOT / 'tests/reference/map.bin').write_bytes(output)
    (ROOT / 'tests/reference/map.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'cases': 512, 'routine_file_offset': '0x6ff7 REL3D, with z0=35', 'stubs': [],
        'scope': 'Original apparent-depth capping and full signed three-component rescaling'
    }, indent=2) + '\n')
    print('Recorded 512 original map projection reductions')


if __name__ == '__main__':
    main()
