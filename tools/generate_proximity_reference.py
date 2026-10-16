#!/usr/bin/env python3
"""Record ordered native actor proximity outputs with real radar range calculations."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    source = exe[70800:]
    randomizer = random.Random(0x50524f58)
    contents = bytearray(b'CORNPRX1' + struct.pack('<I', 384))
    for case in range(384):
        native = load(exe)
        observer = tuple(randomizer.randrange(-1000000, 1000000) for _ in range(2)) + (720,)
        on_foot = case % 2
        target = tuple(observer[k] + randomizer.randrange(-200000, 200000) for k in range(3)) if on_foot else observer
        native.mem_write(DATA_ADDRESS + 0xb05, struct.pack('<3i', *observer))
        native.mem_write(DATA_ADDRESS + 0xe8b, struct.pack('<3i', *target))
        native.mem_write(DATA_ADDRESS + 0x5bb0, struct.pack('<3i', *target))
        native.mem_write(DATA_ADDRESS + 0x2a9, struct.pack('<3i', *target))
        native.mem_write(DATA_ADDRESS + 0xafc, bytes([on_foot]))
        native.mem_write(DATA_ADDRESS + 0x1e5e, struct.pack('<H', 5))
        native.mem_write(DATA_ADDRESS + 0x1b7f, struct.pack('<H', 5))
        native.mem_write(DATA_ADDRESS + 0xf0c9, struct.pack('<HHBHBHB', 256, 0, 255, 64, 0, 63, 128))
        native.mem_write(DATA_ADDRESS + 0xec28, struct.pack('<H', 0xf400))
        native.mem_write(DATA_ADDRESS + 0x1efa, bytes(2))
        for offset in (0xce3b, 0xaf8e, 0xb1a2, 0xc4e9):
            native.mem_write(LOAD_ADDRESS + offset - 512, b'\xc3')
        kinds = [0, 24, 50] if case % 3 else [0, 43, 24, 24, 53]
        randomizer.shuffle(kinds)
        records = []
        for index, kind in enumerate(kinds):
            template = struct.unpack_from('<H', source, 0x2cca + kind * 2)[0]
            record = bytearray(source[template:template + 74])
            span = randomizer.choice((2000, 20000, 100000, 600000))
            position = tuple(observer[k] + randomizer.randrange(-span, span + 1) for k in range(3))
            struct.pack_into('<3i', record, 0, *position)
            flags = struct.unpack_from('<H', record, 24)[0] | (randomizer.randrange(8) << 12)
            if case % 7 == 0:
                flags &= ~8
            struct.pack_into('<H', record, 24, flags)
            record[27] = randomizer.randrange(1, 51)
            struct.pack_into('<H', record, 52, randomizer.choice((23900, 23950, 23800)) if kind == 24 else 32000)
            records.append(bytes(record))
            native.mem_write(DATA_ADDRESS + 0xf800 + index * 80, bytes(record))
        for index, kind in enumerate(kinds):
            native.reg_write(UC_X86_REG_SI, 0xf800 + index * 80)
            native.reg_write(UC_X86_REG_DI, 0)
            native.reg_write(UC_X86_REG_EFLAGS, 2)
            run(native, {0: 0x8dbc, 43: 0xbb97, 24: 0xabd2, 50: 0xc102, 53: 0xc102}[kind])
        contents += struct.pack('<6iBB', *observer, *target, on_foot, len(records)) + b''.join(records)
        for offset, size in ((0xf0d1, 2), (0xf0ce, 2), (0xf0d0, 1), (0xf0c9, 2), (0xf0cd, 1)):
            contents += native.mem_read(DATA_ADDRESS + offset, size)
    destination = ROOT / 'tests/reference/proximity.bin'
    destination.write_bytes(contents)
    destination.with_suffix('.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256,
        'fixture_sha256': hashlib.sha256(contents).hexdigest(),
        'cases': 384,
        'routines': ['0x8dbc orb', '0xbb97 destroyed orb', '0xabd2 KLA', '0xc102 saucer', '0xab2b RADARPROCESS'],
        'stubs': ['RADARDOT omits pixels only; bomb/KLA/virtual-AAA constructors return without creating children'],
        'scope': 'Ordered proximity state from complete original handlers, including live radar magnitude/compression, '
                 'separate pilot and aircraft targets, KLA launch rest, dead saucers and pre-repair health frequency. '
                 'Generic movement is excluded; state begins with the native INITNUMBERS defaults.',
    }, indent=2) + '\n')
    print('Recorded 384 ordered original actor-proximity passes', flush=True)


if __name__ == '__main__':
    main()
