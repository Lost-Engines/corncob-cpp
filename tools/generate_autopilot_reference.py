#!/usr/bin/env python3
"""Execute unchanged original autopilot feedback, PITCHLOCK and DOHLOCK arithmetic."""
import hashlib
import json
from pathlib import Path
import random
import struct
from generate_native_reference import load, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    rng = random.Random(0x4155544f)
    output = bytearray(b'CORNAUT1' + struct.pack('<I', 512))
    for i in range(512):
        native = load(exe)
        world_velocity = (rng.randrange(2000, 13001), rng.randrange(-1000, 1001), rng.choice((-1000, -150, -149, -15, -1, 0, 1, 15, 16, 149, 150, 1000)))
        body_velocity = rng.randrange(-1500, 1501)
        pitch, response = rng.randrange(65536), rng.randrange(-20000000, 20000001)
        engaged, locked = rng.randrange(2), rng.randrange(2)
        altitude, lock_height = rng.randrange(200000), rng.randrange(200000)
        previous = rng.randrange(-1500, 1501)
        for offset, raw in ((0x1dc2, struct.pack('<3h', *world_velocity)), (0x1da0, struct.pack('<h', body_velocity)),
                            (0xe99, struct.pack('<H', pitch)), (0x1dfa, struct.pack('<i', response)),
                            (0xe59, bytes([engaged, locked])), (0xe93, struct.pack('<i', altitude)),
                            (0xf47, struct.pack('<i', lock_height)), (0x19e8, struct.pack('<h', previous))):
            native.mem_write(DATA_ADDRESS + offset, raw)
        native.emu_start(LOAD_ADDRESS + 0x34d5 - 512, LOAD_ADDRESS + 0x3545 - 512, count=100000)
        output += struct.pack('<3hhHiHHii h', *world_velocity, body_velocity, pitch, response, engaged, locked, altitude, lock_height, previous)
        output += native.mem_read(DATA_ADDRESS + 0x1dfa, 4) + native.mem_read(DATA_ADDRESS + 0xe59, 2) + native.mem_read(DATA_ADDRESS + 0xf47, 4) + native.mem_read(DATA_ADDRESS + 0x19e8, 2)
    (ROOT / 'tests/reference/autopilot.bin').write_bytes(output)
    (ROOT / 'tests/reference/autopilot.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(), 'cases': 512,
        'routines': ['0x34d5..0x3545 autopilot feedback', '0x1327 PITCHLOCK', '0x13bc DOHLOCK'], 'stubs': []
    }, indent=2) + '\n')
    print('Recorded 512 original autopilot feedback passes')


if __name__ == '__main__':
    main()
