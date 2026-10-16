#!/usr/bin/env python3
"""Hash preserved original DOS campaign screens; never use native C++ output.

The sibling research tree records preparation and capture sequences in
analysis/airfield-campaign.md. Return and award screens can be recaptured with
its tools/capture_native_return.py. Pillow is only needed to regenerate fixtures.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SCREENS = ('preliminary', 'starting', 'survey', 'reserve', 'populated-scores',
           'populated-careers', 'populated-fastest', 'mission-report',
           'decoration', 'post-mission', 'promotion')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--captures', type=Path, default=ROOT.parent / 'corncob-reverse/analysis/native-campaign')
    args = parser.parse_args()
    lines = []
    manifest = []
    for name in SCREENS:
        image = Image.open(args.captures / (name + '.png')).convert('RGB')
        if image.size != (640, 400):
            raise ValueError('unexpected original screen dimensions: ' + name)
        pixels = image.tobytes()
        value = 0xcbf29ce484222325
        for component in pixels:
            value = ((value ^ component) * 0x100000001b3) & 0xffffffffffffffff
        lines.append(f'{name} {value:016x}')
        manifest.append({'name': name, 'rgb_sha256': hashlib.sha256(pixels).hexdigest()})
    target = ROOT / 'tests/reference'
    (target / 'campaign_menus.txt').write_text('\n'.join(lines) + '\n')
    (target / 'campaign_menus.json').write_text(json.dumps({
        'moag_sha256': '6d8c922d59a873073796b367a2a40993fcaa5e763ed2ca907a945981f0c5bc32',
        'screens': manifest,
        'scope': 'Unchanged MOAG/ISCORE rendering with controlled original-format input files; no original flight was simulated for the synthetic award case.',
    }, indent=2) + '\n')


if __name__ == '__main__':
    main()
