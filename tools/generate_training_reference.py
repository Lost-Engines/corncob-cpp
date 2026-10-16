#!/usr/bin/env python3
"""Execute original INITPERMS; supply only allocation, retaining native object construction and PRNG."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_SP, UC_X86_REG_EFLAGS
from generate_native_reference import load, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    rng = random.Random(0x54524149)
    output = bytearray(b'CORNTRN1' + struct.pack('<I', 32))
    for case in range(32):
        native = load(exe)
        counts = (8, 8, 6) if case == 0 else (0, 0, 0) if case == 1 else tuple(rng.randrange(17) for _ in range(3))
        qualities = (-1, -1, -1) if case < 2 else tuple(rng.randrange(-1, 8) for _ in range(3))
        seed = 2949243 if case < 2 else rng.randrange(2**32)
        native.mem_write(DATA_ADDRESS + 0xe6c, struct.pack('<3H3h', *counts, *qualities))
        native.mem_write(DATA_ADDRESS + 0xf722, struct.pack('<I', seed))
        allocated = []
        def allocate(uc, address, size, context):
            template = struct.unpack('<H', uc.mem_read(DATA_ADDRESS + 0xec28, 2))[0]
            slot = 0x8fa6 + 74 * len(allocated)
            assert slot < 0xec26
            uc.mem_write(DATA_ADDRESS + slot, bytes(uc.mem_read(DATA_ADDRESS + template, 74)))
            allocated.append(slot)
            uc.reg_write(UC_X86_REG_SI, slot)
            uc.reg_write(UC_X86_REG_EFLAGS, uc.reg_read(UC_X86_REG_EFLAGS) & ~1)
            sp = uc.reg_read(UC_X86_REG_SP)
            ret = struct.unpack('<H', uc.mem_read(0x70000 + sp, 2))[0]
            uc.reg_write(UC_X86_REG_SP, sp + 2)
            uc.reg_write(UC_X86_REG_IP, ret)
        from unicorn.x86_const import UC_X86_REG_IP
        native.hook_add(UC_HOOK_CODE, allocate, begin=LOAD_ADDRESS + 0x9fdb - 512, end=LOAD_ADDRESS + 0x9fdb - 512)
        native.reg_write(UC_X86_REG_SP, 0xffec)
        native.mem_write(0x7ffec, b'\0\0')
        native.emu_start(LOAD_ADDRESS + 0x904f - 512, LOAD_ADDRESS, count=1000000)
        output += struct.pack('<3H3hII', *counts, *qualities, seed, len(allocated))
        for slot in allocated:
            output += native.mem_read(DATA_ADDRESS + slot, 74)
    (ROOT / 'tests/reference/training.bin').write_bytes(output)
    (ROOT / 'tests/reference/training.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'cases': 32, 'routine': '0x904f INITPERMS',
        'stubs': ['0x9fdb EXPSTRT: copies the requested native 74-byte template into a separate live slot'],
        'scope': 'Original training layout, random coordinates, defence counts/qualities, flags, timers and motion templates'
    }, indent=2) + '\n')
    print('Recorded 32 native training worlds')


if __name__ == '__main__':
    main()
