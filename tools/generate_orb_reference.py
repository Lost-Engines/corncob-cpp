#!/usr/bin/env python3
"""Execute original orb/objective and mindbender handlers without stubbing native logic."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]
COUNTS = [(27,0xec3e), (52,0xec63), (11,0xf113), (23,0xf150), (12,0xf0e8),
          (13,0xf0ea), (50,0xec51), (51,0xec55), (1,0xec61), (57,0xec57), (None,0xec5b)]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    data = exe[70800:]
    rng = random.Random(0x4f524253)
    output = bytearray(b'CORNORB2' + struct.pack('<I', 768))
    for i in range(768):
        native = load(exe)
        kind = [0,43,1][i//256]
        template = struct.unpack_from('<H', data, 0x2cca + kind * 2)[0]
        record = bytearray(data[template:template+74])
        position = (0,0,rng.choice([-65536,-1,0,3100,11199,11200,11201,65535,65536,100000]))
        span = [20000,100000,500000,800000][i % 4]
        observer = tuple(position[k] + rng.randrange(-span,span+1) for k in range(3))
        delta = tuple(position[k]-observer[k] for k in range(3))
        maximum = max(abs(v) for v in delta)
        tier = 0 if maximum<32768 else 1 if maximum<524288 else 2
        shift = [0,4,8][tier]
        squared = sum((v>>shift)**2 for v in delta)
        struct.pack_into('<3i3H',record,0,*position,*(rng.randrange(65536) for _ in range(3)))
        record[18]=tier
        struct.pack_into('<I',record,20,squared)
        flags=(struct.unpack_from('<H',record,24)[0]&~0x7a00)|(i%8)<<12
        if i%3==0: flags|=0x200
        if i%2==0: flags|=0x800
        struct.pack_into('<H',record,24,flags)
        record[27]=rng.randrange(1,66)
        struct.pack_into('<3h',record,46,*(rng.randrange(-1000,1001) for _ in range(3)))
        struct.pack_into('<3h3h',record,54,*(rng.randrange(-500,501) for _ in range(6)))
        initial_velocity=tuple(rng.randrange(-1000,1001) for _ in range(3))
        elapsed,on_foot,seed=rng.randrange(2,16),bool(i%3),rng.randrange(2**32)
        populations=[rng.randrange(7) for _ in COUNTS]
        if i%5==0:populations=[0]*len(COUNTS)
        populations[-1]=max(populations[-1],populations[-2])
        native.mem_write(DATA_ADDRESS+0xf800,bytes(record))
        native.mem_write(DATA_ADDRESS+0xb05,struct.pack('<3i',*observer))
        native.mem_write(DATA_ADDRESS+0x1dc2,struct.pack('<3h',*initial_velocity))
        native.mem_write(DATA_ADDRESS+0x1e5e,struct.pack('<h',elapsed))
        native.mem_write(DATA_ADDRESS+0x1b7f,struct.pack('<h',5))
        native.mem_write(DATA_ADDRESS+0xafc,bytes([on_foot]))
        native.mem_write(DATA_ADDRESS+0xf722,struct.pack('<I',seed))
        native.mem_write(DATA_ADDRESS+0x8f89,bytes(3))
        native.mem_write(DATA_ADDRESS+0x2dd1,bytes(2))
        native.mem_write(DATA_ADDRESS+0x22c,bytes(2))
        for (_,offset),value in zip(COUNTS,populations):native.mem_write(DATA_ADDRESS+offset,struct.pack('<H',value))
        preceding_orb = i % 4 == 0
        if preceding_orb:
            template = struct.unpack_from('<H',data,0x2cca)[0]
            preceding = bytearray(data[template:template+74])
            struct.pack_into('<3i',preceding,0,*observer)
            preceding[18] = 0
            native.mem_write(DATA_ADDRESS+0xfa00,bytes(preceding))
            native.reg_write(UC_X86_REG_SI,0xfa00)
            native.reg_write(UC_X86_REG_DI,0)
            native.reg_write(UC_X86_REG_EFLAGS,2)
            run(native,0x8dbc)
        native.reg_write(UC_X86_REG_SI,0xf800)
        native.reg_write(UC_X86_REG_DI,0)
        native.reg_write(UC_X86_REG_EFLAGS,2)
        run(native,{0:0x8dbc,43:0xbb97,1:0xc5df}[kind])
        output+=record+struct.pack('<3i3hHB11HI',*observer,*initial_velocity,elapsed,on_foot,*populations,seed)+bytes([preceding_orb])
        output+=native.mem_read(DATA_ADDRESS+0xf800,74)+native.mem_read(DATA_ADDRESS+0x1dc2,6)
        output+=native.mem_read(DATA_ADDRESS+0xf722,4)+native.mem_read(DATA_ADDRESS+0x22c,2)
        output+=native.mem_read(DATA_ADDRESS+0x8f89,1)+native.mem_read(DATA_ADDRESS+0x8f8a,2)+native.mem_read(DATA_ADDRESS+0x2dd1,2)
    (ROOT/'tests/reference/orbs.bin').write_bytes(output)
    (ROOT/'tests/reference/orbs.json').write_text(json.dumps({
        'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),
        'cases':768,'routines':['0x8dbc generator orb / 0xbc83 HQTIM','0xbb97 destroyed orb','0xc5df mindbender'],
        'stubs':[], 'scope':'All eight objective qualities, renderer-supplied distance tiers, previous populations, attraction, completed-orb fall/spin, health regeneration, distortion, hallucination and full PRNG continuation.',
        'population_order':COUNTS, 'preceding_orb': 'Each case includes a flag for a preceding live, non-attracting orb at the observer; its original handler executes before the tested object to check visual-effect overwrite ordering.'
    },indent=2)+'\n')
    print('Recorded 768 original orb/objective and mindbender updates',flush=True)


if __name__ == '__main__': main()
