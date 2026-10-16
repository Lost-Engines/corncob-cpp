#!/usr/bin/env python3
"""Record the original ISCORE calculation, including signed intermediate wrap cases."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_DX, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SP
from generate_native_reference import load, LOAD_ADDRESS

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/ISCORE.EXE').read_bytes()
    header = struct.unpack_from('<H', exe, 8)[0] * 16
    rng = random.Random(0x53434f52)
    output = bytearray(b'CORNSCR1' + struct.pack('<I', 512))
    for i in range(512):
        stats = bytearray(struct.pack('<65H', *(rng.randrange(0, 300 if i < 256 else 65536) for _ in range(65))))
        struct.pack_into('<H', stats, 0, rng.randrange(0, 65536))
        struct.pack_into('<H', stats, 4, rng.randrange(0, 20) if i < 256 else rng.randrange(0, 65535))
        native = load(exe)
        ds_segment = 0x2000 + 0x5ce
        native.reg_write(UC_X86_REG_DS, ds_segment)
        native.reg_write(UC_X86_REG_ES, ds_segment)
        native.mem_write(ds_segment * 16 + 0xd000, bytes(stats))
        native.reg_write(UC_X86_REG_SP, 0xffe0)
        native.mem_write(0x7ffe0, struct.pack('<3H', 0, 0x2000, 0xd000))
        native.emu_start(LOAD_ADDRESS + 0x4698 - header, LOAD_ADDRESS, count=50000)
        output += stats + struct.pack('<HH', native.reg_read(UC_X86_REG_AX), native.reg_read(UC_X86_REG_DX))
    (ROOT / 'tests/reference/score.bin').write_bytes(output)
    (ROOT / 'tests/reference/score.json').write_text(json.dumps({
        'reference_sha256': hashlib.sha256(exe).hexdigest(), 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'cases': 512, 'routine': 'ISCORE.EXE file 0x4698', 'stubs': [],
        'scope': 'Full native score helper with unchanged Borland multiplication and signed division; ordinary and wrapping statistics'
    }, indent=2) + '\n')
    print('Recorded 512 native sortie scores')


if __name__ == '__main__':
    main()
