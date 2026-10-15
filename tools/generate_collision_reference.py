#!/usr/bin/env python3
"""Measure fast and ordinary collision contacts in native Corncob 3.42."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_DI
from generate_native_reference import load, run, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    rng = random.Random(0x5156454c)
    output = bytearray(b'CORNCOL1' + struct.pack('<I', 1024))
    for i in range(1024):
        native = load(exe)
        called = []
        native.mem_write(LOAD_ADDRESS + 0x755d - 512, b'\xc3')
        native.hook_add(UC_HOOK_CODE, lambda uc, a, n, _: called.append(True),
            begin=LOAD_ADDRESS + 0x755d - 512, end=LOAD_ADDRESS + 0x755d - 512)
        a = [rng.randrange(-10000000, 10000001) for _ in range(3)]
        delta = [rng.randrange(-40000, 40001) for _ in range(3)]
        if i % 4 == 0: delta = [rng.randrange(-3000, 3001) for _ in range(3)]
        b = [a[k] + delta[k] for k in range(3)]
        velocity = [rng.randrange(-5000, 5001) for _ in range(3)]
        if i % 3 == 0: velocity = [max(-32768, min(32767, v // rng.randrange(1, 10))) for v in delta]
        radii = [rng.randrange(0, 7001) for _ in range(2)]
        fast = i < 512
        for address, position, radius, flags, kind in ((0xf800, a, radii[0], 9, 30 if fast else 20), (0xf850, b, radii[1], 9, 27)):
            record = bytearray(74)
            struct.pack_into('<3i', record, 0, *position)
            struct.pack_into('<H', record, 24, flags)
            struct.pack_into('<3h', record, 46, *velocity)
            struct.pack_into('<3H', record, 66, radius, radius, radius)
            struct.pack_into('<H', record, 72, kind)
            native.mem_write(DATA_ADDRESS + address, bytes(record))
        native.reg_write(UC_X86_REG_SI, 0xf800)
        native.reg_write(UC_X86_REG_DI, 0xf850)
        run(native, 0x7742 if fast else 0x767b)
        output.extend(struct.pack('<B3i3i3h2HB', fast, *a, *b, *velocity, *radii, bool(called)))
    (ROOT / 'tests/reference/collision.bin').write_bytes(output)
    (ROOT / 'tests/reference/collision.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'fast_contacts': 512, 'ordinary_contacts': 512,
        'routines': ['0x7742 QVELON', '0x767b QCOLLIS1'],
        'stubs': ['0x755d SLAM records a contact without invoking damage handlers'],
        'note': 'Measures actual SLAM contacts, not carry alone: QVELON may clear carry for an aligned but still too-distant shot.'
    }, indent=2) + '\n')
    print('Recorded 512 fast and 512 ordinary native contacts')


if __name__ == '__main__':
    main()
