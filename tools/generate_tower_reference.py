#!/usr/bin/env python3
"""Record native tower/doorway painter ordering; development only."""
import hashlib
import json
import math
from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE, UC_HOOK_INSN, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_INS_OUT, UC_X86_REG_BX, UC_X86_REG_DX,
    UC_X86_REG_IP, UC_X86_REG_SI, UC_X86_REG_SP)
from generate_native_reference import load, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256
from generate_raster_reference import Ega, checksum

ROOT = Path(__file__).resolve().parents[1]
VIEWPORT = 0x1c26
CROP = (200, 4, 440, 345)


def invoke(native, offset):
    native.reg_write(UC_X86_REG_SP, 0xffec)
    native.mem_write(0x7ffec, b'\0\0')
    native.emu_start(LOAD_ADDRESS + offset - 512, LOAD_ADDRESS, count=2000000)
    assert native.reg_read(UC_X86_REG_IP) == 0, 'Native renderer exceeded its instruction budget'


def render(exe, position, heading, doorway=True, *, objects=None, painter_table=False):
    native = load(exe)
    device = Ega()
    device.planes[1][:] = b'\xff' * 65536
    native.hook_add(UC_HOOK_INSN, lambda uc, port, size, value, _: device.out(port, size, value),
                    None, 1, 0, UC_X86_INS_OUT)
    native.hook_add(UC_HOOK_MEM_READ, lambda uc, access, address, size, value, _: device.read(uc, address, size),
                    begin=0xa0000, end=0xaffff)
    native.hook_add(UC_HOOK_MEM_WRITE, lambda uc, access, address, size, value, _: device.write(address, size, value),
                    begin=0xa0000, end=0xaffff)
    native.mem_write(DATA_ADDRESS + 0xb05, struct.pack('<3i', *position))
    native.mem_write(DATA_ADDRESS + 0xf800, struct.pack('<3H', heading, 0, 0))
    native.reg_write(UC_X86_REG_BX, 0xf800)
    invoke(native, 0x5fe1)
    native.reg_write(UC_X86_REG_SI, 0x1fde)
    invoke(native, 0x6021)
    native.mem_write(DATA_ADDRESS + 0x1c3e, struct.pack('<HH', VIEWPORT, 0))
    native.mem_write(DATA_ADDRESS + 0x1efa, b'\0\0')

    addresses = []
    custom_objects = objects is not None
    if objects is None:
        objects = [(6, (0, 0, 16000)), (34, (0, -2000, 0))][:2 if doorway else 1]
    for index, (kind, origin) in enumerate(objects):
        address = 0xf900 + index * 80
        template = struct.unpack('<H', native.mem_read(DATA_ADDRESS + 0x2cca + kind * 2, 2))[0]
        record = bytearray(native.mem_read(DATA_ADDRESS + template, 74))
        struct.pack_into('<3i', record, 0, *origin)
        if custom_objects:
            # static scene probes disable timed/damage callbacks without changing the model or type properties
            struct.pack_into('<H', record, 24, 1)
        native.mem_write(DATA_ADDRESS + address, bytes(record))
        native.reg_write(UC_X86_REG_SI, address)
        invoke(native, 0x715a)
        addresses.append(address)

    native.mem_write(DATA_ADDRESS + 0xf880,
                     struct.pack('<HH', len(addresses), 10) + struct.pack('<' + 'H' * len(addresses), *addresses))
    native.reg_write(UC_X86_REG_SI, 0xf880)
    invoke(native, 0x74c1)
    addresses = struct.unpack('<' + 'H' * len(addresses), native.mem_read(DATA_ADDRESS + 0xf884, len(addresses) * 2))
    order = []
    def drawing(uc, address, size, context):
        record = uc.reg_read(UC_X86_REG_SI)
        order.append(struct.unpack('<H', uc.mem_read(DATA_ADDRESS + record + 72, 2))[0])
    native.hook_add(UC_HOOK_CODE, drawing,
                    begin=LOAD_ADDRESS + 0x7270 - 512, end=LOAD_ADDRESS + 0x7270 - 512)
    if painter_table:
        native.mem_write(DATA_ADDRESS + 0x2393, struct.pack('<H', 0xf880))
        native.reg_write(UC_X86_REG_DX, VIEWPORT)
        invoke(native, 0x7c8c)
    else:
        for address in addresses:
            native.reg_write(UC_X86_REG_SI, address)
            native.reg_write(UC_X86_REG_DX, VIEWPORT)
            invoke(native, 0x7270)
    pixels = device.pixels()
    left, top, right, bottom = CROP
    return bytes(pixels[y * 640 + x] for y in range(top, bottom) for x in range(left, right)), order


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    cases = []
    for octant in range(8):
        radians = octant * math.tau / 8
        position = (round(math.cos(radians) * 20000), round(math.sin(radians) * 20000), 720)
        heading = (octant * 8192 + 32768) & 65535
        without_door = None
        for doorway in (False, True):
            pixels, order = render(exe, position, heading, doorway)
            cases.append({'position': position, 'heading': heading, 'doorway': doorway,
                          'hash_fnv1a64': f'{checksum(pixels):016x}', 'draw_order': order})
            if doorway:
                cases[-1]['doorway_pixels'] = sum(a != b for a, b in zip(pixels, without_door))
            else:
                without_door = pixels

    lines = ['#pragma once', '', '#include <array>', '#include <cstdint>', '',
             '#include "graphics/raster.h"', '', 'namespace corncob::tests {', '',
             'inline graphics::clip_rectangle constexpr native_tower_crop{',
             f'  .left{{{CROP[0]}}},', f'  .top{{{CROP[1]}}},',
             f'  .right{{{CROP[2]}}},', f'  .bottom{{{CROP[3]}}},', '};', '',
             'struct native_tower_view {',
             '  int32_t x;', '  int32_t y;', '  uint16_t heading;', '  bool doorway;',
             '  uint64_t hash;', '};', '',
             f'inline std::array<native_tower_view, {len(cases)}> constexpr native_tower_views{{']
    for case in cases:
        x, y, _ = case['position']
        lines += ['  native_tower_view{', f'    .x{{{x}}},', f'    .y{{{y}}},',
                  f'    .heading{{{case["heading"]}}},', f'    .doorway{{{str(case["doorway"]).lower()}}},',
                  f'    .hash{{0x{case["hash_fnv1a64"]}ull}},', '  },']
    lines += ['};', '', '} // namespace corncob::tests', '']
    destination = ROOT / 'tests/reference/native_tower.h'
    destination.write_text('\n'.join(lines))
    destination.with_suffix('.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256,
        'fixture_sha256': hashlib.sha256(destination.read_bytes()).hexdigest(),
        'routine_file_offsets': {'CALCSIN': '0x5fe1', 'CALCMAT': '0x6021',
                                 'DISTEVAL': '0x715a', 'TBSORT': '0x74c1', 'D3DOBJ': '0x7270'},
        'adapter': 'Original EGA latches/write modes and native full-screen viewport; rear view disabled',
        'crop_left_top_right_bottom': CROP,
        'background': 2,
        'stubs': [],
        'scope': 'Two static original objects with their relative placements retained, viewed at eight ground-level angles. '
                 'Only the central scene region is compared; sky/ground normalize to the uniform EGA background. '
                 'The original renderer paints the doorway over the side/back walls. This is not a whole-game frame comparison.',
        'cases': cases,
    }, indent=2) + '\n')
    print('Recorded 16 native tower/doorway frames; the original draws the door last at all eight viewpoints', flush=True)


if __name__ == '__main__':
    main()
