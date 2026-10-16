#!/usr/bin/env python3
"""Execute the native BIOS-tick rocket recharge and ready-lamp update without stubs."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_CS
from generate_native_reference import load, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256

ROOT=Path(__file__).resolve().parents[1]


def main():
    exe=(ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==EXPECTED_SHA256
    rng=random.Random(0x524f434b)
    output=bytearray(b'CORNROC1'+struct.pack('<I',1024))
    for i in range(1024):
        old=rng.randrange(65536);now=(old+rng.choice((0,1,2,17,65535)))&65535
        stock=rng.choice((0,1,1000,2180,2183,2184)) if i<512 else rng.randrange(65536)
        elapsed=rng.choice((5,50,255,65535));normal=rng.choice((5,5,1,32,65535));burning=bool(i%2)
        native=load(exe)
        native.mem_write(LOAD_ADDRESS+0x37,struct.pack('<H',now))
        native.mem_write(DATA_ADDRESS+0x19f3,struct.pack('<BHH',11 if burning else 0,stock,old))
        native.mem_write(DATA_ADDRESS+0x1e5e,struct.pack('<HH',elapsed,normal))
        native.emu_start(LOAD_ADDRESS+0x45f5-512,LOAD_ADDRESS+0x4658-512,count=10000)
        output+=struct.pack('<5HB',old,now,stock,elapsed,normal,burning)
        output+=native.mem_read(DATA_ADDRESS+0x19f4,4)+native.mem_read(DATA_ADDRESS+0xf1fc,1)
    destination=ROOT/'tests/reference/rocket.bin';destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),'cases':1024,'routine':'0x45f5..0x4657 rocket recharge and readiness','stubs':[],'scope':'Original BIOS tick wrapping, time-advance correction, division-overflow guard, stock wrapping, overshoot retention and needle colours. Native input helper also runs unchanged; no ignition key is supplied.'},indent=2)+'\n')
    print('Recorded 1024 original rocket recharge and ready-indicator updates',flush=True)


if __name__=='__main__':main()
