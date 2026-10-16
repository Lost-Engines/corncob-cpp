#!/usr/bin/env python3
"""Record the original DRAWNEAR runway pass and ordinary distance ordering."""
import hashlib
import json
import math
from pathlib import Path
import struct
from generate_native_reference import EXPECTED_SHA256
from generate_raster_reference import checksum
from generate_tower_reference import render, CROP

ROOT = Path(__file__).resolve().parents[1]


def main():
    exe = (ROOT.parent / 'corncob-reverse/installs/deluxe_dnk/3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXPECTED_SHA256
    cases = []
    output = bytearray(b'CORNSCN1' + struct.pack('<I', 48))
    for kind in (5, 35, 36):
        for altitude in (0, 8000):
            for octant in range(8):
                radians = octant * math.tau / 8
                position = (round(math.cos(radians) * 20000), round(math.sin(radians) * 20000), 720)
                heading = (octant * 8192 + 32768) & 65535
                objects = [(6, (0, 0, 16000)), (kind, (0, 0, altitude))]
                pixels, order = render(exe, position, heading, objects=objects, painter_table=True)
                expected = checksum(pixels)
                output += struct.pack('<H3iHiQ', kind, *position, heading, altitude, expected)
                cases.append({'objects': objects, 'position': position, 'heading': heading,
                              'draw_order': order, 'hash_fnv1a64': f'{expected:016x}'})
    destination = ROOT / 'tests/reference/scene_order.bin'
    destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({
        'reference_sha256': EXPECTED_SHA256,
        'fixture_sha256': hashlib.sha256(output).hexdigest(),
        'routines': ['0x7c8c DRAWNEAR', '0x7c69 CHECKFORRUNWAY', '0x7270 D3DOBJ', '0x74c1 TBSORT'],
        'stubs': [],
        'adapter': 'Original EGA controller/planes, uniform green clear, native full-screen viewport',
        'scope': 'Three original runway types at ground level and elevated, viewed from eight directions. '
                 'The original painter routine runs intact. Static probe objects have flags=1, disabling '
                 'their timed and damage callbacks; geometry and type properties remain original. '
                 'Compare only the central region with sky/ground normalized to the clear colour.',
        'crop_left_top_right_bottom': CROP,
        'cases': cases,
    }, indent=2) + '\n')
    print('Recorded 48 native DRAWNEAR runway/ordinary passes', flush=True)


if __name__ == '__main__':
    main()
