#!/usr/bin/env python3
"""Record native randomized aircraft damage with visual-only indicator calls stubbed."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_CX
from generate_native_reference import load, run, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256
from generate_flight_reference import put

ROOT = Path(__file__).resolve().parents[1]
FIELDS = (0x1c7, 0x230, 0x19fb, 0x19fd, 0x19ff, 0x1a01, 0x1a0b, 0x1de6, 0x1de2)


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    rng = random.Random(0x44414d41)
    output = bytearray(b'CORNDAM1' + struct.pack('<I', 256))
    for i in range(256):
        native = load(exe)
        initial = (rng.randrange(0, 215), rng.randrange(0, 250), rng.randrange(-1650, 1651), rng.randrange(-330, 331), rng.randrange(1000),
                   rng.randrange(1000), rng.randrange(100), rng.randrange(20, 201), rng.randrange(20, 201))
        count, elapsed, invincible = rng.randrange(1, 76), rng.randrange(2, 16), int(i % 20 == 0)
        for field, value in zip(FIELDS, initial): put(native, field, value)
        put(native, 0xae6, invincible, 'B')
        put(native, 0x1e5e, elapsed)
        put(native, 0x1b7f, 5)
        native.mem_write(LOAD_ADDRESS + 0xcacb - 512, b'\xc3')
        native.reg_write(UC_X86_REG_CX, count)
        run(native, 0xf85)
        output.extend(struct.pack('<9H3H', *(v & 65535 for v in initial), count, elapsed, invincible))
        for field in FIELDS: output.extend(native.mem_read(DATA_ADDRESS + field, 2))
        output.extend(native.mem_read(DATA_ADDRESS + 0xf722, 4))
    (ROOT / 'tests/reference/damage.bin').write_bytes(output)
    (ROOT / 'tests/reference/damage.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'cases': 256, 'routine': '0x0f85 DAMAGEPLANE',
        'stubs': ['0xcacb DRAWDAMAGE: visual indicator only'],
        'fields': ['total', 'worst', 'wing', 'rudder', 'stability', 'wing_loss', 'engine_hits', 'maximum_throttle', 'throttle', 'PRNG_state']
    }, indent=2) + '\n')
    print('Recorded 256 native randomized aircraft damage passes')


if __name__ == '__main__':
    main()
