#!/usr/bin/env python3
"""Execute native ADDTOETIME and the unchanged SAS elapsed-time library with a controlled DOS clock."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_INTR
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_CX, UC_X86_REG_DX
from generate_native_reference import load, run, DATA_ADDRESS, EXPECTED_SHA256

ROOT=Path(__file__).resolve().parents[1]
PIT=1193182


def main():
    exe=(ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==EXPECTED_SHA256
    rng=random.Random(0x54494d45)
    output=bytearray(b'CORNTMR1'+struct.pack('<I',512))
    for i in range(512):
        native=load(exe);before=rng.randrange(65536)
        segments=[rng.randrange(PIT*86400//1024) for _ in range(2)]
        if i<16:segments=[i*1165,(16-i)*1165]
        clock=[0]
        def dos(uc,interrupt,user):
            assert interrupt==0x21 and (uc.reg_read(UC_X86_REG_AX)>>8)==0x2c
            centiseconds=clock[0]*1024*100//PIT
            seconds,hundredths=divmod(centiseconds,100)
            minutes,seconds=divmod(seconds,60);hours,minutes=divmod(minutes,60)
            uc.reg_write(UC_X86_REG_CX,(hours<<8)|minutes);uc.reg_write(UC_X86_REG_DX,(seconds<<8)|hundredths)
        native.hook_add(UC_HOOK_INTR,dos)
        native.mem_write(DATA_ADDRESS+0x252,struct.pack('<H',before))
        output+=struct.pack('<H2I',before,*segments)
        for duration in segments:
            clock[0]=duration;native.mem_write(DATA_ADDRESS+0xf71e,bytes(4))
            run(native,0x20df)
            output+=native.mem_read(DATA_ADDRESS+0x252,2)
    destination=ROOT/'tests/reference/sortie_clock.bin';destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),'cases':512,'routines':['0x20df ADDTOETIME','unchanged SAS TMR_READ/time arithmetic'],'stubs':['DOS int 21h/AH=2Ch returns a controlled time-of-day, derived from the supplied PIT ticks'],'scope':'Two consecutive elapsed-time segments, fractional-second discard at a pause boundary, midnight-based clock conversion, native ignored-hour quirk and wrapping 16-bit flight result seconds.'},indent=2)+'\n')
    print('Recorded 512 original two-segment sortie-clock updates',flush=True)


if __name__=='__main__':main()
