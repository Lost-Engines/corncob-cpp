#!/usr/bin/env python3
"""Probe unchanged CCTITLE flyby/lettering motion, independently of drawing and DOS sound."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE, UC_HOOK_INSN
from unicorn.x86_const import (UC_X86_REG_DS, UC_X86_REG_SS, UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_INS_OUT)
from generate_native_reference import load, LOAD_ADDRESS
from title_video_reference import letter_masks

ROOT = Path(__file__).resolve().parents[1]
DS = LOAD_ADDRESS + 0x7410
STACK = DS
BP = 0xf000
HEADER = 0x800
FORMAT = '<BH6hB9hI'
# stage/frame/fade/intensity/scrolls/aircraft/passes/first-pass/saucer/explosion/mask/credits/PRNG
OFFSETS = (None, 0x1288, 0xe9e, None, 0x27d4, 0x27ce, 0x27f8, 0x127a, 0xd0,
           0x792, 0x796, 0x794, 0x798, 0x79c, 0x79a, 0x79e, 0xe9c, 0x27d6, 0x696)
SIZES = (1, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4)
STUBS = (0x3c41, 0x3ae4, 0x3ea3, 0x61ab, 0x61ff, 0x60b4, 0x6120,
         0x6337, 0x62c0, 0x62e9)


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/CCTITLE.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == '825f10a41d5d2963d83b6ad346c02aa081fbe1ef098728ef4feaba8af506e426'
    rng = random.Random(0x5449544c)
    output = bytearray(b'CORNTTL1' + struct.pack('<2I', 1024, 48))
    for i in range(1024):
        stage = i // 512
        values = [stage, rng.choice([0, 4, 5, 6, 65535, rng.randrange(65536)]), rng.choice([0, 255] if stage == 0 else [-1, 0, 255]),
                  rng.choice([0, 111, rng.randrange(112)]), rng.randrange(400), rng.randrange(288), rng.randrange(-490, 571), rng.randrange(3), rng.randrange(2),
                  rng.randrange(4), rng.randrange(100, 200), rng.randrange(-250, 400), rng.randrange(80), rng.randrange(300), rng.randrange(200), rng.randrange(3),
                  rng.choice([32, 64, 96, 128, 160, 192, 224]), rng.randrange(176), rng.randrange(1 << 32)]
        native = load(exe)
        native.reg_write(UC_X86_REG_DS, DS // 16)
        native.reg_write(UC_X86_REG_SS, DS // 16)
        native.reg_write(UC_X86_REG_BP, BP)
        native.reg_write(UC_X86_REG_SP, 0xd000)
        def put(offset, value, size=2, address=DS):
            native.mem_write(address + offset, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))
        for field, (offset, size) in enumerate(zip(OFFSETS, SIZES)):
            if offset is not None: put(offset, values[field], size)
        put(BP - 0x2e, values[3], address=STACK)
        put(BP - 0x2c, 255, address=STACK)
        put(BP - 0x29, 0, 1, address=STACK)
        for offset, value in [(0x1ca6, 16), (0x1278, -1), (0x27be, 4), (0x2800, 200), (0x2802, 308),
                              (0x27d0, 144), (0x27d2, 288), (0x27fc, 121), (0x27fe, 184), (0x27fa, 56),
                              (0x27f4, 11), (0x27f6, 16), (0x27e4, 69), (0x27e6, 160), (0x27e0, 64), (0x27e2, 72),
                              (0x27ec, 48), (0x27ee, 288), (0x27e8, 10), (0x27ea, 8), (0x27dc, 176), (0x27d8, 144), (0x27de, 320)]: put(offset, value)
        for k, width in enumerate([44, 40, 42, 46]):
            put(0xc00 + k * 4, width); put(0xc02 + k * 4, 35)
        for k, (width, height) in enumerate([(25, 21), (33, 26), (59, 37)]):
            put(0xc10 + k * 4, width); put(0xc12 + k * 4, height)
        for offset in STUBS: native.mem_write(LOAD_ADDRESS + offset - HEADER, b'\xcb')
        native.mem_write(LOAD_ADDRESS + 0x213a - HEADER, b'\x31\xc0\xcb')
        native.hook_add(UC_HOOK_INSN, lambda *_: None, None, 1, 0, UC_X86_INS_OUT)
        stopped = []
        def stop(uc, address, size, _):
            stopped.append(address - LOAD_ADDRESS + HEADER); uc.emu_stop()
        for offset in ([0x57a9, 0x57b2] if stage == 0 else [0x5d79, 0x5d82]):
            native.hook_add(UC_HOOK_CODE, stop, begin=LOAD_ADDRESS + offset - HEADER, end=LOAD_ADDRESS + offset - HEADER)
        native.emu_start(LOAD_ADDRESS + (0x52a8 if stage == 0 else 0x58bd) - HEADER, 0, count=500000)
        assert stopped, ('native frame did not finish', i)
        expected = values.copy()
        for field, (offset, size) in enumerate(zip(OFFSETS, SIZES)):
            if offset is not None: expected[field] = int.from_bytes(native.mem_read(DS + offset, size), 'little', signed=size == 2 and field != 1)
        expected[3] = int.from_bytes(native.mem_read(STACK + BP - 0x2e, 2), 'little', signed=True)
        output += struct.pack(FORMAT, *values) + struct.pack(FORMAT, *expected) + bytes([stopped[-1] in [0x57b2, 0x5d82]])
    output += letter_masks(exe, rng)
    (ROOT / 'tests/reference/title.bin').write_bytes(output)
    (ROOT / 'tests/reference/title.json').write_text(json.dumps({
        'reference_sha256': hashlib.sha256(exe).hexdigest(), 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'motion_cases': 1024, 'native_vga_rows': 48, 'routines': ['CCTITLE file 0x52a8..0x57a9 flyby', '0x58bd..0x5d79 lettering', '0x60b4 masked VGA write', '0x6120 thresholded masked VGA write'],
        'scope': 'Frame counters, motion, fades, credit scrolling, stage completion and complete Borland PRNG continuation. Drawing is not claimed pixel-equivalent by this fixture.',
        'stubs': [hex(o) + ' RETF: drawing or sound port boundary' for o in STUBS] + ['0x213a keyboard availability: return zero'],
        'vga_adapter': 'Unchained VGA plane selection, per-plane latches and original GC write masks; full 128-pixel rows, actual original OUT/MOVSB instructions, no stubs in mask routines.',
        'presentation_clock': 'Original CRTC page change does not wait for retrace. Native implementation uses a stable 70 Hz presentation clock; motion values retain original per-frame meaning.'
    }, indent=2) + '\n')
    print('Recorded 1024 original title motion passes', flush=True)


if __name__ == '__main__': main()
