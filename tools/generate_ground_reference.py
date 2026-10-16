#!/usr/bin/env python3
"""Execute original near-ground, star and complete three-bank initialisation routines."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_SI
from generate_native_reference import load,run,LOAD_ADDRESS,DATA_ADDRESS,EXPECTED_SHA256

ROOT=Path(__file__).resolve().parents[1]


def main():
    exe=(ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==EXPECTED_SHA256
    rng=random.Random(0x47524e44)
    output=bytearray(b'CORNGRD1'+struct.pack('<3I',512,256,64))
    for case in range(512):
        native=load(exe);observer=[rng.randrange(-2**31,2**31) for _ in range(3)]
        if case<16:observer[2]=[0,1,65535,65536,100000,33554431,33554432,67108864,-1,-65536,2**31-1,720,1000000,10000000,20000000,30000000][case]
        before=[rng.randrange(-2**31,2**31) for _ in range(3)];seed=rng.randrange(2**32)
        native.mem_write(DATA_ADDRESS+0xb05,struct.pack('<3i',*observer));native.mem_write(DATA_ADDRESS+0xf800,struct.pack('<3i',*before))
        native.mem_write(DATA_ADDRESS+0xf722,struct.pack('<I',seed));native.reg_write(UC_X86_REG_SI,0xf800)
        run(native,0xb7f)
        output+=struct.pack('<6iI',*observer,*before,seed)+native.mem_read(DATA_ADDRESS+0xf800,12)+native.mem_read(DATA_ADDRESS+0xf722,4)
    for case in range(256):
        native=load(exe);seed=rng.randrange(2**32)
        native.mem_write(DATA_ADDRESS+0xf722,struct.pack('<I',seed));native.reg_write(UC_X86_REG_SI,0xf800)
        run(native,0xc20)
        output+=struct.pack('<I',seed)+native.mem_read(DATA_ADDRESS+0xf800,12)+native.mem_read(DATA_ADDRESS+0xf722,4)
    for case in range(64):
        native=load(exe);observer=[rng.randrange(-8000000,8000001),rng.randrange(-8000000,8000001),rng.randrange(0,20000000)];seed=rng.randrange(2**32)
        native.mem_write(DATA_ADDRESS+0xb05,struct.pack('<3i',*observer));native.mem_write(DATA_ADDRESS+0xf722,struct.pack('<I',seed))
        native.emu_start(LOAD_ADDRESS+0x2616-512,LOAD_ADDRESS+0x2690-512,count=100000)
        output+=struct.pack('<3iI',*observer,seed)
        for address in (0x2eea0+2,0x2eea0+0x8fa):
            for point in range(80):output+=native.mem_read(address+point*14,12)
        output+=native.mem_read(DATA_ADDRESS+0xf722,4)
    destination=ROOT/'tests/reference/ground.bin';destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),'ground_cases':512,'star_cases':256,'initialisation_cases':64,'routines':['0xb7f GRNDPT with unchanged DRANDR','0xc20 SGRNDPT','0x2616..0x268f full three-bank initialisation'],'stubs':[],'scope':'Signed/wrapping ground ranges, underground no-op, independent high/low star coordinates, anchors, all 160 drawn initial positions, unused far-bank/random draws, and PRNG continuation.'},indent=2)+'\n')
    print('Recorded 512 ground points, 256 stars and 64 complete native bank initialisations',flush=True)


if __name__=='__main__':main()
