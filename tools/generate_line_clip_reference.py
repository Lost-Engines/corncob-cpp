#!/usr/bin/env python3
"""Execute native OCclip with its original X-first, then Y clipping order."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, DATA_ADDRESS, EXPECTED_SHA256

ROOT=Path(__file__).resolve().parents[1]


def main():
    exe=(ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==EXPECTED_SHA256
    rng=random.Random(0x434c4950)
    cases=[]
    for x,y in [(8,4),(631,199),(8,199),(631,4),(319,101),(-1,-1),(-1,250),(700,250),(700,-1)]:
        for end in [(8,4),(631,199),(319,101),(-100,500),(700,-50)]:cases.append((x,y,*end))
    cases += [tuple(rng.randrange(-2000,2001) for _ in range(4)) for _ in range(512-len(cases))]
    output=bytearray(b'CORNCLP1'+struct.pack('<I',len(cases)))
    for points in cases:
        native=load(exe);native.mem_write(DATA_ADDRESS+0xf800,struct.pack('<4h',8,4,631,199))
        native.mem_write(DATA_ADDRESS+0x1b83,struct.pack('<4h',*points));native.reg_write(UC_X86_REG_SI,0xf800)
        run(native,0x5421);run(native,0x5458)
        codes=native.mem_read(DATA_ADDRESS+0x2077,2)
        accepted=not(codes[0]&codes[1])
        if accepted and (codes[0]|codes[1]):
            run(native,0x558a);accepted=not(native.reg_read(UC_X86_REG_EFLAGS)&1)
        output+=struct.pack('<4hB',*points,accepted)+native.mem_read(DATA_ADDRESS+0x1b83,8)
    destination=ROOT/'tests/reference/line_clipping.bin';destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),'cases':len(cases),'routines':['0x5421/0x5458 outcodes','0x558a OCclip'],'stubs':[],'scope':'Original inclusive wire viewport, rejection, X-first endpoint adjustment, recalculated slopes and subsequent Y clipping. Both directions and corner cases.'},indent=2)+'\n')
    print('Recorded 512 original wire-viewport clipping cases',flush=True)


if __name__=='__main__':main()
