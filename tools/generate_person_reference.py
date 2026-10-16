#!/usr/bin/env python3
"""Record original free-fall damping and parachute swing."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_BX
from generate_native_reference import load, run, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    native = load(exe)
    rng = random.Random(0x43485554)
    output = bytearray(b'CORNPER1' + struct.pack('<II', 512, 512))
    for _ in range(512):
        velocity = tuple(rng.randrange(-20000, 20001) for _ in range(3))
        damping, terminal, elapsed = rng.choice((30000, 31000, 32000)), rng.choice((900, 4800)), rng.randrange(2, 51)
        native.mem_write(DATA_ADDRESS + 0x1da2, struct.pack('<3h', *velocity))
        native.mem_write(DATA_ADDRESS + 0x19ef, struct.pack('<2h', damping, terminal))
        native.mem_write(DATA_ADDRESS + 0x1e5e, struct.pack('<h', elapsed))
        native.mem_write(DATA_ADDRESS + 0x1b7f, struct.pack('<h', 5))
        run(native, 0x233f)
        output += struct.pack('<6h', *velocity, damping, terminal, elapsed) + native.mem_read(DATA_ADDRESS + 0x1da2, 6)
    native.mem_write(LOAD_ADDRESS + 0x644 - 512, b'\xc3')
    stick = [0, 0]
    native.hook_add(UC_HOOK_CODE, lambda uc, a, n, _: (uc.reg_write(UC_X86_REG_AX, stick[0] + 512), uc.reg_write(UC_X86_REG_BX, stick[1] + 512)),
                    begin=LOAD_ADDRESS + 0x644 - 512, end=LOAD_ADDRESS + 0x644 - 512)
    for i in range(512):
        attitude, saved = tuple(rng.randrange(65536) for _ in range(3)), tuple(rng.randrange(65536) for _ in range(3))
        tumble = tuple(rng.randrange(-8000, 8000) for _ in range(2))
        count = i + 1 if i < 508 else 500
        stick[:] = [rng.choice((0, 0, 0, 16, 32, 33, 512)), rng.choice((0, 0, 0, 16, 32, 33, 512))]
        native.mem_write(DATA_ADDRESS + 0xb11, struct.pack('<3H', *attitude))
        native.mem_write(DATA_ADDRESS + 0x1d84, struct.pack('<3H2h', *saved, *tumble))
        native.mem_write(DATA_ADDRESS + 0x19ea, struct.pack('<H', count))
        native.mem_write(DATA_ADDRESS + 0xb0d, struct.pack('<i', 100000))
        native.mem_write(DATA_ADDRESS + 0x2bf, struct.pack('<i', 0))
        run(native, 0x10c3)
        output += struct.pack('<6H2hH2h', *attitude, *saved, *tumble, count, *stick)
        output += native.mem_read(DATA_ADDRESS + 0xb11, 6) + native.mem_read(DATA_ADDRESS + 0x1d8a, 4) + native.mem_read(DATA_ADDRESS + 0x19ea, 2)
    (ROOT / 'tests/reference/person.bin').write_bytes(output)
    (ROOT / 'tests/reference/person.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'damping_cases': 512, 'swing_cases': 512, 'routines': ['0x233f DAMPCHUTE', '0x10c3 SWING'],
        'stubs': ['0x0644 JOYTD supplies recorded digital stick positions; no arithmetic or swing stubs']
    }, indent=2) + '\n')
    print('Recorded 512 original fall-damping and 512 parachute-swing passes')


if __name__ == '__main__':
    main()
