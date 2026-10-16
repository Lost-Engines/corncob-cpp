#!/usr/bin/env python3
"""Execute original launch sites and complete actor constructors; supply only pool allocation."""
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
    rng = random.Random(0x4c41554e)
    output = bytearray(b'CORNLAU1' + struct.pack('<I', 768))
    successes = 0
    for i in range(768):
        native = load(exe)
        kind = [11, 13, 51][i // 256]
        template = struct.unpack_from('<H', data, 0x2cca + kind * 2)[0]
        record = bytearray(data[template:template + 74])
        struct.pack_into('<3i3H', record, 0, rng.randrange(-20000, 20001), rng.randrange(-20000, 20001), rng.randrange(20000), 123, 456, 789)
        struct.pack_into('<H', record, 24, (struct.unpack_from('<H',record,24)[0] & ~0x7000) | (i % 8) << 12)
        struct.pack_into('<H', record, 52, rng.choice([0, 1, 98, 99, 100, 149, 150, 199, 399, 400, rng.randrange(600)]))
        sites, population, elapsed = rng.randrange(13), rng.randrange(13), rng.randrange(2, 16)
        seed = rng.randrange(2**32)
        if i % 4 == 0: seed = (((rng.randrange(50) << 16) | rng.randrange(65536)) * pow(0x278dde6d, -1, 2**32)) % 2**32
        full = i % 13 == 0
        for offset, value, fmt in [(0x1e5e,elapsed,'H'), (0x1b7f,5,'H'), (0xf722,seed,'I'),
                                    (0x1dc,population,'H'), (0xf113,sites,'H'),
                                    (0xf0e8,population,'H'), (0xf0ea,sites,'H'),
                                    (0xec51,population,'H'), (0xec55,sites,'H')]:
            native.mem_write(DATA_ADDRESS + offset, struct.pack('<'+fmt,value))
        native.mem_write(DATA_ADDRESS + 0xf800, bytes(record))
        native.mem_write(LOAD_ADDRESS + 0x9fdb - 512, b'\xc3')
        attempts = []
        def allocate(uc, address, size, context):
            attempts.append(True)
            if full:
                uc.reg_write(UC_X86_REG_EFLAGS,uc.reg_read(UC_X86_REG_EFLAGS) | 1)
                return
            requested = uc.reg_read(UC_X86_REG_DI)
            uc.mem_write(DATA_ADDRESS + 0xf900, bytes(uc.mem_read(DATA_ADDRESS + requested,74)))
            uc.reg_write(UC_X86_REG_SI,0xf900)
            uc.reg_write(UC_X86_REG_EFLAGS,uc.reg_read(UC_X86_REG_EFLAGS) & ~1)
        native.hook_add(UC_HOOK_CODE, allocate, begin=LOAD_ADDRESS+0x9fdb-512, end=LOAD_ADDRESS+0x9fdb-512)
        native.reg_write(UC_X86_REG_SI,0xf800)
        native.reg_write(UC_X86_REG_DI,0)
        native.reg_write(UC_X86_REG_EFLAGS,2)
        run(native,{11:0xb6f6,13:0xb626,51:0xbfcd}[kind])
        allocated = bool(attempts) and not full
        successes += allocated
        output += record + struct.pack('<3HIB',sites,population,elapsed,seed,full)
        output += native.mem_read(DATA_ADDRESS + 0xf800,74) + struct.pack('<IB',struct.unpack('<I',native.mem_read(DATA_ADDRESS+0xf722,4))[0],allocated)
        output += native.mem_read(DATA_ADDRESS + 0xf900,74)
    (ROOT/'tests/reference/launchers.bin').write_bytes(output)
    (ROOT/'tests/reference/launchers.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'cases':768, 'constructors':successes, 'routines':['0xb6f6 KLA site','0xb626 vehicle site','0xbfcd saucer site'],
        'stubs':['0x9fdb EXPSTRT: fixed-pool allocation succeeds or fails; successful allocation copies the requested original template'],
        'scope':'All eight qualities, previous-census population caps, reset/retry timers, complete child constructors, exhausted pools and full random continuation.'
    },indent=2)+'\n')
    print(f'Recorded 768 original launch-site handlers and {successes} complete child constructors',flush=True)


if __name__ == '__main__': main()
