#!/usr/bin/env python3
"""Record native DRAWBUTTON EGA pixels with its original colour-compare and two-page writes."""
import hashlib
import json
from pathlib import Path
import struct
from unicorn import UC_HOOK_INSN, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_INS_OUT, UC_X86_REG_AX, UC_X86_REG_BX
from generate_native_reference import load, run, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256
from generate_raster_reference import Ega, checksum

ROOT=Path(__file__).resolve().parents[1]


def main():
    exe=(ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==EXPECTED_SHA256
    output=bytearray(b'CORNBTN1'+struct.pack('<I',56))
    # Every colour and every bit position occurs, including on both sides of each button boundary.
    original=Ega()
    for plane in range(4):
        for y in range(350):
            for byte in range(80):
                value=sum((1 << (7-bit)) for bit in range(8) if (((byte*8+bit)*5+y*7)&15)&(1<<plane))
                original.planes[plane][y*80+byte]=value
    for button in range(14):
        for source,target in [(7,15),(0,1),(8,7),(5,0)]:
            native=load(exe);device=Ega()
            for plane in range(4):
                device.planes[plane][:28000]=original.planes[plane][:28000]
                device.planes[plane][0x7e00:0x7e00+28000]=original.planes[plane][:28000]
            native.hook_add(UC_HOOK_INSN,lambda uc,port,size,value,_:device.out(port,size,value),None,1,0,UC_X86_INS_OUT)
            native.hook_add(UC_HOOK_MEM_READ,lambda uc,access,address,size,value,_:device.read(uc,address,size),begin=0xa0000,end=0xaffff)
            native.hook_add(UC_HOOK_MEM_WRITE,lambda uc,access,address,size,value,_:device.write(address,size,value),begin=0xa0000,end=0xaffff)
            native.mem_write(DATA_ADDRESS+0x1c3e,struct.pack('<H',0xf800))
            native.mem_write(DATA_ADDRESS+0xf800,struct.pack('<4H',8,4,631,199))
            native.mem_write(DATA_ADDRESS+0x1b90,bytes(2))
            native.reg_write(UC_X86_REG_AX,source|(target<<8));native.reg_write(UC_X86_REG_BX,button)
            run(native,0xcc91)
            pixels=device.pixels();output+=struct.pack('<BBBQ',button,source,target,checksum(pixels))
            for plane in range(4):assert device.planes[plane][:28000]==device.planes[plane][0x7e00:0x7e00+28000]
    destination=ROOT/'tests/reference/buttons.bin';destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),'cases':56,'routine':'0xcc91 DRAWBUTTON with native RAWHLINEBIT','stubs':[],'adapter':'EGA read-mode 1 colour comparison; write modes, latches, masks, sequencer map mask and original two-page writes','scope':'All fourteen original button rectangles, four colour substitutions each, full-frame indexed-pixel checksums and matching second-page pixels.'},indent=2)+'\n')
    print('Recorded 56 complete original cockpit-button masks',flush=True)


if __name__=='__main__':main()
