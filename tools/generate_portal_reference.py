#!/usr/bin/env python3
"""Execute the unchanged portal transfer position/PRNG block, excluding tile I/O."""
import hashlib
import json
from pathlib import Path
import random
import struct
from generate_native_reference import load, run, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_EFLAGS

ROOT=Path(__file__).resolve().parents[1]


def main():
    exe=(ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==EXPECTED_SHA256
    rng=random.Random(0x504f5254)
    output=bytearray(b'CORNPRT2'+struct.pack('<2I',512,256))
    for i in range(512):
        native=load(exe);flag=[0,1,255][i%3]
        start=tuple(rng.randrange(-8000000,8000001) for _ in range(3))
        position=tuple(rng.randrange(-8000000,8000001) for _ in range(3));seed=rng.randrange(2**32)
        native.mem_write(DATA_ADDRESS+0x8f88,bytes([flag]));native.mem_write(DATA_ADDRESS+0x2a9,struct.pack('<3i',*start))
        native.mem_write(DATA_ADDRESS+0xb05,struct.pack('<3i',*position));native.mem_write(DATA_ADDRESS+0xf722,struct.pack('<I',seed))
        if flag==0:run(native,0x217b)
        else:native.emu_start(LOAD_ADDRESS+0x2188-512,LOAD_ADDRESS+0x21ce-512,count=10000)
        output+=struct.pack('<B6iI',flag,*start,*position,seed)+native.mem_read(DATA_ADDRESS+0xb05,12)+native.mem_read(DATA_ADDRESS+0xf722,4)
    data=exe[70800:]
    template=struct.unpack_from('<H',data,0x2cca+58*2)[0]
    for i in range(256):
        native=load(exe);record=bytearray(data[template:template+74]);seed=rng.randrange(2**32)
        flags=struct.unpack_from('<H',record,24)[0]|((i%8)<<12)|(0x200 if i%3==0 else 0)
        struct.pack_into('<H',record,24,flags)
        struct.pack_into('<H',record,52,rng.choice((0,16383,16384,32000,65535)))
        native.mem_write(DATA_ADDRESS+0xf800,bytes(record));native.mem_write(DATA_ADDRESS+0xf722,struct.pack('<I',seed))
        native.reg_write(UC_X86_REG_SI,0xf800);native.reg_write(UC_X86_REG_DI,0);native.reg_write(UC_X86_REG_EFLAGS,2)
        run(native,0xc7ba)
        output+=record+struct.pack('<I',seed)+native.mem_read(DATA_ADDRESS+0xf800,74)+native.mem_read(DATA_ADDRESS+0xf722,4)+native.mem_read(DATA_ADDRESS+0x2dd3,1)
    destination=ROOT/'tests/reference/portals.bin';destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),'destination_cases':512,'handler_cases':256,'routines':['0x2188..0x21cd DOXP position block','0xc7ba XPTIM noncollision handler'],'stubs':[],'scope':'Original disabled, home and random portal destinations, including signed high words, full low words, fixed arrival altitude and four-draw RNG continuation. Tile unload/reload surrounds this block and is excluded. Includes complete noncollision XPTIM timer, completed-object colour, stippled flash colours and RNG continuation.'},indent=2)+'\n')
    print('Recorded 512 original portal destinations and random continuations',flush=True)


if __name__=='__main__':main()
