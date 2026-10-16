#!/usr/bin/env python3
"""Execute original parked-aircraft placement, repair and boarding decisions and tower reset."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, LOAD_ADDRESS, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    source = exe[70800:]
    rng = random.Random(0x5041524b)
    output = bytearray(b'CORNPAR1' + struct.pack('<II', 512, 256))
    template = struct.unpack_from('<H', source, 0x2cca + 4)[0]
    for i in range(512):
        native = load(exe)
        record = bytearray(source[template:template + 74])
        base = tuple(rng.randrange(-2000000, 2000000) for _ in range(2)) + (rng.choice((720, 8000)),)
        attitude = tuple(rng.randrange(65536) for _ in range(3))
        position = tuple(base[k] + rng.randrange(-100000, 100000) for k in range(3))
        observer = tuple(position[k] + rng.randrange(-1500, 1500) for k in range(3)) if i % 2 else base
        flags = struct.unpack_from('<H', record, 24)[0] | rng.choice((0, 2, 4, 6, 0x100, 0x1000, 0x2000, 0x2100))
        flags_runtime = rng.choice((0, 1, 0x10, 0x11, 0xff))
        struct.pack_into('<3i3H', record, 0, *position, *(rng.randrange(65536) for _ in range(3)))
        struct.pack_into('<H', record, 24, flags)
        struct.pack_into('<H', record, 52, rng.randrange(65536))
        record[65] = flags_runtime
        placed = rng.randrange(8)
        resetting, new_arrival, on_foot, exiting = [rng.randrange(2) for _ in range(4)]
        native.mem_write(DATA_ADDRESS + 0xf800, bytes(record))
        native.mem_write(DATA_ADDRESS + 0x2a9, struct.pack('<3i3H', *base, *attitude))
        native.mem_write(DATA_ADDRESS + 0xb05, struct.pack('<3i', *observer))
        native.mem_write(DATA_ADDRESS + 0x2dce, struct.pack('<HB', placed, resetting))
        native.mem_write(DATA_ADDRESS + 0xf4c, bytes([new_arrival]))
        native.mem_write(DATA_ADDRESS + 0xafc, bytes([on_foot]))
        native.mem_write(DATA_ADDRESS + 0x19ec, bytes([exiting]))
        native.mem_write(DATA_ADDRESS + 0x1efa, bytes(2))
        boarded = []
        def board(uc, *_):
            boarded.append(bytes(uc.mem_read(DATA_ADDRESS + 0xf800, 74)))
        native.mem_write(LOAD_ADDRESS + 0x1e6a - 512, b'\xc3')
        native.hook_add(UC_HOOK_CODE, board, begin=LOAD_ADDRESS + 0x1e6a - 512, end=LOAD_ADDRESS + 0x1e6a - 512)
        native.reg_write(UC_X86_REG_SI, 0xf800)
        native.reg_write(UC_X86_REG_EFLAGS, 2)
        run(native, 0x96d6)
        output += struct.pack('<3i3H3iH4B', *base, *attitude, *observer, placed, resetting, new_arrival, on_foot, exiting)
        output += record + native.mem_read(DATA_ADDRESS + 0xf800, 74)
        output += native.mem_read(DATA_ADDRESS + 0x2dce, 2) + native.mem_read(DATA_ADDRESS + 0xf4c, 1) + bytes([bool(boarded)])
    for i in range(256):
        native = load(exe)
        observer = tuple(rng.randrange(-2000000, 2000000) for _ in range(3))
        span = (268800, 332800, 524288, 1500000)[i % 4]
        tower = tuple(observer[k] + rng.randrange(-span, span) for k in range(3))
        resetting, placed = rng.randrange(2), rng.randrange(65536)
        native.mem_write(DATA_ADDRESS + 0xf800, struct.pack('<3i', *tower))
        native.mem_write(DATA_ADDRESS + 0xb05, struct.pack('<3i', *observer))
        native.mem_write(DATA_ADDRESS + 0x2dce, struct.pack('<HB', placed, resetting))
        native.mem_write(DATA_ADDRESS + 0x1efa, bytes(2))
        native.reg_write(UC_X86_REG_SI, 0xf800)
        run(native, 0xa610)
        output += struct.pack('<6iBH', *tower, *observer, resetting, placed)
        output += native.mem_read(DATA_ADDRESS + 0x2dd0, 1) + native.mem_read(DATA_ADDRESS + 0x2dce, 2)
    destination = ROOT / 'tests/reference/parking.bin'
    destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'parked_aircraft_cases': 512, 'tower_cases': 256,
        'routines': ['0x96d6 OBJ5DATIM', '0x96ac PLACEPLANE', '0xa610 HOMEDETECT', '0x967e EVALDIST'],
        'stubs': ['0x1e6a GETINPLANE records a boarding request without entering the separate player-flight handoff'],
        'scope': 'Complete timed parked-aircraft handler including position/pose repair, all runtime-placement bits, '
                 'reset/exit gates, reserve/experimental/crashed variants, first-arrival runway departure, normal boarding '
                 'and updated placement counter. Tower near/far parking reset executes unchanged, with no stubs.',
    }, indent=2) + '\n')
    print('Recorded 512 parked-aircraft and 256 tower parking cases', flush=True)


if __name__ == '__main__':
    main()
