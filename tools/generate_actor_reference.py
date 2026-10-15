#!/usr/bin/env python3
"""Record KLA steering with the radar drawing call supplied its measured relative-vector inputs."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_SI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256
from generate_flight_reference import put

ROOT = Path(__file__).resolve().parents[1]


def signed(value):
    return (value + 32768) % 65536 - 32768


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    rng = random.Random(0x4143544f)
    output = bytearray(b'CORNACT1' + struct.pack('<I', 512))
    for i in range(512):
        native = load(exe)
        record = bytearray(74)
        position = (rng.randrange(-300000, 300001), rng.randrange(-300000, 300001), rng.choice((100, 999, 1000, 2000, 100000)))
        target = tuple(position[k] + rng.randrange(-300000, 300001) for k in range(3))
        attitude = tuple(rng.randrange(65536) for _ in range(3))
        velocity = tuple(rng.randrange(-2000, 2001) for _ in range(3))
        timer, quality, elapsed = rng.choice((24000, 23900, 23899, rng.randrange(1, 23899))), rng.randrange(8), rng.randrange(2, 16)
        struct.pack_into('<3i3H', record, 0, *position, *attitude)
        struct.pack_into('<H', record, 24, 0x39 | (quality << 12))
        struct.pack_into('<3hH', record, 46, *velocity, timer)
        struct.pack_into('<3h', record, 60, 20, 16000, 20)
        direction = [signed(-((position[k] - target[k]) >> 4)) for k in range(3)]
        native.mem_write(DATA_ADDRESS + 0xf700, struct.pack('<3h', direction[0], direction[1], 0))
        native.reg_write(UC_X86_REG_SI, 0xf700)
        run(native, 0x792b)
        distance = native.reg_read(UC_X86_REG_AX)
        native.mem_write(DATA_ADDRESS + 0xf800, bytes(record))
        native.mem_write(DATA_ADDRESS + 0xf0d4, struct.pack('<3h', *direction))
        put(native, 0xf102, distance)
        put(native, 0x1e5e, elapsed)
        put(native, 0x1b7f, 5)
        native.mem_write(LOAD_ADDRESS + 0xab2b - 512, b'\xc3')
        native.hook_add(UC_HOOK_CODE, lambda uc, a, n, _: uc.reg_write(UC_X86_REG_EFLAGS, uc.reg_read(UC_X86_REG_EFLAGS) & ~1),
            begin=LOAD_ADDRESS + 0xab2b - 512, end=LOAD_ADDRESS + 0xab2b - 512)
        native.reg_write(UC_X86_REG_SI, 0xf800)
        native.reg_write(UC_X86_REG_EFLAGS, 2)
        run(native, 0xabd2)
        output.extend(record)
        output.extend(struct.pack('<3iH', *target, elapsed))
        output.extend(native.mem_read(DATA_ADDRESS + 0xf800, 74))
    (ROOT / 'tests/reference/actors.bin').write_bytes(output)
    (ROOT / 'tests/reference/actors.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'kla_cases': 512, 'routine': '0xabd2 DBALLTIM',
        'stubs': ['0xab2b RADARPROCESS: supplies scaled target vector and native VMAG horizontal distance; no radar pixels/audio'],
        'scope': 'Launch rest, quality-specific steering, native speed relaxation/noise, angular errors and low-ground avoidance'
    }, indent=2) + '\n')
    print('Recorded 512 native KLA steering passes')


if __name__ == '__main__':
    main()
