#!/usr/bin/env python3
"""Compare the native full saucer movement handler; radar and weapon constructors are isolated."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    data = exe[70800:]
    rng = random.Random(0x53415543)
    output = bytearray(b'CORNSAU1' + struct.pack('<I', 512))
    for i in range(512):
        native = load(exe)
        kind, quality = rng.choice((50, 53)), rng.randrange(8)
        template = struct.unpack_from('<H', data, 0x2cca + kind * 2)[0]
        record = bytearray(data[template:template + 74])
        position = tuple(rng.randrange(-3000000, 3000000) for _ in range(2)) + (rng.randrange(0, 500000),)
        attitude = tuple(rng.randrange(65536) for _ in range(3))
        velocity = (0, 0, 0) if i < 16 else tuple(rng.randrange(-2500, 2501) for _ in range(3))
        base = tuple(rng.randrange(-2000000, 2000000) for _ in range(2)) + (720,)
        flags = (struct.unpack_from('<H',record,24)[0] & ~0x7006) | (quality << 12) | (i % 4) * 2
        struct.pack_into('<3i3H', record, 0, *position, *attitude)
        struct.pack_into('<H', record, 24, flags)
        record[27] = rng.randrange(1, 51)
        struct.pack_into('<3hH', record, 46, *velocity, rng.choice((32767, 32700, 32000, rng.randrange(1,32000))))
        elapsed = rng.randrange(2, 16)
        native.mem_write(DATA_ADDRESS + 0xf800, bytes(record))
        native.mem_write(DATA_ADDRESS + 0x2a9, struct.pack('<3i', *base))
        native.mem_write(DATA_ADDRESS + 0x1e5e, struct.pack('<h', elapsed))
        native.mem_write(DATA_ADDRESS + 0x1b7f, struct.pack('<h', 5))
        native.mem_write(DATA_ADDRESS + 0xec28, struct.pack('<H', 0xf880))
        for offset in (0xab2b, 0xaf8e, 0xb1a2, 0xc4e9):
            native.mem_write(LOAD_ADDRESS + offset - 512, b'\xc3')
        native.hook_add(UC_HOOK_CODE, lambda uc, a, n, _: uc.reg_write(UC_X86_REG_EFLAGS, uc.reg_read(UC_X86_REG_EFLAGS) | 1),
                        begin=LOAD_ADDRESS + 0xab2b - 512, end=LOAD_ADDRESS + 0xab2b - 512)
        native.reg_write(UC_X86_REG_SI, 0xf800)
        native.reg_write(UC_X86_REG_DI, 0)
        native.reg_write(UC_X86_REG_EFLAGS, 2)
        run(native, 0xc102)
        output += record + struct.pack('<3iH', *base, elapsed) + native.mem_read(DATA_ADDRESS + 0xf800, 74)
    (ROOT / 'tests/reference/saucers.bin').write_bytes(output)
    (ROOT / 'tests/reference/saucers.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'cases':512, 'qualities':list(range(8)), 'routine':'0xc102 BMRTIM',
        'stubs':['0xab2b RADARPROCESS skips drawing/proximity audio', '0xaf8e bomb constructor', '0xb1a2 KLA constructor', '0xc4e9 virtual AAA firing'],
        'scope':'Whole living movement handler: objective rest/takeoff, modes, height correction, wave terms, speed noise and limiting; all eight qualities, including stationary starts'
    }, indent=2) + '\n')
    print('Recorded 512 original saucer movement passes')


if __name__ == '__main__':
    main()
