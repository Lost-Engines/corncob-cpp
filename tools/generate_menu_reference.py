#!/usr/bin/env python3
"""Hash RGB pixels from unchanged DOSBox menu captures, never from the C++ renderer.

First run corncob-reverse/tools/capture_native_menus.py. This development-only
step needs Pillow; builds and tests only read the committed text fixture.
"""
import argparse
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SCREENS = ('main', 'roster-empty', 'roster', 'records', 'record-statistics',
           'training', 'training-quality', 'training-counts',
           'training-information', 'joystick', 'score-menu', 'high-scores',
           'top-careers', 'fast-careers', 'documentation', 'documentation-one',
           'documentation-input', 'read-this',
           'combat-empty', 'new-theatre', 'combat', 'theatre-records')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--captures', type=Path, default=ROOT.parent / 'corncob-reverse/analysis/native-menus')
    args = parser.parse_args()
    lines = []
    for name in SCREENS:
        image = Image.open(args.captures / (name + '.png')).convert('RGB')
        if image.size != (640, 400):
            raise ValueError('unexpected original menu dimensions: ' + name)
        value = 0xcbf29ce484222325
        for component in image.tobytes():
            value = ((value ^ component) * 0x100000001b3) & 0xffffffffffffffff
        lines.append(f'{name} {value:016x}')
    (ROOT / 'tests/reference/menus.txt').write_text('\n'.join(lines) + '\n')
    header = (args.captures / 'theatre-header.bin').read_bytes()
    if len(header) != 48:
        raise ValueError('unexpected original theatre header length')
    (ROOT / 'tests/reference/menu_theatre_header.bin').write_bytes(header)


if __name__ == '__main__':
    main()
