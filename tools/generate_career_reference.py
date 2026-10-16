#!/usr/bin/env python3
"""Execute unchanged MOAG sortie-to-career accumulation, including best and peak records."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_CS, UC_X86_REG_SP
from generate_native_reference import load, LOAD_ADDRESS

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/MOAG.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == '6d8c922d59a873073796b367a2a40993fcaa5e763ed2ca907a945981f0c5bc32'
    rng = random.Random(0x43415245)
    output = bytearray(b'CORNCRR1' + struct.pack('<I',256))
    for i in range(256):
        pilot = bytearray(rng.randrange(256) for _ in range(623))
        pilot[:22] = b'Career test' + bytes(11)
        for offset in [28,162,296,430]:
            for field in range(0,130,2):struct.pack_into('<H',pilot,offset+field,rng.randrange(1000) if i<128 else rng.randrange(65536))
            struct.pack_into('<i',pilot,offset+130,rng.randrange(-300000,300001) if i<128 else rng.randrange(-2**31,2**31))
        pilot[598] = rng.randrange(4)
        stats = bytearray(struct.pack('<65H',*(rng.randrange(100) if i<128 else rng.randrange(65536) for _ in range(65))))
        struct.pack_into('<H',stats,0,rng.randrange(4096) | (0x8000 if i % 3 == 0 else 0))
        struct.pack_into('<H',stats,4,rng.randrange(20) if i<128 else rng.randrange(65535))
        native = load(exe)
        native.reg_write(UC_X86_REG_DS,0x3886)
        native.reg_write(UC_X86_REG_ES,0x3886)
        native.reg_write(UC_X86_REG_CS,0x3000)
        native.mem_write(0x38860+0xd000,bytes(pilot))
        native.mem_write(0x38860+0xd400,bytes(stats)+bytes(4))
        native.reg_write(UC_X86_REG_SP,0xffe0)
        native.mem_write(0x7ffe0,struct.pack('<4H',0,0x2000,0xd000,0xd400))
        native.emu_start(LOAD_ADDRESS+0x136c4-0x3200,LOAD_ADDRESS,count=1000000)
        output+=pilot+stats+native.mem_read(0x38860+0xd000,623)
    (ROOT/'tests/reference/career.bin').write_bytes(output)
    (ROOT/'tests/reference/career.json').write_text(json.dumps({
        'reference_sha256':hashlib.sha256(exe).hexdigest(),'fixture_sha256':hashlib.sha256(output).hexdigest(),
        'cases':256,'routine':'MOAG.EXE file 0x136c4..0x138fe; unchanged score helper and Borland arithmetic/memcpy',
        'stubs':[], 'scope':'Native sortie count, recalculated score, latest/best/peak/cumulative records and dword continuation. Promotion, awards and theatre scanning are separate subsequent decisions.'
    },indent=2)+'\n')
    print('Recorded 256 complete native career accumulation passes',flush=True)


if __name__ == '__main__': main()
