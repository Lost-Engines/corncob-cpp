#!/usr/bin/env python3
"""Record the original animation-frame word increment/restart block."""
import hashlib
import json
from pathlib import Path
import random
import struct
from generate_native_reference import load, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256
from unicorn.x86_const import UC_X86_REG_IP

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    randomizer = random.Random(0x4652414d)
    cases = [0, 1, 6, 7, 127, 128, 16382, 16383, 16384, 32767, 65534, 65535]
    cases += [randomizer.randrange(65536) for _ in range(512 - len(cases))]
    output = bytearray(b'CORNFRM1' + struct.pack('<I', len(cases)))
    native = load(exe)
    for current in cases:
        native.mem_write(DATA_ADDRESS + 0x1bc8, struct.pack('<H', current))
        native.emu_start(LOAD_ADDRESS + 0x2a3f - 512, LOAD_ADDRESS + 0x2a51 - 512, count=100)
        assert native.reg_read(UC_X86_REG_IP) == 0x2a51 - 512
        output += struct.pack('<H', current) + native.mem_read(DATA_ADDRESS + 0x1bc8, 2)
    destination = ROOT / 'tests/reference/frame_clock.bin'
    destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256,
        'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'routine_file_offsets': '0x2a3f..0x2a51',
        'cases': len(cases),
        'stubs': [],
        'scope': 'Native frame-word increment, 16,384-to-128 restart and word overflow',
    }, indent=2) + '\n')
    print('Recorded 512 native frame-clock transitions')


if __name__ == '__main__':
    main()
