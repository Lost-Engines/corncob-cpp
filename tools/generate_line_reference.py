#!/usr/bin/env python3
"""Record native EGALINE pixel decisions for direction, slope and endpoint ties."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_INSN, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_INS_OUT, UC_X86_REG_ES
from generate_native_reference import load, run, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256
from generate_raster_reference import Ega, checksum

ROOT=Path(__file__).resolve().parents[1]


def main():
    exe=(ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==EXPECTED_SHA256
    rng=random.Random(0x4c494e45)
    cases=[]
    for dx,dy in [(0,0),(0,17),(17,0),(2,1),(1,2),(4,2),(2,4),(31,31),(31,32),(32,31),(17,7),(7,17),(100,1),(100,2),(200,20),(100,33),(31,5),(60,13)]:
        for sign in [-1,1]:
            points=(200,100,200+dx,100+sign*dy)
            cases.append(points);cases.append(points[2:]+points[:2])
    cases += [tuple(rng.randrange(15,185) if k%2 else rng.randrange(20,610) for k in range(4)) for _ in range(80)]
    cases=[(routine,points) for routine in [0x9d15,0x98c4] for points in cases]
    output=bytearray(b'CORNLIN2'+struct.pack('<I',len(cases)))
    for routine,points in cases:
        native=load(exe);device=Ega()
        native.hook_add(UC_HOOK_INSN,lambda uc,port,size,value,_:device.out(port,size,value),None,1,0,UC_X86_INS_OUT)
        native.hook_add(UC_HOOK_MEM_READ,lambda uc,access,address,size,value,_:device.read(uc,address,size),begin=0xa0000,end=0xaffff)
        native.hook_add(UC_HOOK_MEM_WRITE,lambda uc,access,address,size,value,_:device.write(address,size,value),begin=0xa0000,end=0xaffff)
        native.reg_write(UC_X86_REG_ES,0xa000)
        native.mem_write(DATA_ADDRESS+0x1b83,struct.pack('<4h',*points))
        native.mem_write(DATA_ADDRESS+0x1b8b,b'\x0c');native.mem_write(DATA_ADDRESS+0x1b90,bytes(2))
        run(native,routine)
        output+=struct.pack('<B4hQ',routine==0x98c4,*points,checksum(device.pixels()))
    destination=ROOT/'tests/reference/lines.bin';destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),'cases':len(cases),'routines':['0x9d15 EGALINE','0x98c4 KLINE'],'stubs':[],'adapter':'Original EGA latches/masks/write modes','scope':'Complete indexed frames for all octants, reversed endpoints, zero-length lines, axis-aligned lines and exact decision ties. Endpoints are within the viewport; clipping is excluded from these references.'},indent=2)+'\n')
    print('Recorded',len(cases),'original EGALINE raster decisions',flush=True)


if __name__=='__main__':main()
