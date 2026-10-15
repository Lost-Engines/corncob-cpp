#!/usr/bin/env python3
"""Execute original DRAWPoly through the EGA fixture adapter; development only."""
import hashlib
import json
from pathlib import Path
import struct
from unicorn import UC_HOOK_INSN, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_INS_OUT, UC_X86_REG_SI, UC_X86_REG_BX, UC_X86_REG_SP, UC_X86_REG_IP
from generate_native_reference import load, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256
from generate_raster_reference import Ega, checksum

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    native = load(exe)
    device = [Ega()]
    native.hook_add(UC_HOOK_INSN, lambda uc, port, size, value, _: device[0].out(port, size, value), None, 1, 0, UC_X86_INS_OUT)
    native.hook_add(UC_HOOK_MEM_READ, lambda uc, access, address, size, value, _: device[0].read(uc, address, size), begin=0xa0000, end=0xaffff)
    native.hook_add(UC_HOOK_MEM_WRITE, lambda uc, access, address, size, value, _: device[0].write(address, size, value), begin=0xa0000, end=0xaffff)
    window = (8, 4, 631, 199, 623, 195, 300, 200, 319, 101, 321, 15920)
    native.mem_write(DATA_ADDRESS + 0xf800, struct.pack('<12H', *window))
    cases = [
        [(100, 50), (200, 70), (150, 150)],
        [(150, 150), (200, 70), (100, 50)],
        [(120, 40), (500, 40), (500, 180), (120, 180)],
        [(0, 0), (639, 0), (639, 349), (0, 349)],
        [(-200, 25), (320, -100), (620, 180), (100, 150)],
        [(100, 50), (101, 50), (150, 180)],
        [(100, 50), (150, 50), (200, 50)],
        [(100, 50), (200, 50), (220, 100), (200, 150), (100, 150), (80, 100)],
        [(100, 70), (200, 60), (300, 70), (320, 160), (180, 120)],
        [(20, -300), (300, 10), (620, 180), (10, 180)],
        [(8, 4), (631, 4), (631, 199), (8, 199)],
        [(120, 40), (320, 150), (500, 40), (320, 60)],
    ]
    lines = ['#pragma once', '', '#include <array>', '#include <cstdint>', '#include "graphics/raster.h"', '',
             'namespace corncob::tests {', '', 'struct native_polygon {',
             '  std::array<graphics::screen_point, 20> vertices;', '  unsigned int count;', '  uint64_t hash;', '};', '',
             f'inline std::array<native_polygon, {len(cases)}> const native_polygons{{']
    records = []
    for number, vertices in enumerate(cases):
        device[0] = Ega()
        native.mem_write(DATA_ADDRESS + 0xf840, b''.join(struct.pack('<2h', *point) for point in vertices))
        native.mem_write(DATA_ADDRESS + 0x2965, struct.pack('<H', len(vertices)))
        native.mem_write(DATA_ADDRESS + 0x1b8b, b'\x0c')
        native.mem_write(DATA_ADDRESS + 0x1b90, b'\0\0')
        native.reg_write(UC_X86_REG_SI, 0xf800)
        native.reg_write(UC_X86_REG_BX, 0xf840)
        native.reg_write(UC_X86_REG_SP, 0xffec)
        native.mem_write(0x7ffec, b'\0\0')
        native.emu_start(LOAD_ADDRESS + 0x68cc - 512, LOAD_ADDRESS, count=1000000)
        assert native.reg_read(UC_X86_REG_IP) == 0, 'Native polygon exceeded the instruction budget'
        pixels = device[0].pixels()
        value = checksum(pixels)
        lines += ['  native_polygon{', '    .vertices{']
        lines += [f'      graphics::screen_point{{{x}, {y}}},' for x, y in vertices]
        lines += ['    },', f'    .count{{{len(vertices)}}},', f'    .hash{{0x{value:016x}ull}},', '  },']
        records.append({'vertices': vertices, 'hash_fnv1a64': f'{value:016x}', 'ega_writes': device[0].writes})
    lines += ['};', '', '} // namespace corncob::tests', '']
    (ROOT / 'tests/reference/native_polygons.h').write_text('\n'.join(lines))
    (ROOT / 'tests/reference/native_polygons.json').write_text(json.dumps({'reference_sha256': EXPECTED_SHA256,
        'routine_file_offset': '0x68cc', 'adapter': 'Same EGA plane/write-mode adapter as the native disc fixtures',
        'viewport': window, 'colour': 12, 'background': 0, 'cases': records}, indent=2) + '\n')
    print('Recorded original EGA pixels for', len(cases), 'polygon cases', flush=True)


if __name__ == '__main__':
    main()
