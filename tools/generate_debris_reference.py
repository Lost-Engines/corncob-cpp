#!/usr/bin/env python3
"""Capture original debris/incendiary constructors with allocation supplied by the harness."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_SI, UC_X86_REG_SP, UC_X86_REG_IP, UC_X86_REG_EFLAGS
from generate_native_reference import load, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    rng = random.Random(0x44454252)
    output = bytearray(b'CORNDEB2' + struct.pack('<I', 192))
    for case in range(192):
        native = load(exe)
        pattern, count, elapsed, seed = case % 4, rng.randrange(2, 20), rng.randrange(2, 16), rng.randrange(2**32)
        if case >= 64:
            pattern = 4 + case % 2 if case < 128 else 6
        if pattern == 6:
            count = rng.choice((0, 0x20, 0xc0))
        source = bytearray(74)
        struct.pack_into('<3i', source, 0, rng.randrange(-1000000, 1000001), rng.randrange(-1000000, 1000001), rng.choice((0, 65530, 100000)))
        struct.pack_into('<3h', source, 46, *(rng.randrange(-1500, 1501) for _ in range(3)))
        native.mem_write(DATA_ADDRESS + 0xf800, bytes(source))
        native.mem_write(DATA_ADDRESS + 0xf722, struct.pack('<I', seed))
        native.mem_write(DATA_ADDRESS + 0x1e5e, struct.pack('<H', elapsed))
        native.mem_write(DATA_ADDRESS + 0x1b7f, struct.pack('<H', 5))
        native.mem_write(DATA_ADDRESS + 0xec36, bytes([3 if pattern == 2 else 0]))
        native.mem_write(DATA_ADDRESS + 0x1cd, bytes([1 if pattern in (0, 4) else 0]))
        native.mem_write(DATA_ADDRESS + 0xf15e, bytes([count if pattern == 6 else 0]))
        native.mem_write(LOAD_ADDRESS + 0x9eee - 512, b'\xc3')
        allocated = []
        def allocate(uc, address, size, context):
            template = struct.unpack('<H', uc.mem_read(DATA_ADDRESS + 0xec28, 2))[0]
            slot = 0x8fa6 + 74 * len(allocated)
            uc.mem_write(DATA_ADDRESS + slot, bytes(uc.mem_read(DATA_ADDRESS + template, 74)))
            allocated.append(slot)
            uc.reg_write(UC_X86_REG_SI, slot)
            uc.reg_write(UC_X86_REG_EFLAGS, uc.reg_read(UC_X86_REG_EFLAGS) & ~1)
            sp = uc.reg_read(UC_X86_REG_SP)
            ret = struct.unpack('<H', uc.mem_read(0x70000 + sp, 2))[0]
            uc.reg_write(UC_X86_REG_SP, sp + 2)
            uc.reg_write(UC_X86_REG_IP, ret)
        native.hook_add(UC_HOOK_CODE, allocate, begin=LOAD_ADDRESS + 0x9fdb - 512, end=LOAD_ADDRESS + 0x9fdb - 512)
        native.reg_write(UC_X86_REG_SP, 0xffec)
        native.mem_write(0x7ffec, b'\0\0')
        native.reg_write(UC_X86_REG_SI, 0xf800)
        native.reg_write(UC_X86_REG_AX, count)
        native.reg_write(UC_X86_REG_BX, count)
        native.emu_start(LOAD_ADDRESS + (0xaf17 if pattern == 6 else 0xa407 if pattern in (4, 5) else 0xa356 if pattern == 3 else 0xa204) - 512, LOAD_ADDRESS, count=1000000)
        output += struct.pack('<4HI', pattern, count, elapsed, 5, seed) + source + struct.pack('<I', len(allocated))
        for slot in allocated:
            output += native.mem_read(DATA_ADDRESS + slot, 74)
        output += native.mem_read(DATA_ADDRESS + 0xf722, 4)
    (ROOT / 'tests/reference/debris.bin').write_bytes(output)
    (ROOT / 'tests/reference/debris.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'cases': 192, 'routines': ['0xa204 STRTFLT', '0xa356 STRTINCEND', '0xa407 STRTSHARD', '0xaf17 STRTBOOM'],
        'stubs': ['0x9fdb EXPSTRT: copies the requested native template into a separate live slot', '0x9eee MAKEBOOM: audio notification only'],
        'scope': 'Outward/upward debris, mortar barrages and incendiaries, with original PRNG draws, low-word altitude carry, velocities, lifetimes, flags, persistent shards, explosion drift and complete PRNG continuation'
    }, indent=2) + '\n')
    print('Recorded 192 native debris, shard and explosion emissions')


if __name__ == '__main__':
    main()
