#!/usr/bin/env python3
"""Record unchanged RADARDOT coordinates and DSQRT radar compression."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_DX, UC_X86_REG_SI
from generate_native_reference import load, run, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    native = load(exe)
    output = bytearray(b'CORNRAD1' + struct.pack('<I', 512))
    rng = random.Random(0x52414452)
    for i in range(512):
        distance = i % 256
        heading = rng.randrange(65536)
        native.reg_write(UC_X86_REG_AX, distance)
        native.reg_write(UC_X86_REG_BX, heading)
        native.emu_start(LOAD_ADDRESS + 0xce47 - 512, LOAD_ADDRESS + 0xce96 - 512, count=10000)
        output += struct.pack('<2H', distance, heading)
        output += native.mem_read(DATA_ADDRESS + 0x2082, 4)
    # DSQRT entry is found from the unchanged VMAG call, rather than another square-root implementation.
    call = exe[0x794f:0x7952]
    assert call[0] == 0xe8, call.hex()
    sqrt_offset = 0x7952 + struct.unpack('<h', call[1:])[0]
    for squared in range(32768):
        native.reg_write(UC_X86_REG_AX, squared)
        native.reg_write(UC_X86_REG_DX, 0)
        run(native, sqrt_offset)
        output += struct.pack('<H', native.reg_read(UC_X86_REG_AX))
    (ROOT / 'tests/reference/radar.bin').write_bytes(output)
    (ROOT / 'tests/reference/radar.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'radar_coordinate_cases': 512, 'square_root_inputs': 32768, 'stubs': [],
        'routines': ['0xce47..0xce96 RADARDOT coordinates', hex(sqrt_offset) + ' DSQRT'],
        'scope': 'Original coordinate packing and every radar distance compression input; actual disc pixels covered by native raster checks'
    }, indent=2) + '\n')
    print('Recorded 512 radar coordinates and all 32768 radar square-root inputs')


if __name__ == '__main__':
    main()
