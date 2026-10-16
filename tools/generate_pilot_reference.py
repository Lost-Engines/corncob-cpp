#!/usr/bin/env python3
"""Record the original Borland pilot-file masking stream without game files or DOS I/O."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_DS, UC_X86_REG_SP
from generate_native_reference import load, LOAD_ADDRESS

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/MOAG.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == '6d8c922d59a873073796b367a2a40993fcaa5e763ed2ca907a945981f0c5bc32'
    rng = random.Random(0x50494c4f)
    output = bytearray(b'CORNPLT1' + struct.pack('<I', 64))
    for i in range(64):
        seed = rng.randrange(65536)
        native = load(exe)
        native.reg_write(UC_X86_REG_DS, 0x3886)
        native.mem_write(0x38860 + 0x67a6, struct.pack('<I', seed))
        output += struct.pack('<HH', seed, 256)
        for _ in range(256):
            native.reg_write(UC_X86_REG_SP, 0xffe0)
            native.mem_write(0x7ffe0, struct.pack('<HH', 0, 0x2000))
            native.emu_start(LOAD_ADDRESS + 0xa82, LOAD_ADDRESS, count=1000)
            output.append(native.reg_read(UC_X86_REG_AX) // 128)
    (ROOT / 'tests/reference/pilots.bin').write_bytes(output)
    (ROOT / 'tests/reference/pilots.json').write_text(json.dumps({
        'reference_sha256': hashlib.sha256(exe).hexdigest(), 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'seeds': 64, 'mask_bytes_per_seed': 256,
        'routine': 'MOAG.EXE module offset 0x0a82 / file offset 0x3c82 Borland rand', 'stubs': []
    }, indent=2) + '\n')
    print('Recorded 16384 native pilot-file mask bytes')


if __name__ == '__main__':
    main()
