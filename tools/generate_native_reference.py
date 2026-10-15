#!/usr/bin/env python3
"""Regenerate fixed-point reference cases by executing original Corncob 3.42 code.

Development only: requires Unicorn and the sibling reverse-engineering workspace.
Neither the build nor the application needs Python, Unicorn or a DOS executable.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CS,
    UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SI, UC_X86_REG_DI,
    UC_X86_REG_SS, UC_X86_REG_SP)

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_SHA256 = 'c411dde7d18e1dc19d718327b70d1e69321448520344d3baa4a0c8e68a71e81e'
LOAD_SEGMENT = 0x2000
LOAD_ADDRESS = LOAD_SEGMENT * 16
DATA_SEGMENT = 0x1129
DATA_ADDRESS = (LOAD_SEGMENT + DATA_SEGMENT) * 16


def load(data):
    header = struct.unpack_from('<14H', data)
    module = bytearray(data[header[4] * 16:])
    for i in range(header[3]):
        offset, segment = struct.unpack_from('<HH', data, header[12] + i * 4)
        address = segment * 16 + offset
        value = struct.unpack_from('<H', module, address)[0]
        struct.pack_into('<H', module, address, (value + LOAD_SEGMENT) & 0xffff)
    native = Uc(UC_ARCH_X86, UC_MODE_16)
    native.mem_map(0, 0x200000)
    native.mem_write(LOAD_ADDRESS, bytes(module))
    for reg, value in ((UC_X86_REG_CS, LOAD_SEGMENT),
                       (UC_X86_REG_DS, LOAD_SEGMENT + DATA_SEGMENT),
                       (UC_X86_REG_ES, LOAD_SEGMENT + DATA_SEGMENT),
                       (UC_X86_REG_SS, 0x7000)):
        native.reg_write(reg, value)
    return native


def run(native, offset):
    native.reg_write(UC_X86_REG_SP, 0xffec)
    native.mem_write(0x7ffec, b'\0\0')
    native.emu_start(LOAD_ADDRESS + offset - 512, LOAD_ADDRESS, count=10000)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE')
    args = parser.parse_args()
    data = args.exe.read_bytes()
    assert hashlib.sha256(data).hexdigest() == EXPECTED_SHA256, 'Reference must be the verified normal 3.42 flight engine'
    native = load(data)
    source = ROOT.parent / 'corncob-reverse/analysis/original_source/SIN.ASM'
    import re
    quarter = [int(value) for value in re.findall(r'^\s*(?:sintbl\s+)?dw\s+(\d+)\s*$', source.read_text(), re.M)]
    assert len(quarter) == 257
    assert data[0x10280:0x10280 + 514] == struct.pack('<257H', *quarter)
    lines = ['#pragma once', '', '#include <array>', '#include <cstdint>', '', 'namespace corncob::maths {', '',
             '// original SIN.ASM table; byte-identical to the 3.42 executable at file offset 0x10280',
             'inline std::array<uint16_t, 257> constexpr sine_quarter{']
    for offset in range(0, len(quarter), 8):
        lines.append('  ' + ', '.join(str(value) for value in quarter[offset:offset + 8]) + ',')
    lines.extend(['};', '', '} // namespace corncob::maths', ''])
    (ROOT / 'src/maths/sine_table.h').write_text('\n'.join(lines))

    counts = (65536, 512, 512, 128)
    output = bytearray(b'CORNREF1' + struct.pack('<4I', *counts))
    for angle in range(counts[0]):
        native.reg_write(UC_X86_REG_AX, angle)
        run(native, 0xd580)
        output.extend(struct.pack('<H', native.reg_read(UC_X86_REG_AX)))
    print('Executed all 65,536 signed sine inputs', flush=True)

    rng = random.Random(0x434f524e)
    boundary_angles = (0, 1, 63, 64, 16383, 16384, 32767, 32768, 49152, 65535)
    angle_cases = [(h, p, r) for h in boundary_angles for p in boundary_angles[:4] for r in (0, 16384)]
    angle_cases += [tuple(rng.randrange(65536) for _ in range(3)) for _ in range(counts[1] - len(angle_cases))]
    for angles in angle_cases:
        output.extend(struct.pack('<3H', *angles))
        for function in (0x6021, 0x6201):
            native.mem_write(DATA_ADDRESS + 0xf800, struct.pack('<3H', *angles))
            native.reg_write(UC_X86_REG_BX, 0xf800)
            run(native, 0x5fe1)
            native.reg_write(UC_X86_REG_SI, 0xf810)
            run(native, function)
            output.extend(native.mem_read(DATA_ADDRESS + 0xf810, 18))

    for i in range(counts[2]):
        matrix = [rng.randrange(-32768, 32768) for _ in range(9)]
        vector = [rng.randrange(-32768, 32768) for _ in range(3)]
        if i == 0:
            matrix = [32767 if row == column else 0 for row in range(3) for column in range(3)]
            vector = [32767, -32768, -1]
        output.extend(struct.pack('<12h', *matrix, *vector))
        native.mem_write(DATA_ADDRESS + 0xf800, struct.pack('<9h', *matrix))
        native.mem_write(DATA_ADDRESS + 0xf820, struct.pack('<3h', *vector))
        for function in (0x581d, 0x5807):
            native.reg_write(UC_X86_REG_SI, 0xf820)
            native.reg_write(UC_X86_REG_BX, 0xf800)
            native.reg_write(UC_X86_REG_DI, 0xf830)
            run(native, function)
            output.extend(native.mem_read(DATA_ADDRESS + 0xf830, 6))

    for _ in range(counts[3]):
        lhs = [rng.randrange(-32768, 32768) for _ in range(9)]
        rhs = [rng.randrange(-32768, 32768) for _ in range(9)]
        output.extend(struct.pack('<18h', *lhs, *rhs))
        native.mem_write(DATA_ADDRESS + 0xf800, struct.pack('<9h', *lhs))
        native.mem_write(DATA_ADDRESS + 0xf820, struct.pack('<9h', *rhs))
        for row in range(3):
            for column in range(3):
                native.reg_write(UC_X86_REG_SI, 0xf820 + column * 2)
                native.reg_write(UC_X86_REG_BX, 0xf800 + row * 6)
                native.reg_write(UC_X86_REG_DI, 0xf840)
                run(native, 0x570f)
                output.extend(native.mem_read(DATA_ADDRESS + 0xf840, 2))

    destination = ROOT / 'tests/reference/fixed_math.bin'
    destination.write_bytes(output)
    metadata = {'reference': 'normal Corncob 3D 3.42 / 3.EXE', 'sha256': EXPECTED_SHA256,
        'original_source_commit': '2759414da1e6e645c3640288577d1e0cc8c32be4',
        'sine_table_matches_source': True,
        'sine_inputs': counts[0], 'forward_and_inverse_matrices': counts[1],
        'saturated_and_fast_transforms': counts[2], 'matrix_products': counts[3],
        'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'routine_file_offsets': {'ssin': '0xd580', 'calcsin': '0x5fe1', 'calcmat': '0x6021',
            'ncalcmat': '0x6201', 'matvmul': '0x581d', 'pmvmul': '0x5807', 'dot2': '0x570f'},
        'format': 'CORNREF1; four little-endian uint32 counts; int16 sine results; (3 uint16 angles, 18 int16 matrix values); (12 int16 inputs, 6 int16 outputs); (18 int16 inputs, 9 int16 outputs)',
        'stubs': [], 'scope': 'Original integer routines executed directly; no DOS or hardware I/O'}
    destination.with_suffix('.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print('Recorded native matrix, transform and composition cases:', counts[1:], flush=True)


if __name__ == '__main__':
    main()
