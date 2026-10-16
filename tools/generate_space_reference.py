#!/usr/bin/env python3
"""Record star initialization/projection from unchanged normal 3.42 SRATS.EXE."""
import hashlib
import json
from pathlib import Path
import struct
from unicorn import UC_HOOK_INSN, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_INS_OUT, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS, UC_X86_REG_SP
from generate_native_reference import load, LOAD_ADDRESS, LOAD_SEGMENT
from generate_raster_reference import Ega, checksum

ROOT = Path(__file__).resolve().parents[1]
SHA256 = '6ab6c0c7f6d772cc82db51067b8a66ac1841bc82688dab34e3643c77fe60dc3a'


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/SRATS.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == SHA256
    native = load(exe)
    data_segment = LOAD_SEGMENT + 0xc1
    data_address = data_segment * 16
    for reg in (UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS):
        native.reg_write(reg, data_segment)
    device = [Ega()]
    native.hook_add(UC_HOOK_INSN, lambda uc, port, size, value, _: device[0].out(port, size, value), None, 1, 0, UC_X86_INS_OUT)
    native.hook_add(UC_HOOK_MEM_READ, lambda uc, access, address, size, value, _: device[0].read(uc, address, size), begin=0xa0000, end=0xaffff)
    native.hook_add(UC_HOOK_MEM_WRITE, lambda uc, access, address, size, value, _: device[0].write(address, size, value), begin=0xa0000, end=0xaffff)

    def run(offset):
        native.reg_write(UC_X86_REG_SP, 0xffec)
        native.mem_write(data_address + 0xffec, b'\0\0')
        native.emu_start(LOAD_ADDRESS + offset - 512, LOAD_ADDRESS, count=2000000)

    run(0x7c6)
    output = bytearray(b'CORNSTA1' + struct.pack('<II', 768, 64))
    for i in range(768):
        output += native.mem_read(data_address + 0x2c8 + i * 10, 6)
    hashes = []
    for frame in range(64):
        device[0] = Ega()
        run(0x7d6)
        value = checksum(device[0].pixels())
        output += struct.pack('<Q', value)
        hashes.append(f'{value:016x}')
    (ROOT / 'tests/reference/space.bin').write_bytes(output)
    (ROOT / 'tests/reference/space.json').write_text(json.dumps({
        'reference_sha256': SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'routines': {'RANMIL': '0x7c6', 'RANSTR': '0x77b', 'EPSTR': '0x7d6'},
        'star_count': 768, 'projected_frames': 64, 'pixel_hashes_fnv1a64': hashes,
        'adapter': 'EGA plane latches, write mode 2 and graphics-controller output', 'stubs': [],
        'scope': 'Original star initialization and indexed projection, excluding cockpit/window fills and BIOS timing'
    }, indent=2) + '\n')
    print('Recorded 768 original stars and 64 original starfield frames')


if __name__ == '__main__':
    main()
