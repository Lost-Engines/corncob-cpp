#!/usr/bin/env python3
"""Execute the unchanged original vehicle handler, including pursuit/capture state and PRNG continuation."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    data = exe[70800:]
    rng = random.Random(0x56454849)
    output = bytearray(b'CORNVEH1' + struct.pack('<I',512))
    for i in range(512):
        native = load(exe)
        kind, quality = rng.choice((12,48)), rng.randrange(8)
        template = struct.unpack_from('<H',data,0x2cca+kind*2)[0]
        record = bytearray(data[template:template+74])
        position = tuple(rng.randrange(-200000,200001) for _ in range(2)) + (rng.randrange(-5000,5001),)
        observer = tuple(rng.randrange(-400000,400001) for _ in range(3))
        if i % 4 == 0: observer = (position[0] + rng.choice((0,32*64,100*64,256*64,512*64)),position[1],720)
        struct.pack_into('<3i3H',record,0,*position,*(rng.randrange(65536) for _ in range(3)))
        flags = (struct.unpack_from('<H',record,24)[0] & ~0x7806) | (quality<<12) | (i%4)*2
        if i % 3 == 0: flags |= 0x800
        if i < 16: flags = (quality<<12)|0x71
        struct.pack_into('<H',record,24,flags)
        struct.pack_into('<3hH',record,46,*(rng.randrange(-2000,2001) for _ in range(3)),rng.choice((0,1,15,32000)))
        maximum = max(abs(position[k]-observer[k]) for k in range(3))
        record[18] = 0 if maximum < 32768 else 1 if maximum < 524288 else 2
        elapsed, ticks, on_foot, seed = rng.randrange(2,16), rng.randrange(65536),bool(i%3),rng.randrange(2**32)
        native.mem_write(DATA_ADDRESS+0xf800,bytes(record))
        native.mem_write(DATA_ADDRESS+0xb05,struct.pack('<3i',*observer))
        native.mem_write(DATA_ADDRESS+0xafc,bytes((on_foot,)))
        native.mem_write(DATA_ADDRESS+0x1e5e,struct.pack('<h',elapsed))
        native.mem_write(DATA_ADDRESS+0x1b7f,struct.pack('<h',5))
        native.mem_write(DATA_ADDRESS+0x1efa,struct.pack('<H',32767))
        native.mem_write(DATA_ADDRESS+0xf722,struct.pack('<I',seed))
        native.mem_write(DATA_ADDRESS+0x8f94,bytes(14))
        native.mem_write(LOAD_ADDRESS+0x43,struct.pack('<H',ticks))
        native.reg_write(UC_X86_REG_SI,0xf800)
        native.reg_write(UC_X86_REG_DI,0)
        native.reg_write(UC_X86_REG_EFLAGS,2)
        run(native,0xb2cc)
        output += record + struct.pack('<3iHHBI',*observer,elapsed,ticks,on_foot,seed)
        output += native.mem_read(DATA_ADDRESS+0xf800,74) + native.mem_read(DATA_ADDRESS+0xf722,4) + native.mem_read(DATA_ADDRESS+0x8f94,14)
    (ROOT/'tests/reference/vehicles.bin').write_bytes(output)
    (ROOT/'tests/reference/vehicles.json').write_text(json.dumps({
        'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),
        'cases':512,'routine':'0xb2cc VECHTIM, RESETVECH and FOLLOWANGLES','stubs':[],
        'scope':'All eight qualities and both vehicle types; wandering, pursuit, halt/capture/rescue state, timer, velocity, angles and full random continuation; original renderer supplies the distance-tier byte'
    },indent=2)+'\n')
    print('Recorded 512 original vehicle handlers')


if __name__ == '__main__':
    main()
