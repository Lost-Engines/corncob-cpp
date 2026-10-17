#!/usr/bin/env python3
"""Preserve the Other Worlds distribution resources in one embedded data pack.

Development only. The application never needs the DOS install or this script.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--research', type=Path, default=ROOT.parent / 'corncob-reverse')
    args = parser.parse_args()
    install = args.research / 'installs/other_worlds_download'
    files = []
    for folder in ('DEF', 'IMG', 'VGA', 'DMO'):
        files += [(p.relative_to(install).as_posix().lower(), p.read_bytes())
                  for p in sorted((install / folder).iterdir()) if p.is_file()]
    for name in ('CORNCOB.DOC', 'MOAG.INF', 'MBUILDER.DOC', 'HINTS.TXT', 'PICT.DAT', '3.ADL', 'DEF3.ADL', 'BOSS.TXT', 'ROLFILES.TXT'):
        files.append((name.lower(), (install / name).read_bytes()))
    pack = bytearray(b'CORNUI01' + struct.pack('<H', len(files)))
    records = []
    for name, data in files:
        encoded = name.encode('ascii')
        pack += struct.pack('<BI', len(encoded), len(data)) + encoded + data
        records.append({'name': name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
    (ROOT / 'assets/other_worlds.dat').write_bytes(pack)
    metadata = {'reference': 'Other Worlds Campaign 3.42', 'source': 'installs/other_worlds_download',
                'flight_sha256': hashlib.sha256((install / '3.EXE').read_bytes()).hexdigest(),
                'menu_sha256': hashlib.sha256((install / 'MOAG.EXE').read_bytes()).hexdigest(),
                'files': records}
    (ROOT / 'assets/other_worlds_provenance.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(f'Embedded {len(files)} Other Worlds resources ({len(pack)} bytes)')


if __name__ == '__main__':
    main()
