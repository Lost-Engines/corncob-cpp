#!/usr/bin/env python3
"""Record generic motion and strict box overlaps from the original 3.42 executable."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256
from generate_flight_reference import put

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    rng = random.Random(0x4d4f5645)
    output = bytearray(b'CORNMO01' + struct.pack('<2I', 512, 512))
    for i in range(512):
        native = load(exe)
        record = bytearray(74)
        position = (rng.randrange(-10000000, 10000001), rng.randrange(-10000000, 10000001), rng.choice((-100, 10, 100, 10000, 100000)))
        attitude = tuple(rng.randrange(65536) for _ in range(3))
        velocity = tuple(rng.randrange(-1500, 1501) for _ in range(3))
        rotates = i % 2
        motion = tuple(rng.randrange(-2000, 2001) for _ in range(3)) + (rng.choice((0, 10, 31, 200)), rng.choice((0, 8000, 20000)), rng.choice((0, 54, 55, 256 + 54)))
        elapsed, normal = rng.randrange(2, 16), 5
        struct.pack_into('<3i3H', record, 0, *position, *attitude)
        record[26] = rotates
        struct.pack_into('<3h', record, 46, *velocity)
        struct.pack_into('<6h', record, 54, *motion)
        native.mem_write(DATA_ADDRESS + 0xf800, bytes(record))
        for address, value in ((0x1ce, motion[3]), (0x1d0, motion[4]), (0x1d2, motion[5] & 255), (0x1e5e, elapsed), (0x1b7f, normal)):
            put(native, address, value)
        native.reg_write(UC_X86_REG_SI, 0xf800)
        output.extend(record)
        output.extend(struct.pack('<2H', elapsed, normal))
        run(native, 0x9ffd)
        output.extend(native.mem_read(DATA_ADDRESS + 0xf800, 74))
        output.append(bool(native.reg_read(UC_X86_REG_EFLAGS) & 1))
    for i in range(512):
        native = load(exe)
        native.mem_write(LOAD_ADDRESS + 0x755d - 512, b'\xc3')
        extents = [tuple(rng.randrange(0, 65001) for _ in range(3)) for _ in range(2)]
        if i % 7 == 0: extents[i % 2] = (0, 1000, 1000)
        a = tuple(rng.randrange(-10000000, 10000001) for _ in range(3))
        b = tuple(a[axis] + rng.choice((-1, 1)) * (sum(e[axis] for e in extents) + rng.randrange(-2, 3)) for axis in range(3))
        for address, position, extent in ((0xf800, a, extents[0]), (0xf850, b, extents[1])):
            record = bytearray(74)
            struct.pack_into('<3i', record, 0, *position)
            struct.pack_into('<3H', record, 66, *extent)
            native.mem_write(DATA_ADDRESS + address, bytes(record))
        native.reg_write(UC_X86_REG_SI, 0xf800)
        native.reg_write(UC_X86_REG_DI, 0xf850)
        run(native, 0x7586)
        output.extend(struct.pack('<3i3H3i3HB', *a, *extents[0], *b, *extents[1], not bool(native.reg_read(UC_X86_REG_EFLAGS) & 1)))
    (ROOT / 'tests/reference/motion.bin').write_bytes(output)
    (ROOT / 'tests/reference/motion.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'motion_cases': 512, 'box_cases': 512,
        'routines': ['0x9ffd OBJMOV', '0x7586 CHKBOX'],
        'stubs': ['0x755d successful-collision callback; only the overlap predicate is measured'],
        'scope': 'Native PRNG, quadratic drag, gravity, time scaling, rotation, ground stopping and strict unsigned-extent boxes'
    }, indent=2) + '\n')
    print('Recorded 512 native object motions and 512 strict box overlaps')


if __name__ == '__main__':
    main()
