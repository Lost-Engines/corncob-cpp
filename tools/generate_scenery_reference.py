#!/usr/bin/env python3
"""Execute native local-distance and permanent scenery handlers without stubs."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_SI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    source = exe[70800:]
    rng = random.Random(0x5343454e)
    range_cases, handler_cases = 512, 576
    output = bytearray(b'CORNSCN1' + struct.pack('<II', range_cases, handler_cases))
    native = load(exe)
    boundaries = (0, 1, 1999, 2000, 2001, 3999, 4000, 4001, 32767, 32768,
                  -1999, -2000, -2001, -32767, -32768, -2147483648)
    for case in range(range_cases):
        observer = tuple(rng.randrange(-20000000, 20000000) for _ in range(3))
        if case < len(boundaries) * 3:
            delta = [0, 0, 0]
            delta[case % 3] = boundaries[case // 3]
        else:
            span = (2001, 4001, 32768, 100000, 2147483648)[case % 5]
            delta = [rng.randrange(-span, span) for _ in range(3)]
        position = tuple((observer[i] + delta[i] + 2147483648) % 4294967296 - 2147483648 for i in range(3))
        native.mem_write(DATA_ADDRESS + 0xb05, struct.pack('<3i', *observer))
        native.mem_write(DATA_ADDRESS + 0xf800, struct.pack('<3i', *position))
        native.mem_write(DATA_ADDRESS + 0x1efa, b'\0\0')
        native.reg_write(UC_X86_REG_SI, 0xf800)
        run(native, 0x967e)
        valid = not (native.reg_read(UC_X86_REG_EFLAGS) & 1)
        output += struct.pack('<6iBH', *position, *observer, valid, native.reg_read(UC_X86_REG_AX) if valid else 0)
    kinds = (3, 4, 5, 7, 29, 33, 34, 35, 36, 37, 38, 39)
    for case in range(handler_cases):
        native = load(exe)
        kind = kinds[case % len(kinds)]
        template = struct.unpack_from('<H', source, 0x2cca + kind * 2)[0]
        record = bytearray(source[template:template + 74])
        observer = tuple(rng.randrange(-1000000, 1000000) for _ in range(3))
        span = (1000, 2001, 4001, 32768, 100000)[case // len(kinds) % 5]
        position = tuple(observer[i] + rng.randrange(-span, span) for i in range(3))
        struct.pack_into('<3i', record, 0, *position)
        struct.pack_into('<3H', record, 12, *(rng.randrange(65536) for _ in range(3)))
        flags = struct.unpack_from('<H', record, 24)[0]
        if kind == 7:
            flags |= rng.choice((0, 0x800, 0xa00, 0x1000))
        else:
            flags |= rng.randrange(8) << 12
        struct.pack_into('<H', record, 24, flags)
        struct.pack_into('<H', record, 52, rng.randrange(65536))
        pose = 0
        if kind == 7 and rng.randrange(4) == 0:
            pose = 1
            struct.pack_into('<3H', record, 40, 0x6f71, 0x6f71, 0x6f71)
        if kind == 5 and rng.randrange(4) == 0:
            pose = 2
            struct.pack_into('<3H', record, 40, 0x5e2a, 0x5e2a, 0x5ff2)
        on_foot = rng.randrange(2)
        ticks = rng.randrange(4294967296)
        native.mem_write(DATA_ADDRESS + 0xf800, bytes(record))
        native.mem_write(DATA_ADDRESS + 0xb05, struct.pack('<3i', *observer))
        native.mem_write(DATA_ADDRESS + 0xafc, bytes([on_foot]))
        native.mem_write(DATA_ADDRESS + 0x1efa, b'\0\0')
        native.mem_write(DATA_ADDRESS + 0x1ca, b'\0')
        native.mem_write(DATA_ADDRESS + 0x22c, b'\0\0')
        native.mem_write(LOAD_ADDRESS + 0x43, struct.pack('<I', ticks))
        native.reg_write(UC_X86_REG_SI, 0xf800)
        native.reg_write(UC_X86_REG_EFLAGS, 2)
        routine = struct.unpack_from('<H', source, 0x2c50 + kind * 2)[0] + 512
        run(native, routine)
        output += struct.pack('<3iBBI', *observer, on_foot, pose, ticks) + record
        output += native.mem_read(DATA_ADDRESS + 0xf800, 74)
        output += bytes([bool(native.mem_read(DATA_ADDRESS + 0x1ca, 1)[0])])
        output += native.mem_read(DATA_ADDRESS + 0x22c, 2)
    destination = ROOT / 'tests/reference/scenery.bin'
    destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256,
        'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'range_cases': range_cases, 'handler_cases': handler_cases,
        'routines': ['0x967e EVALDIST', '0x6ff7 REL3D', '0x792b VMAG',
                     '0x8cfd MANDEST', '0x9887 HUTTIM', '0x8e7e FDESTRDR',
                     '0x9026 ROADTIM', '0x9020 DONOTHING', '0xbdbf INCONDEST'],
        'stubs': [],
        'scope': 'Full unchanged timed handlers, original local word-distance gate and magnitude, '
                 'latched gestures and paved poses, corpse rotation, objective credit and lockout, '
                 'tower entry, radar heading and permanent timers. Generic motion and painter-table '
                 'dispatch surround these calls and are excluded. Dword position wrapping is included.',
    }, indent=2) + '\n')
    print(f'Recorded {range_cases} native local distances and {handler_cases} complete scenery handlers', flush=True)


if __name__ == '__main__':
    main()
