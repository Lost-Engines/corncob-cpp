#!/usr/bin/env python3
"""Execute the complete original shared-colour and random particle-disc animation block."""
import hashlib
import json
from pathlib import Path
import random
import struct
from generate_native_reference import load, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256

ROOT=Path(__file__).resolve().parents[1]


def main():
    exe=(ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==EXPECTED_SHA256
    rng=random.Random(0x50415254)
    output=bytearray(b'CORNPRC1'+struct.pack('<I',512))
    native=load(exe)
    for i in range(512):
        seed=rng.randrange(2**32)
        native.mem_write(DATA_ADDRESS+0xf722,struct.pack('<I',seed))
        native.emu_start(LOAD_ADDRESS+0x3ff1-512,LOAD_ADDRESS+0x4105-512,count=100000)
        output+=struct.pack('<I',seed)+native.mem_read(DATA_ADDRESS+0x31b6,1)
        for address,count in ((0x36fb,16),(0x35b7,16),(0x3e54,3)):
            for disc in range(count):output+=native.mem_read(DATA_ADDRESS+address+2+disc*10,8)
        output+=native.mem_read(DATA_ADDRESS+0xf722,4)
    destination=ROOT/'tests/reference/particles.bin';destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),'cases':512,'routines':['0x3ff1..0x4104 shared colour and SDS/SMS/flotsam disc randomisation'],'stubs':[],'scope':'Complete unchanged 141-draw native frame block; all 35 disc positions and diameters, shared colour, and PRNG continuation.'},indent=2)+'\n')
    print('Recorded 512 complete original particle-animation passes',flush=True)


if __name__=='__main__':main()
