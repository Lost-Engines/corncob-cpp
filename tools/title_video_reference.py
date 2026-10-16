"""VGA plane adapter for unchanged CCTITLE letter-mask routines; development only."""
import struct
from unicorn import UC_HOOK_INSN, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_INS_OUT, UC_X86_REG_SP
from generate_native_reference import load, LOAD_ADDRESS
from generate_raster_reference import Ega


class ModeX(Ega):
    def __init__(self):
        super().__init__()
        self.map_mask = 15
        self.sequence_index = 0

    def out(self, port, size, value):
        if port == 0x3c4:
            self.sequence_index = value & 255
            if size == 2 and self.sequence_index == 2: self.map_mask = (value >> 8) & 15
        elif port == 0x3c5 and self.sequence_index == 2: self.map_mask = value & 15
        else: super().out(port, size, value)

    def write(self, address, size, value):
        offset = address - 0xa0000
        previous = [bytes(plane[offset:offset + size]) for plane in self.planes]
        super().write(address, size, value)
        for i, plane in enumerate(self.planes):
            if not (self.map_mask & (1 << i)): plane[offset:offset + size] = previous[i]


def letter_masks(exe, rng):
    result = bytearray()
    for case in range(48):
        native = load(exe)
        device = ModeX()
        native.hook_add(UC_HOOK_INSN, lambda uc, port, size, value, _: device.out(port, size, value), None, 1, 0, UC_X86_INS_OUT)
        native.hook_add(UC_HOOK_MEM_READ, lambda uc, access, address, size, value, _: device.read(uc, address, size), begin=0xa0000, end=0xaffff)
        native.hook_add(UC_HOOK_MEM_WRITE, lambda uc, access, address, size, value, _: device.write(address, size, value), begin=0xa0000, end=0xaffff)
        cloud = bytes(rng.randrange(256) for _ in range(128))
        stencil = bytes(rng.randrange(8) * 32 for _ in range(128))
        limit = 255 if case < 8 else rng.randrange(256)
        planar = bytearray()
        for plane in range(4):
            device.planes[plane][0x4000:0x4020] = cloud[plane::4]
            planar.extend(stencil[plane::4])
        native.mem_write(0x60000, bytes(planar))
        native.reg_write(UC_X86_REG_SP, 0xffe0)
        native.mem_write(0x7ffe0, struct.pack('<7H', 0, 0x2000, 32, 0x4000, 0, 0x6000, limit))
        offset = 0x60b4 if case < 8 else 0x6120
        native.emu_start(LOAD_ADDRESS + offset - 0x800, LOAD_ADDRESS, count=10000)
        actual = bytes(device.planes[i % 4][0x4000 + i // 4] for i in range(128))
        result += struct.pack('<BH', limit, 128) + cloud + stencil + actual
    return result
