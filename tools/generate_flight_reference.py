#!/usr/bin/env python3
"""Execute original angle/force/integration routines for development fixtures."""
import hashlib
import json
from pathlib import Path
import random
import re
import struct
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_IP
from generate_native_reference import load, run, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def put(native, offset, value, fmt='H'):
    mask = (1 << (struct.calcsize(fmt) * 8)) - 1
    native.mem_write(DATA_ADDRESS + offset, struct.pack('<' + fmt, value & mask))


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    source = (ROOT.parent / 'corncob-reverse/analysis/original_source/ATN.ASM').read_text()
    table = [int(v) for v in re.findall(r'^\s*(?:atntbl\s+)?dw\s+(\d+)\s*$', source, re.M)]
    assert len(table) == 1024
    table_offset = exe.find(struct.pack('<1024H', *table))
    assert table_offset >= 0
    lines = ['#pragma once', '', '#include <array>', '#include <cstdint>', '', 'namespace corncob::maths {', '',
             f'// original ATN.ASM table; byte-identical to 3.42 at file offset 0x{table_offset:x}',
             'inline std::array<uint16_t, 1024> constexpr arctangent_table{']
    for i in range(0, len(table), 12):
        lines += ['  ' + ', '.join(map(str, table[i:i + 12])) + ',']
    lines += ['};', '', '} // namespace corncob::maths', '']
    (ROOT / 'src/maths/arctangent_table.h').write_text('\n'.join(lines))

    rng = random.Random(0x464c5954)
    angle_cases = [(x, y) for x in (-32768, -32767, -1, 0, 1, 32767)
                   for y in (-32768, -32767, -1, 0, 1, 32767)]
    angle_cases += [(rng.randrange(-32768, 32768), rng.randrange(-32768, 32768)) for _ in range(8192)]
    output = bytearray(b'CORNFY01' + struct.pack('<3I', len(angle_cases), 256, 128))
    native = load(exe)
    for x, y in angle_cases:
        native.reg_write(UC_X86_REG_AX, x & 65535)
        native.reg_write(UC_X86_REG_DX, y & 65535)
        run(native, 0xd5d5)
        assert native.reg_read(UC_X86_REG_IP) == 0
        output.extend(struct.pack('<2hH', x, y, native.reg_read(UC_X86_REG_AX)))
    print('Recorded', len(angle_cases), 'native airflow angles', flush=True)

    for case in range(256):
        native = load(exe)
        # Stub HUD/audio effects only; force, division, trigonometry and matrix code remain unchanged.
        for address in (0xcc91, 0xe0e9, 0xe0ce):
            native.mem_write(LOAD_ADDRESS + address - 512, b'\xc3')
        baseline = case < 8
        angles = (0, 0, 0) if baseline else tuple(rng.randrange(65536) for _ in range(3))
        velocity = (5000, 0, -1000) if baseline else tuple(rng.randrange(-12000, 12001) for _ in range(3))
        if case == 6: velocity = (0, 0, -1000)
        if case == 7: velocity = (-5000, 0, -1000)
        throttle = 20 if case == 1 else 200 if baseline else rng.randrange(20, 201)
        flaps = case == 2 if baseline else bool(rng.randrange(2))
        altitude = 1000000 if case == 3 else 0 if baseline else rng.randrange(-10000, 1800000)
        density = 3000 if case == 4 else 5957 if baseline else rng.randrange(1000, 10001)
        gravity = -96 if case == 5 else -48 if baseline else rng.randrange(-100, -14)
        elapsed = 5 if baseline else rng.randrange(2, 36)
        flags = 0 if baseline else rng.choice((0, 0x1000, 0x2000))
        on_ground = False if baseline else bool(rng.randrange(2))
        rocket = False if baseline else bool(rng.randrange(2))
        rocket_ticks = 2184 if baseline else rng.randrange(1, 2200)
        old_density = 5957 if baseline else rng.randrange(100, 10001)
        alpha = 0 if baseline else rng.randrange(-32768, 32768)
        for address, routine in ((0x1d36, 0x6021), (0x1d48, 0x6201)):
            native.mem_write(DATA_ADDRESS + 0xf800, struct.pack('<3H', *angles))
            native.reg_write(UC_X86_REG_BX, 0xf800)
            run(native, 0x5fe1)
            native.reg_write(UC_X86_REG_SI, address)
            run(native, routine)
        if baseline:
            for address in (0x1d36, 0x1d48):
                native.mem_write(DATA_ADDRESS + address, struct.pack('<9h', *(32767 if i in (0, 4, 8) else 0 for i in range(9))))
        native.mem_write(DATA_ADDRESS + 0x1dc2, struct.pack('<3h', *velocity))
        for address, value in ((0x1de2, throttle), (0x1ddc, gravity), (0x1e1c, density), (0x1e1e, 5957),
                               (0x1e20, old_density), (0x1e34, alpha), (0x1e5e, elapsed), (0x1b7f, 5),
                               (0x1de, flags), (0x19f4, rocket_ticks)):
            put(native, address, value)
        for address, value in ((0xf59, flaps), (0xf77, on_ground), (0x19f3, rocket)):
            put(native, address, value, 'B')
        put(native, 0xe93, altitude, 'I')
        output.extend(struct.pack('<3H3hHHiHHh7H', *angles, *velocity, throttle, int(flaps), altitude,
                                  density, 5957, gravity, elapsed, flags, int(on_ground), int(rocket), rocket_ticks,
                                  old_density, alpha & 65535))
        native.emu_start(LOAD_ADDRESS + 0x37d0 - 512, LOAD_ADDRESS + 0x3c0c - 512, count=100000)
        assert native.reg_read(UC_X86_REG_IP) == 0x3c0c - 512
        # Compare force accumulators and intermediate values, not only the rounded final velocity.
        for address, size in ((0x1d90, 12), (0x1d9c, 6), (0x1dc8, 6), (0x1db6, 6), (0x1dbc, 6),
                              (0x1dc2, 6), (0x1e20, 2), (0x1e24, 2), (0x1e34, 4), (0x1df0, 2),
                              (0x1e10, 2), (0x19f3, 1), (0x19f4, 2)):
            output.extend(native.mem_read(DATA_ADDRESS + address, size))
    print('Recorded 256 native force passes', flush=True)

    for _ in range(128):
        native = load(exe)
        position = tuple(rng.randrange(-8000000, 8000001) for _ in range(3))
        velocity = tuple(rng.randrange(-20000, 20001) for _ in range(3))
        acceleration = tuple(rng.randrange(-1000, 1001) for _ in range(3))
        flags, elapsed = rng.choice((0, 0x1000, 0x2000)), rng.randrange(2, 36)
        for axis in range(3):
            put(native, 0xb05 + axis * 4, position[axis], 'I')
            put(native, 0x1dc2 + axis * 2, velocity[axis])
            put(native, 0x1dbc + axis * 2, acceleration[axis])
        put(native, 0x1de, flags)
        put(native, 0x1e5e, elapsed)
        put(native, 0x1b7f, 5)
        output.extend(struct.pack('<3i6h2H', *position, *velocity, *acceleration, flags, elapsed))
        run(native, 0x15d5)
        assert native.reg_read(UC_X86_REG_IP) == 0
        output.extend(native.mem_read(DATA_ADDRESS + 0xb05, 12))
    destination = ROOT / 'tests/reference/flight.bin'
    destination.write_bytes(output)
    metadata = {'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
                'arctangent_table_file_offset': hex(table_offset), 'angle_cases': len(angle_cases),
                'force_cases': 256, 'position_cases': 128,
                'routines': {'atn2': '0xd5d5', 'forces': '0x37d0..0x3c0c', 'moveme': '0x15d5'},
                'stubs': {'0xcc91': 'drawbutton', '0xe0e9': 'voiceoff', '0xe0ce': 'voiceon'},
                'format': 'CORNFY01; three uint32 counts; (int16 x/y, uint16 angle); force inputs as <3H3hHHiHHh7H followed by native fields in the generator-listed order; position inputs <3i6h2H and outputs <3i; all little-endian',
                'scope': 'Original arithmetic executes unchanged; HUD and audio side effects omitted.'}
    (ROOT / 'tests/reference/flight.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print('Recorded 128 native position integrations', flush=True)


if __name__ == '__main__':
    main()
