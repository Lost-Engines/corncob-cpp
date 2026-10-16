#!/usr/bin/env python3
"""Capture unmodified original engine-pitch calculation and OPL note writes."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_INSN
from unicorn.x86_const import UC_X86_INS_IN, UC_X86_INS_OUT
from generate_native_reference import load, DATA_ADDRESS, LOAD_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    rng = random.Random(0x534f554e)
    output = bytearray(b'CORNSND1' + struct.pack('<I', 512))
    for i in range(512):
        native = load(exe)
        pressure, speed = rng.randrange(65536), rng.randrange(0, 16001)
        native.mem_write(DATA_ADDRESS + 0x1e5c, struct.pack('<H', pressure))
        native.mem_write(DATA_ADDRESS + 0x1d9c, struct.pack('<h', speed))
        native.mem_write(DATA_ADDRESS + 0xf5b0, b'\x08\0')
        address = [0]
        writes = []
        def out(uc, port, size, value, context):
            if port == 0x388:
                address[0] = value
            elif port == 0x389:
                writes.append((address[0], value))
        native.hook_add(UC_HOOK_INSN, out, None, 1, 0, UC_X86_INS_OUT)
        native.hook_add(UC_HOOK_INSN, lambda *args: 0, None, 1, 0, UC_X86_INS_IN)
        native.emu_start(LOAD_ADDRESS + 0x2fed - 512, LOAD_ADDRESS + 0x301c - 512, count=100000)
        assert len(writes) == 2 and [pair[0] for pair in writes] == [0xa0, 0xb0]
        output += struct.pack('<HH4B', pressure, speed, *(value for pair in writes for value in pair))
    (ROOT / 'tests/reference/audio.bin').write_bytes(output)
    (ROOT / 'tests/reference/audio.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'cases': 512, 'routines': ['0x2fed..0x301c engine pitch', '0xe0ce VOICEON', '0xe034 ADLWRT'],
        'stubs': ['IN returns zero; OUT captures actual OPL register/data writes'],
        'scope': 'Pressure/airflow pitch arithmetic, native clamp/wrap and exact A0/B0 register packing; not full soundscape timing'
    }, indent=2) + '\n')
    print('Captured 512 original engine-pitch/register pairs')


if __name__ == '__main__':
    main()
