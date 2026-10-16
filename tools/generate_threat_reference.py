#!/usr/bin/env python3
"""Record original AAA, balloon/bee and alien handlers, supplying only effect allocation."""
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
    rng = random.Random(0x54485254)
    output = bytearray(b'CORNTHR1' + struct.pack('<I', 1536))
    allocations = 0
    for i in range(1536):
        native = load(exe)
        kind = 23 if i < 512 else (40, 41)[i % 2] if i < 1024 else 57
        template = struct.unpack_from('<H', data, 0x2cca + kind * 2)[0]
        record = bytearray(data[template:template + 74])
        position = (rng.randrange(-3000000, 3000001), rng.randrange(-3000000, 3000001), rng.choice((-65536, -1, 0, 1000, 2000, 16382, 16383, 65535, 65536, 300000)))
        target = tuple(position[k] + rng.randrange(-1500000,1500001) for k in range(3))
        if i % 4 == 0: target = tuple(position[k] + rng.randrange(-30000,30001) for k in range(3))
        elif i % 2 == 0 or kind == 57: target = tuple(position[k] + rng.randrange(-400000,400001) for k in range(3))
        attitude = tuple(rng.randrange(65536) for _ in range(3))
        velocity = tuple(rng.randrange(-2000,2001) for _ in range(3))
        if i % 7 == 0: velocity = (0, 0, 0)
        flags = (struct.unpack_from('<H', record,24)[0] & ~0x7000) | rng.randrange(8) << 12
        if kind == 57 and i % 6 == 0: flags &= ~4
        struct.pack_into('<3i3H',record,0,*position,*attitude)
        struct.pack_into('<H',record,24,flags)
        struct.pack_into('<3hH',record,46,*velocity,rng.choice((16000,32000,32767)))
        # The renderer supplies the near/middle/far byte before invoking an actor.
        record[18] = 0 if max(abs(position[k] - target[k]) for k in range(3)) < 32768 else 1
        elapsed, on_foot = rng.randrange(2,16), bool(i % 3)
        target_velocity = tuple(rng.randrange(-32768,32768) for _ in range(3))
        seed = rng.randrange(2**32)
        native.mem_write(DATA_ADDRESS + 0xf722, struct.pack('<I', seed))
        native.mem_write(DATA_ADDRESS + 0xf800, bytes(record))
        native.mem_write(DATA_ADDRESS + 0xb05, struct.pack('<3i',*target))
        native.mem_write(DATA_ADDRESS + 0x1dc2, struct.pack('<3h',*target_velocity))
        native.mem_write(DATA_ADDRESS + 0x1e5e, struct.pack('<h',elapsed))
        native.mem_write(DATA_ADDRESS + 0x1b7f, struct.pack('<h',5))
        native.mem_write(DATA_ADDRESS + 0x1efa, struct.pack('<H',32767))
        native.mem_write(DATA_ADDRESS + 0xafc, bytes((on_foot if kind == 57 else 0,)))
        # ADD AA only updates scoring; EXPSTRT allocates the requested 74-byte template.
        native.mem_write(LOAD_ADDRESS + 0xaa93 - 512, b'\xc3')
        native.mem_write(LOAD_ADDRESS + 0x9fdb - 512, b'\xc3')
        allocated = []
        def allocate(uc, address, size, context):
            requested = uc.reg_read(UC_X86_REG_DI)
            uc.mem_write(DATA_ADDRESS + 0xf900, bytes(uc.mem_read(DATA_ADDRESS + requested,74)))
            uc.reg_write(UC_X86_REG_SI,0xf900)
            uc.reg_write(UC_X86_REG_EFLAGS,uc.reg_read(UC_X86_REG_EFLAGS) & ~1)
            allocated.append(True)
        native.hook_add(UC_HOOK_CODE, allocate, begin=LOAD_ADDRESS+0x9fdb-512, end=LOAD_ADDRESS+0x9fdb-512)
        native.reg_write(UC_X86_REG_SI,0xf800)
        native.reg_write(UC_X86_REG_DI,0)
        native.reg_write(UC_X86_REG_EFLAGS,2)
        run(native,{23:0xa64c,40:0xb88a,41:0xb875,57:0xc6cc}[kind])
        allocations += bool(allocated)
        output += record + struct.pack('<3i3hHBI',*target,*target_velocity,elapsed,on_foot,seed)
        output += native.mem_read(DATA_ADDRESS + 0xf800,74) + bytes((bool(allocated),))
        output += native.mem_read(DATA_ADDRESS + 0xf900,74)
    (ROOT/'tests/reference/threats.bin').write_bytes(output)
    (ROOT/'tests/reference/threats.json').write_text(json.dumps({
        'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),
        'cases':1536,'shell_launches':allocations,'routines':['0xa64c AATIM','0xb88a BBTIM','0xb875 BBSMTIM','0xc6cc ALTIM'],
        'stubs':['0xaa93 ADDAA score accumulation','0x9fdb EXPSTRT: allocation only, copies the requested original template'],
        'scope':'Living actor tracking, target lead, firing and shell initialization, balloon bounds/reversals, bee angles, alien activation/hops; original relative-vector and random routines execute unchanged'
    },indent=2)+'\n')
    print(f'Recorded 1536 native threat handlers, including {allocations} AAA shell launches')


if __name__ == '__main__':
    main()
