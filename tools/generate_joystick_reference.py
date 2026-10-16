#!/usr/bin/env python3
"""Check the original SPKE response using normalized digital-device samples."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_CX
from generate_native_reference import load, run, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent/'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    rng = random.Random(0x4a4f5953)
    native = load(exe)
    output = bytearray(b'CORNJOY1' + struct.pack('<I',512))
    for i in range(512):
        sample = (-32768,-32767,-1,0,1,32767)[i] if i < 6 else rng.randrange(-32768,32768)
        native.reg_write(UC_X86_REG_AX,min(abs(sample),32767))
        native.reg_write(UC_X86_REG_CX,32767)
        run(native,0x3d2)
        response = min(native.reg_read(UC_X86_REG_AX),512)
        output += struct.pack('<hh',sample,-response if sample < 0 else response)
    (ROOT/'tests/reference/joystick.bin').write_bytes(output)
    (ROOT/'tests/reference/joystick.json').write_text(json.dumps({
        'reference_sha256':EXPECTED_SHA256,'fixture_sha256':hashlib.sha256(output).hexdigest(),
        'cases':512,'routine':'0x3d2 SPKE','stubs':[],
        'scope':'Native quadratic response, followed by JOYTD sign and clipping; a digital full axis is 32767 units in place of an analogue timing calibration'
    },indent=2)+'\n')
    print('Recorded 512 native joystick response cases')


if __name__ == '__main__':
    main()
