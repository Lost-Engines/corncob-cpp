#!/usr/bin/env python3
"""Record original local rotations, angle recovery and aircraft control response."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_SI, UC_X86_REG_IP
from generate_native_reference import load, run, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256
from generate_flight_reference import put

ROOT = Path(__file__).resolve().parents[1]


def matrix(native, angles, destination, inverse=True):
    native.mem_write(DATA_ADDRESS + 0xf800, struct.pack('<3H', *angles))
    native.reg_write(UC_X86_REG_BX, 0xf800)
    run(native, 0x5fe1)
    native.reg_write(UC_X86_REG_SI, destination)
    run(native, 0x6201 if inverse else 0x6021)


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    rng = random.Random(0x53544943)
    output = bytearray(b'CORNCT01' + struct.pack('<2I', 512, 512))
    native = load(exe)
    for i in range(512):
        angles = tuple(rng.randrange(65536) for _ in range(3))
        if i < 8: angles = (i * 8192, 16384 if i & 1 else 49152, 0)
        change = tuple(rng.randrange(-3000, 3001) for _ in range(3))
        output.extend(struct.pack('<3H3h', *angles, *change))
        matrix(native, angles, 0xf840)
        native.reg_write(UC_X86_REG_BX, 0xf840)
        native.reg_write(UC_X86_REG_SI, 0xf860)
        run(native, 0xd2b7)
        output.extend(native.mem_read(DATA_ADDRESS + 0xf860, 6))
        native.reg_write(UC_X86_REG_AX, change[2] & 65535)
        native.reg_write(UC_X86_REG_BX, change[1] & 65535)
        native.reg_write(UC_X86_REG_CX, change[0] & 65535)
        native.reg_write(UC_X86_REG_SI, 0xf840)
        run(native, 0xd4db)
        output.extend(native.mem_read(DATA_ADDRESS + 0xf840, 18))
    for i in range(512):
        native = load(exe)
        for address in (0x1492, 0xcc91):
            native.mem_write(LOAD_ADDRESS + address - 512, b'\xc3')
        native.mem_write(LOAD_ADDRESS + 0x627 - 512, b'\xc3')
        controls = tuple(rng.randrange(-512, 513) for _ in range(3))
        native.hook_add(UC_HOOK_CODE, lambda uc, a, n, _: uc.reg_write(UC_X86_REG_AX, controls[2] & 65535 if uc.reg_read(UC_X86_REG_AX) & 255 == 0x2c else 0),
                        begin=LOAD_ADDRESS + 0x627 - 512, end=LOAD_ADDRESS + 0x627 - 512)
        angles = tuple(rng.randrange(65536) for _ in range(3))
        velocity = (rng.randrange(-6000, 12001), rng.randrange(-1000, 1001), rng.randrange(-1200, 1201))
        response = tuple(rng.randrange(-20000000, 20000001) for _ in range(3))
        wing_damage, rudder_damage = rng.randrange(-8, 9), rng.randrange(-8, 9)
        density, elapsed = rng.randrange(100, 6000), rng.randrange(2, 36)
        grounded, stalled, flaps, protected = (rng.randrange(2) for _ in range(4))
        attack, previous_attack = rng.randrange(-7000, 7001), rng.randrange(-7000, 7001)
        last_protected, almost = rng.randrange(1000), rng.randrange(1, 3000)
        side_force = rng.randrange(-10000, 10001)
        matrix(native, angles, 0x1d24)
        matrix(native, angles, 0x1d36, False)
        native.mem_write(DATA_ADDRESS + 0xb11, struct.pack('<3H', *angles))
        native.mem_write(DATA_ADDRESS + 0xe97, struct.pack('<3H', *angles))
        native.mem_write(DATA_ADDRESS + 0x1d9c, struct.pack('<3h', *velocity))
        native.mem_write(DATA_ADDRESS + 0x1dc8, struct.pack('<3h', *(v >> 4 for v in velocity)))
        native.mem_write(DATA_ADDRESS + 0x1dde, bytes(native.mem_read(DATA_ADDRESS + 0x1d36 + 10, 2)))
        native.mem_write(DATA_ADDRESS + 0x1de0, bytes(native.mem_read(DATA_ADDRESS + 0x1d36 + 16, 2)))
        for address, value in ((0x1dfe, response[0]), (0x1dfa, response[1]), (0x1e04, response[2])):
            put(native, address, value, 'I')
        for address, value in ((0x19fb, wing_damage), (0x19fd, rudder_damage), (0x1e20, density),
                               (0x1e34, attack), (0x1e36, previous_attack), (0x1e39, last_protected),
                               (0x1e16, almost), (0x1e10, side_force), (0x1e12, 3882 if flaps else 2912),
                               (0x1df0, -1 if stalled else 0), (0x1e5e, elapsed), (0x1b7f, 5)):
            put(native, address, value)
        for address, value in ((0xf77, grounded), (0xf53, 1), (0x1e38, protected), (0xf7b, 0), (0xafc, 0), (0xe59, 0), (0xe2e, 0)):
            put(native, address, value, 'B')
        output.extend(struct.pack('<3H3h3h3i2h2H4H2h2Hh', *angles, *controls, *velocity, *response,
                                  wing_damage, rudder_damage, density, elapsed, grounded, stalled, flaps, protected,
                                  attack, previous_attack, last_protected, almost, side_force))
        native.reg_write(UC_X86_REG_AX, controls[0] & 65535)
        native.reg_write(UC_X86_REG_BX, controls[1] & 65535)
        native.emu_start(LOAD_ADDRESS + 0x31a7 - 512, LOAD_ADDRESS + 0x37d0 - 512, count=100000)
        assert native.reg_read(UC_X86_REG_IP) == 0x37d0 - 512
        for address, count in ((0x1dfe, 4), (0x1dfa, 4), (0x1e04, 4), (0x1e38, 1), (0x1e39, 2), (0x1e16, 2), (0xb11, 6)):
            output.extend(native.mem_read(DATA_ADDRESS + address, count))
    destination = ROOT / 'tests/reference/controls.bin'
    destination.write_bytes(output)
    (ROOT / 'tests/reference/controls.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'matrix_cases': 512, 'control_cases': 512, 'routines': ['0xd2b7 GETANG', '0xd4db ROLPLN', '0x31a7..0x37d0 controls'],
        'stubs': ['0x0627 supplies recorded rudder input', '0x1492 flashstall HUD/random only', '0xcc91 drawbutton'],
        'scope': 'Undamaged stability, autopilot off, wheel contact marked runway to avoid taxi jitter; wing/rudder damage and stall limiter included.'
    }, indent=2) + '\n')
    print('Recorded 512 matrix/angle cases and 512 native control passes')


if __name__ == '__main__':
    main()
