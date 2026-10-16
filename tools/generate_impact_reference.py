#!/usr/bin/env python3
"""Execute original destruction callbacks, retaining emitted effects, score words and PRNG state."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE, UC_HOOK_INSN
from unicorn.x86_const import (UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_EFLAGS,
    UC_X86_REG_SP, UC_X86_REG_IP, UC_X86_INS_IN, UC_X86_INS_OUT)
from generate_native_reference import load, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    data = exe[70800:]
    rng = random.Random(0x494d5043)
    kinds = (0, 1, 2, 3, 6, 7, 11, 12, 13, 14, 15, 16, 17, 20, 21, 22, 23, 24, 25, 26, 27, 30, 34, 42, 48, 49, 50, 51, 52, 53, 57)
    singles, pairs = len(kinds) * 32, 128
    cases = singles + pairs
    output = bytearray(b'CORNIMP2' + struct.pack('<I', cases))
    children = 0
    for case in range(cases):
        native = load(exe)
        paired = case >= singles
        kind = kinds[case % len(kinds)]
        template = struct.unpack_from('<H', data, 0x2cca + kind * 2)[0]
        record = bytearray(data[template:template + 74])
        struct.pack_into('<3i3H', record, 0, rng.randrange(-3000000, 3000000), rng.randrange(-3000000, 3000000),
                         rng.choice((-1, 0, 1000, 65530, 100000)), *(rng.randrange(65536) for _ in range(3)))
        record[18] = 0 if max(abs(p) for p in struct.unpack_from('<3i', record)) < 32768 else 1 if max(abs(p) for p in struct.unpack_from('<3i', record)) < 524288 else 2
        flags = struct.unpack_from('<H', record, 24)[0] | 8 | (rng.randrange(8) << 12)
        if rng.randrange(3) == 0: flags |= 0x800
        if kind == 7 and case // len(kinds) % 8 == 0: flags |= 0x1000
        struct.pack_into('<H', record, 24, flags)
        if kind in (25, 26): record[19] = rng.randrange(4)
        record[27] = rng.choice((1, 6, 25, 26, 65, 127, 128, 255))
        struct.pack_into('<3h', record, 46, *(rng.randrange(-1500, 1500) for _ in range(3)))
        attacker_kind = rng.choice((30, 21, 20) if paired else (30, 21, 20, 32, 15, 12, 19, 28))
        attacker_template = struct.unpack_from('<H', data, 0x2cca + attacker_kind * 2)[0]
        attacker = bytearray(data[attacker_template:attacker_template + 74])
        struct.pack_into('<H', attacker, 24, 9 | rng.choice((0, 0x40, 0x80, 0x800, 0x880, 2)))
        attacker[19] = rng.randrange(4)
        if paired:
            record[19], attacker[19] = 2, 0
            attacker[:12] = record[:12]
            struct.pack_into('<3h', record, 66, 2000, 2000, 2000)
            struct.pack_into('<3h', attacker, 66, 2000, 2000, 2000)
        seed = rng.randrange(4294967296)
        elapsed = rng.randrange(2, 16)
        vehicle_rest = rng.choice((150, 200, 300))
        native.mem_write(DATA_ADDRESS + 0xf800, bytes(record))
        native.mem_write(DATA_ADDRESS + 0xf880, bytes(attacker))
        native.mem_write(DATA_ADDRESS + 0xf722, struct.pack('<I', seed))
        native.mem_write(LOAD_ADDRESS + 0x43, struct.pack('<I', seed))
        native.mem_write(DATA_ADDRESS + 0x1e4, bytes(130))
        native.mem_write(DATA_ADDRESS + 0xb05, bytes(12))
        native.mem_write(DATA_ADDRESS + 0x1e5e, struct.pack('<H', elapsed))
        native.mem_write(DATA_ADDRESS + 0x1b7f, struct.pack('<H', 5))
        native.mem_write(DATA_ADDRESS + 0xf0f2, struct.pack('<H', vehicle_rest))
        native.mem_write(DATA_ADDRESS + 0xf5b0, struct.pack('<H', 8))
        native.hook_add(UC_HOOK_INSN, lambda *_: 0, None, 1, 0, UC_X86_INS_IN)
        native.hook_add(UC_HOOK_INSN, lambda *_: None, None, 1, 0, UC_X86_INS_OUT)
        allocated = []
        def allocate(uc, address, size, _):
            template = struct.unpack('<H', uc.mem_read(DATA_ADDRESS + 0xec28, 2))[0]
            slot = 0x9000 + 74 * len(allocated)
            assert slot < 0xe000
            uc.mem_write(DATA_ADDRESS + slot, bytes(uc.mem_read(DATA_ADDRESS + template, 74)))
            allocated.append(slot)
            uc.reg_write(UC_X86_REG_SI, slot)
            uc.reg_write(UC_X86_REG_EFLAGS, uc.reg_read(UC_X86_REG_EFLAGS) & ~1)
            sp = uc.reg_read(UC_X86_REG_SP)
            ret = struct.unpack('<H', uc.mem_read(0x70000 + sp, 2))[0]
            uc.reg_write(UC_X86_REG_SP, sp + 2)
            uc.reg_write(UC_X86_REG_IP, ret)
        native.hook_add(UC_HOOK_CODE, allocate, begin=LOAD_ADDRESS + 0x9fdb - 512, end=LOAD_ADDRESS + 0x9fdb - 512)
        routine = struct.unpack_from('<H', data, 0x2c50 + kind * 2)[0] + 512
        if paired: routine = 0x755d
        native.reg_write(UC_X86_REG_SI, 0xf800)
        native.reg_write(UC_X86_REG_DI, 0xf880)
        native.reg_write(UC_X86_REG_EFLAGS, 3)
        native.reg_write(UC_X86_REG_SP, 0xffec)
        native.mem_write(0x7ffec, b'\0\0')
        native.emu_start(LOAD_ADDRESS + routine - 512, LOAD_ADDRESS, count=2000000)
        assert native.reg_read(UC_X86_REG_IP) == 0, (case, kind, hex(native.reg_read(UC_X86_REG_IP)))
        output += struct.pack('<IHHB', seed, elapsed, vehicle_rest, paired) + record + attacker
        output += native.mem_read(DATA_ADDRESS + 0xf800, 74) + native.mem_read(DATA_ADDRESS + 0xf880, 74)
        output += native.mem_read(DATA_ADDRESS + 0x1e4, 130)
        output += native.mem_read(DATA_ADDRESS + 0xf722, 4)
        output += struct.pack('<H', len(allocated))
        for slot in allocated: output += native.mem_read(DATA_ADDRESS + slot, 74)
        children += len(allocated)
    destination = ROOT / 'tests/reference/impacts.bin'
    destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'cases': cases, 'single_callbacks': singles, 'paired_contacts': pairs, 'object_types': list(kinds), 'constructed_effects': children,
        'routines': ['Original type callback table at DS:0x2c50', 'Full radar/tower callbacks and shared FDEST hit path',
                     '0x755d SLAM paired contact callbacks', 'Original STRTFLT, STRTSHARD, STRTBOOM, RAND_W and quality-score helpers'],
        'stubs': ['0x9fdb EXPSTRT supplies independent empty slots and copies the requested native template'],
        'scope': 'Complete hit callbacks with accepted/rejected weapons, surviving and fatal health, '
                 'full object state/models, all 130 score bytes, every constructed effect and complete random continuation. '
                 'Paired SLAM cases retain mutations from the first callback as inputs to the second; radar heading uses the supplied fast tick word. No collision detection, painter dispatch or player-aircraft impact sentinel is supplied.',
    }, indent=2) + '\n')
    print(f'Recorded {cases} native impact handlers and {children} emitted effects', flush=True)


if __name__ == '__main__':
    main()
