#!/usr/bin/env python3
"""Record original CHANG EWALL geometry patches and collision dimensions."""
import hashlib
import json
from pathlib import Path
import random
import struct
from generate_native_reference import load, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    native = load(exe)
    rng = random.Random(0x57414c4c)
    frames = list(range(256)) + [rng.randrange(65536) for _ in range(256)]
    output = bytearray(b'CORNPUL1' + struct.pack('<I', len(frames)))
    word = lambda address: struct.unpack('<H', native.mem_read(DATA_ADDRESS + address, 2))[0]
    for frame in frames:
        native.mem_write(DATA_ADDRESS + 0x6312, struct.pack('<H', (frame - 1) & 65535))
        native.emu_start(LOAD_ADDRESS + 0x8ada - 512, LOAD_ADDRESS + 0x8bd6 - 512, count=100000)
        output += struct.pack('<I5H', frame, abs(struct.unpack('<h', native.mem_read(DATA_ADDRESS + 0x638b, 2))[0]),
                              abs(struct.unpack('<h', native.mem_read(DATA_ADDRESS + 0x638f, 2))[0]),
                              abs(struct.unpack('<h', native.mem_read(DATA_ADDRESS + 0x6429, 2))[0]), word(0x6a93), word(0x6a9f))
        for kind in (44, 45, 46, 47):
            template = word(0x2cca + kind * 2)
            output += native.mem_read(DATA_ADDRESS + template + 66, 6)
            model = word(template + 40)
            primitive = word(model + 10)
            line = word(primitive + 2)
            assert word(line) == 4
            output += native.mem_read(DATA_ADDRESS + line + 4, 30)
    (ROOT / 'tests/reference/model_animation.bin').write_bytes(output)
    (ROOT / 'tests/reference/model_animation.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'routine_file_offsets': '0x8ada..0x8bd6', 'cases': len(frames), 'stubs': [],
        'scope': 'Three native pulses, two saucer discs, four wire forcefields with every vertex and collision component'
    }, indent=2) + '\n')
    print('Recorded 512 original animated geometry passes')


if __name__ == '__main__':
    main()
