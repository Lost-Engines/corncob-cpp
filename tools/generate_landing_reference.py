#!/usr/bin/env python3
"""Record native wheel clearances, suspension, braking and DSQRT behaviour."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256
from generate_flight_reference import put
from generate_control_reference import matrix

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    rng = random.Random(0x5748454c)
    output = bytearray(b'CORNLD01' + struct.pack('<3I', 1024, 256, 512))
    native = load(exe)
    for i in range(1024):
        vector = tuple(rng.randrange(-30000, 30001) for _ in range(3)) if i else (0, 0, 0)
        native.mem_write(DATA_ADDRESS + 0xf800, struct.pack('<3h', *vector))
        native.reg_write(UC_X86_REG_SI, 0xf800)
        run(native, 0x792b)
        output.extend(struct.pack('<3hH', *vector, native.reg_read(UC_X86_REG_AX)))
    for _ in range(256):
        native = load(exe)
        attitude = (rng.randrange(65536), rng.randrange(-12000, 12001) & 65535, rng.randrange(-12000, 12001) & 65535)
        matrix(native, attitude, 0x1d48)
        native.mem_write(DATA_ADDRESS + 0xe97, struct.pack('<3H', *attitude))
        run(native, 0x129a)
        success = not bool(native.reg_read(UC_X86_REG_EFLAGS) & 1)
        output.extend(struct.pack('<3HB', *attitude, success))
        if success:
            output.extend(native.mem_read(DATA_ADDRESS + 0x1e42, 4))
            output.extend(native.mem_read(DATA_ADDRESS + 0x1db0, 2))
    for _ in range(512):
        native = load(exe)
        attitude = (rng.randrange(65536), rng.randrange(-2000, 2001) & 65535, rng.randrange(-2000, 2001) & 65535)
        matrix(native, attitude, 0x1d48)
        native.mem_write(DATA_ADDRESS + 0xe97, struct.pack('<3H', *attitude))
        run(native, 0x129a)
        clearance = struct.unpack('<h', native.mem_read(DATA_ADDRESS + 0x1db0, 2))[0]
        body_velocity = (rng.randrange(100, 12001), rng.randrange(-1000, 1001), rng.randrange(-500, 501))
        native.mem_write(DATA_ADDRESS + 0x1d9c, struct.pack('<3h', *body_velocity))
        native.reg_write(UC_X86_REG_BX, 0x1d48)
        native.reg_write(UC_X86_REG_SI, 0x1d9c)
        native.reg_write(UC_X86_REG_DI, 0x1dc2)
        run(native, 0x581d)
        velocity = bytes(native.mem_read(DATA_ADDRESS + 0x1dc2, 6))
        surface = rng.choice((0, 0, 50000, 100000))
        altitude = surface + clearance + 90 - rng.randrange(-400, 1250)
        if rng.randrange(10) == 0: altitude += 100000
        throttle, elapsed = rng.choice((20, 40, 41, 100, 200)), rng.randrange(2, 20)
        runway, brakes, old_ground = rng.randrange(2), rng.randrange(2), rng.randrange(2)
        roll = rng.randrange(-1000000, 1000001)
        for address, value in ((0x2bb, surface), (0xe93, altitude), (0x1dfe, roll)):
            put(native, address, value, 'I')
        for address, value in ((0x1de2, throttle), (0x1e5e, elapsed), (0x1b7f, 5)):
            put(native, address, value)
        for address, value in ((0xf53, runway), (0xf77, old_ground), (0xf7b, 0), (0xf7a, 0)):
            put(native, address, value, 'B')
        native.mem_write(LOAD_ADDRESS + 0x2cf - 512, b'\xc3')
        native.hook_add(UC_HOOK_CODE, lambda uc, a, n, _: uc.reg_write(UC_X86_REG_AX, int(brakes) if uc.reg_read(UC_X86_REG_AX) & 255 == 0x34 else 0),
                        begin=LOAD_ADDRESS + 0x2cf - 512, end=LOAD_ADDRESS + 0x2cf - 512)
        native.mem_write(LOAD_ADDRESS + 0x1236 - 512, b'\xc3')
        output.extend(struct.pack('<3H3h', *attitude, *body_velocity))
        output.extend(velocity)
        output.extend(struct.pack('<3i5H', surface, altitude, roll, throttle, elapsed, runway, brakes, old_ground))
        run(native, 0x1805)
        output.extend(struct.pack('<B', bool(native.reg_read(UC_X86_REG_EFLAGS) & 1)))
        for address, size in ((0x1dc2, 6), (0x1d9c, 6), (0x1dfe, 4), (0x1db2, 4), (0xf77, 1), (0xf7a, 2)):
            output.extend(native.mem_read(DATA_ADDRESS + address, size))
    (ROOT / 'tests/reference/landing.bin').write_bytes(output)
    (ROOT / 'tests/reference/landing.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'magnitude_cases': 1024, 'wheel_cases': 256, 'contact_cases': 512,
        'routines': ['0x792b VMAG', '0x129a FINDHEIGHT', '0x1805 DOLANDINGGEAR'],
        'stubs': ['0x02cf supplies recorded brake key', '0x1236 home-base statistics only'],
        'note': 'The preserved source disables DAMPFLAG; the 3.42 binary likewise omits optional suspension damping.'
    }, indent=2) + '\n')
    print('Recorded original magnitude, wheel-contact and suspension/braking results')


if __name__ == '__main__':
    main()
