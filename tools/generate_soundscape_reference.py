#!/usr/bin/env python3
"""Execute the original orb/saucer/KLA OPL channel block across a connected sequence."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_INSN
from unicorn.x86_const import UC_X86_INS_IN, UC_X86_INS_OUT, UC_X86_REG_IP
from generate_native_reference import load, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    install = ROOT.parent / 'corncob-reverse/installs/deluxe_dnk'
    exe = (install / '3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    native = load(exe)
    bank = (install / '3.ADL').read_bytes()
    patches = []
    patch = bytearray()
    for line in bank.decode('ascii').splitlines():
        if not line:
            continue
        if line == 'ff':
            patch.append(255)
            assert len(patch) <= 29
            patches.append(bytes(patch))
            patch.clear()
        else:
            patch.extend(int(v, 16) for v in line.split(','))
    assert len(patches) == 12
    for index, patch in enumerate(patches):
        native.mem_write(DATA_ADDRESS + 0xf5bd + index * 29, patch)
    native.mem_write(DATA_ADDRESS + 0xf5b0, struct.pack('<H', 8))
    native.mem_write(DATA_ADDRESS + 0x19ba, bytes(3))
    native.mem_write(DATA_ADDRESS + 0x1d6, struct.pack('<HH', 64, 0))
    register = [0]
    writes = []
    def output(uc, port, size, value, context):
        if port == 0x388:
            register[0] = value
        elif port == 0x389:
            writes.append((register[0], value))
    native.hook_add(UC_HOOK_INSN, output, None, 1, 0, UC_X86_INS_OUT)
    native.hook_add(UC_HOOK_INSN, lambda *args: 0, None, 1, 0, UC_X86_INS_IN)

    def advance(orb, orb_frequency, kla, quality, saucer, saucer_frequency):
        native.mem_write(DATA_ADDRESS + 0xf0d1, struct.pack('<HB', orb, orb_frequency))
        native.mem_write(DATA_ADDRESS + 0xf0ce, struct.pack('<HB', kla, quality))
        native.mem_write(DATA_ADDRESS + 0xf0c9, struct.pack('<H', saucer))
        native.mem_write(DATA_ADDRESS + 0xf0cd, bytes([saucer_frequency]))
        writes.clear()
        native.emu_start(LOAD_ADDRESS + 0x2cd5 - 512, LOAD_ADDRESS + 0x2e26 - 512, count=100000)
        assert native.reg_read(UC_X86_REG_IP) == 0x2e26 - 512
        return list(writes)

    advance(63, 128, 64, 0, 256, 255)
    randomizer = random.Random(0x53434150)
    cases = []
    for index in range(512):
        # repeatedly enter and leave both shared-channel modes, including a direct orb-to-saucer handoff
        orb = randomizer.choice((0, 12, 13, 14, 62, 63, 64, 200))
        saucer = randomizer.choice((0, 1, 63, 127, 254, 255, 256))
        if index % 4 == 0:
            orb, saucer = 1, 0
        elif index % 4 == 1:
            orb, saucer = 63, 127
        elif index % 4 == 2:
            orb, saucer = 63, 256
        cases.append((orb, randomizer.randrange(256), randomizer.choice((0, 7, 8, 9, 63, 64, 65)),
                      randomizer.randrange(8), saucer, randomizer.randrange(256)))
    contents = bytearray(b'CORNSFX1' + struct.pack('<I', len(cases)))
    register_count = 0
    for values in cases:
        captured = advance(*values)
        contents += struct.pack('<HBHBHBH', *values, len(captured))
        contents += bytes(value for entry in captured for value in entry)
        register_count += len(captured)
    destination = ROOT / 'tests/reference/soundscape.bin'
    destination.write_bytes(contents)
    destination.with_suffix('.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256,
        'fixture_sha256': hashlib.sha256(contents).hexdigest(),
        'sound_bank_sha256': hashlib.sha256(bank).hexdigest(),
        'routine_file_offsets': '0x2cd5..0x2e26',
        'frames': len(cases),
        'register_writes': register_count,
        'stubs': ['IN returns zero; OUT records OPL address/data writes'],
        'scope': 'Connected original orb/saucer channel priority, complete patch transitions, note packing, '
                 'distance attenuation, persistent KLA pitch smoothing and on/off transitions. '
                 'The supplied ADL bank is loaded into the original 29-byte slots as READSOUNDS would. '
                 'A default proximity frame primes both implementations before comparisons. No sound logic is replaced.',
    }, indent=2) + '\n')
    print('Recorded', len(cases), 'connected native proximity frames and', register_count, 'OPL writes', flush=True)


if __name__ == '__main__':
    main()
