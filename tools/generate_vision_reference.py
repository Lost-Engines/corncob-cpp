#!/usr/bin/env python3
"""Record original force-averaged blackout/capture state and all SETCOLORS DAC writes."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_INSN
from unicorn.x86_const import UC_X86_REG_AL, UC_X86_INS_OUT
from generate_native_reference import load, run, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe=(ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==EXPECTED_SHA256
    rng=random.Random(0x56495349)
    output=bytearray(b'CORNVIS1'+struct.pack('<2I',512,512))
    for i in range(512):
        native=load(exe)
        force=rng.randrange(-2**31,2**31)
        average=rng.randrange(-110*65536,110*65536)
        fade=rng.randrange(65536)
        frame=rng.choice((0,1,2,3,4,5,65535))
        elapsed,on_foot,closing=rng.randrange(2,16),bool(i%2),bool(i%3)
        native.mem_write(DATA_ADDRESS+0x1d98,struct.pack('<i',force))
        native.mem_write(DATA_ADDRESS+0xf6f,struct.pack('<HH',(average>>16)&65535,average&65535))
        native.mem_write(DATA_ADDRESS+0xf73,struct.pack('<H',200))
        native.mem_write(DATA_ADDRESS+0x270,struct.pack('<HH',fade,60))
        native.mem_write(DATA_ADDRESS+0x1bc8,struct.pack('<H',frame))
        native.mem_write(DATA_ADDRESS+0xafc,bytes((on_foot,)))
        native.mem_write(DATA_ADDRESS+0x1e5e,struct.pack('<h',elapsed))
        native.mem_write(DATA_ADDRESS+0x1b7f,struct.pack('<h',5))
        native.mem_write(LOAD_ADDRESS+0x6519-512,b'\xc3')
        run(native,0x1a5c)
        if closing:native.emu_start(LOAD_ADDRESS+0x2ef5-512,LOAD_ADDRESS+0x2f30-512,count=10000)
        hi,lo=struct.unpack('<HH',native.mem_read(DATA_ADDRESS+0xf6f,4))
        output+=struct.pack('<ii4H2B',force,average,fade,frame,elapsed,5,on_foot,closing)
        output+=struct.pack('<IH',(hi<<16)|lo,struct.unpack('<H',native.mem_read(DATA_ADDRESS+0x270,2))[0])
    for i in range(512):
        native=load(exe)
        intensity=i%256
        colours=bytes(rng.randrange(64) for _ in range(48))
        table=b''.join(bytes((index,))+colours[index*3:index*3+3] for index in range(16))
        native.mem_write(DATA_ADDRESS+0x1e6e,table)
        writes=[]
        def port_write(uc,port,size,value,context):
            if port==0x3c9:writes.append(value&255)
        native.hook_add(UC_HOOK_INSN,port_write,None,1,0,UC_X86_INS_OUT)
        native.reg_write(UC_X86_REG_AL,intensity)
        run(native,0x6519)
        assert len(writes)==48
        output+=struct.pack('<H',intensity)+colours+bytes(writes)
    (ROOT/'tests/reference/vision.bin').write_bytes(output)
    (ROOT/'tests/reference/vision.json').write_text(json.dumps({
        'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),
        'vision_cases':512,'palette_cases':512,
        'routines':['0x1a5c DODVELAVG','0x2ef5..0x2f30 capture fade','0x6519 SETCOLORS'],
        'stubs':['SETCOLORS returns without I/O in state cases; the separate palette cases execute and capture all original DAC writes'],
        'scope':'Force averaging, signed high-word clamps, on-foot recovery, first-frame gate, fractional capture fade and native six-bit palette products at every intensity'
    },indent=2)+'\n')
    print('Recorded 512 native visual-state cases and 512 complete palette writes')


if __name__=='__main__':main()
