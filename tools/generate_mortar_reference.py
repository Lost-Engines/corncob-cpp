#!/usr/bin/env python3
"""Execute unchanged native MORTIM and its complete particle constructors."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256

ROOT=Path(__file__).resolve().parents[1]


def main():
    exe=(ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==EXPECTED_SHA256
    data=exe[70800:];rng=random.Random(0x4d4f5254)
    output=bytearray(b'CORNMRT1'+struct.pack('<I',256));total=0
    for i in range(256):
        native=load(exe)
        template=struct.unpack_from('<H',data,0x2cca+42*2)[0]
        record=bytearray(data[template:template+74])
        position=tuple(rng.randrange(-100000,100001) for _ in range(3))
        observer=tuple(position[k]+rng.randrange(-60000,60001) for k in range(3))
        if i%3==0:observer=tuple(position[k]+rng.randrange(-15000,15001) for k in range(3))
        struct.pack_into('<3i',record,0,*position)
        record[18]=0 if max(abs(position[k]-observer[k]) for k in range(3))<32768 else 1
        struct.pack_into('<3hH',record,46,*(rng.randrange(-500,501) for _ in range(3)),rng.choice((0,9993,9994,9995,9999,10000)))
        seed,elapsed,full=rng.randrange(2**32),rng.randrange(2,16),i%17==0
        for off,fmt,value in [(0xf722,'I',seed),(0x1e5e,'H',elapsed),(0x1b7f,'H',5),(0xec33,'H',0),(0xec35,'H',0)]:native.mem_write(DATA_ADDRESS+off,struct.pack('<'+fmt,value))
        native.mem_write(DATA_ADDRESS+0xf800,bytes(record));native.mem_write(DATA_ADDRESS+0xb05,struct.pack('<3i',*observer))
        native.mem_write(LOAD_ADDRESS+0x9fdb-512,b'\xc3');allocated=[]
        def allocate(uc,address,size,context):
            if full:
                uc.reg_write(UC_X86_REG_EFLAGS,uc.reg_read(UC_X86_REG_EFLAGS)|1);return
            requested=uc.reg_read(UC_X86_REG_DI);slot=0xf900+len(allocated)*74
            uc.mem_write(DATA_ADDRESS+slot,bytes(uc.mem_read(DATA_ADDRESS+requested,74)))
            allocated.append(slot);uc.reg_write(UC_X86_REG_SI,slot)
            uc.reg_write(UC_X86_REG_EFLAGS,uc.reg_read(UC_X86_REG_EFLAGS)&~1)
        native.hook_add(UC_HOOK_CODE,allocate,begin=LOAD_ADDRESS+0x9fdb-512,end=LOAD_ADDRESS+0x9fdb-512)
        native.reg_write(UC_X86_REG_SI,0xf800);native.reg_write(UC_X86_REG_DI,0);native.reg_write(UC_X86_REG_EFLAGS,2)
        run(native,0xb91c)
        output+=record+struct.pack('<3iHIB',*observer,elapsed,seed,full)+native.mem_read(DATA_ADDRESS+0xf800,74)+native.mem_read(DATA_ADDRESS+0xf722,4)+bytes([len(allocated)])
        for slot in allocated:output+=native.mem_read(DATA_ADDRESS+slot,74)
        total+=len(allocated)
    destination=ROOT/'tests/reference/mortars.bin';destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),'cases':256,'constructed_particles':total,'routines':['0xb91c MORTIM','unchanged STRTFLT and RAND_W'],'stubs':['EXPSTRT supplies an empty slot or reports an exhausted pool'],'scope':'Native timer gate, renderer distance tier, Manhattan range, complete emitted particles and random continuation.'},indent=2)+'\n')
    print('Recorded 256 original mortar updates and',total,'complete particles',flush=True)


if __name__=='__main__':main()
