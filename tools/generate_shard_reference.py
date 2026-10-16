#!/usr/bin/env python3
"""Record the original per-slot airborne shard flame colours and random continuation."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_SI, UC_X86_REG_EFLAGS
from generate_native_reference import load, run, DATA_ADDRESS, EXPECTED_SHA256

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    source = exe[70800:]
    rng = random.Random(0x53485244)
    output = bytearray(b'CORNSHR1' + struct.pack('<I', 512))
    for i in range(512):
        native = load(exe)
        kind, slot = 8 + i % 2, rng.randrange(320)
        template = struct.unpack_from('<H', source, 0x2cca + kind * 2)[0]
        record = bytearray(source[template:template + 74])
        struct.pack_into('<3i', record, 0, 0, 0, rng.choice((0, 1, 1000, 65535, 65536, -65536)))
        seed = rng.randrange(4294967296)
        address = 0x8fa6 + slot * 74
        native.mem_write(DATA_ADDRESS + address, bytes(record))
        native.mem_write(DATA_ADDRESS + 0xf722, struct.pack('<I', seed))
        native.reg_write(UC_X86_REG_SI, address)
        native.reg_write(UC_X86_REG_EFLAGS, 2)
        run(native, 0xa511)
        output += struct.pack('<HI', slot, seed) + record
        output += native.mem_read(DATA_ADDRESS + 0x71cf, 2) + native.mem_read(DATA_ADDRESS + 0xf722, 4)
    destination = ROOT / 'tests/reference/shards.bin'
    destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256, 'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'cases': 512, 'routine': '0xa511 SHARDTIM', 'stubs': [],
        'scope': 'Original pool-slot modulo, low-word ground test, red/yellow airborne flicker, black settled shards and complete PRNG continuation. Generic movement is excluded.',
    }, indent=2) + '\n')
    print('Recorded 512 native shard colour passes', flush=True)


if __name__ == '__main__':
    main()
