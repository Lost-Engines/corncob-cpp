#!/usr/bin/env python3
"""Execute original DRAWDisc with a minimal EGA write-mode adapter; development only."""
import hashlib
import json
from pathlib import Path
import struct
from unicorn import UC_HOOK_INSN, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_INS_OUT, UC_X86_REG_SI, UC_X86_REG_SP, UC_X86_REG_IP
from generate_native_reference import load, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


class Ega:
    def __init__(self):
        self.planes = [bytearray(65536) for _ in range(4)]
        self.latches = [0] * 4
        self.registers = [0] * 9
        self.registers[8] = 255
        self.index = 0
        self.sequencer_index = 0
        self.map_mask = 15
        self.writes = 0

    def out(self, port, size, value):
        if port == 0x3c4:
            self.sequencer_index = value & 255
            if size == 2 and self.sequencer_index == 2:self.map_mask = (value >> 8) & 15
        elif port == 0x3c5 and self.sequencer_index == 2:self.map_mask = value & 15
        elif port == 0x3ce:
            self.index = value & 255
            if size == 2 and self.index < len(self.registers):
                self.registers[self.index] = (value >> 8) & 255
        elif port == 0x3cf and self.index < len(self.registers):
            self.registers[self.index] = value & 255

    def read(self, native, address, size):
        values = bytearray()
        for i in range(size):
            offset = address + i - 0xa0000
            self.latches = [plane[offset] for plane in self.planes]
            if self.registers[5] & 8:
                matches = 255
                for plane in range(4):
                    if self.registers[7] & (1 << plane):
                        matches &= self.latches[plane] if self.registers[2] & (1 << plane) else self.latches[plane] ^ 255
                values.append(matches)
            else:values.append(self.latches[self.registers[4] & 3])
        native.mem_write(address, bytes(values))

    def write(self, address, size, value):
        for i in range(size):
            offset = address + i - 0xa0000
            byte = (value >> (i * 8)) & 255
            mode = self.registers[5] & 3
            rotate = self.registers[3] & 7
            rotated = ((byte >> rotate) | (byte << (8 - rotate))) & 255
            for plane in range(4):
                if not self.map_mask & (1 << plane):continue
                mask = self.registers[8]
                if mode == 3:
                    mask &= rotated
                    result = 255 if self.registers[0] & (1 << plane) else 0
                elif mode == 2:
                    result = 255 if byte & (1 << plane) else 0
                elif mode == 0:
                    result = (255 if self.registers[0] & (1 << plane) else 0) if self.registers[1] & (1 << plane) else rotated
                else:
                    result = self.latches[plane]
                operation = (self.registers[3] >> 3) & 3
                if operation == 1:
                    result &= self.latches[plane]
                elif operation == 2:
                    result |= self.latches[plane]
                elif operation == 3:
                    result ^= self.latches[plane]
                self.planes[plane][offset] = ((result & mask) | (self.latches[plane] & (~mask & 255))) & 255
            self.writes += 1

    def pixels(self):
        output = bytearray(640 * 350)
        for i in range(len(output)):
            offset, bit = i // 8, 7 - i % 8
            output[i] = sum(((self.planes[plane][offset] >> bit) & 1) << plane for plane in range(4))
        return output


def checksum(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
    return value


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
    cases = [(320, 100, size) for size in range(17)]
    cases += [(320, 100, size) for size in (17, 18, 31, 64, 127, 500, 1023)]
    cases += [(8, 4, 16), (8, 4, 17), (7, 4, 16), (7, 4, 17), (630, 198, 16), (630, 198, 31)]
    lines = ['#pragma once', '', '#include <array>', '#include <cstdint>', '', 'namespace corncob::tests {', '',
             'struct native_disc {', '  int x;', '  int y;', '  int diameter;', '  uint64_t hash;', '};', '',
             f'inline std::array<native_disc, {len(cases)}> constexpr native_discs{{']
    records = []
    for x, y, diameter in cases:
        device[0] = Ega()
        native.mem_write(DATA_ADDRESS + 0x2082, struct.pack('<3h', x, y, diameter))
        native.mem_write(DATA_ADDRESS + 0x1b8b, b'\x0c')
        native.mem_write(DATA_ADDRESS + 0x1b90, b'\0\0')
        native.reg_write(UC_X86_REG_SI, 0xf800)
        native.reg_write(UC_X86_REG_SP, 0xffec)
        native.mem_write(0x7ffec, b'\0\0')
        native.emu_start(LOAD_ADDRESS + 0x6b45 - 512, LOAD_ADDRESS, count=1000000)
        assert native.reg_read(UC_X86_REG_IP) == 0, 'Native disc exceeded the instruction budget'
        value = checksum(device[0].pixels())
        lines += ['  native_disc{', f'    .x{{{x}}},', f'    .y{{{y}}},', f'    .diameter{{{diameter}}},', f'    .hash{{0x{value:016x}ull}},', '  },']
        records.append({'x': x, 'y': y, 'diameter': diameter, 'hash_fnv1a64': f'{value:016x}', 'ega_writes': device[0].writes})
    lines += ['};', '', '} // namespace corncob::tests', '']
    (ROOT / 'tests/reference/native_discs.h').write_text('\n'.join(lines))
    (ROOT / 'tests/reference/native_discs.json').write_text(json.dumps({'reference_sha256': EXPECTED_SHA256,
        'routine_file_offset': '0x6b45', 'adapter': 'EGA plane latches, write modes 0/2/3, masks and graphics-controller output',
        'viewport': window, 'colour': 12, 'background': 0, 'cases': records}, indent=2) + '\n')
    print('Recorded original EGA pixels for', len(cases), 'disc cases', flush=True)


if __name__ == '__main__':
    main()
