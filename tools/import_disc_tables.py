#!/usr/bin/env python3
"""Regenerate original raster tables from the preserved source; development only."""
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]


def main():
    research = ROOT.parent / 'corncob-reverse'
    exe = (research / 'installs/deluxe_dnk/3.EXE').read_bytes()
    source = (research / 'analysis/original_source/F3DVEC.ASM').read_text()
    rows, small = [], []
    blob = bytearray()
    pattern = r'^cir(\d+)\s+dw\s+(\d+)\s*\n((?:\s*dw[^\n]*\n)+)'
    for diameter, count, block in re.findall(pattern, source, re.M):
        entries = []
        assert int(diameter) == len(small)
        for line in block.splitlines():
            dy, mask = re.match(r'\s*dw\s+([^,]+),\s*([0-9a-f]+h)', line, re.I).groups()
            dy = dy.strip()
            if 'lconst' in dy:
                factor = dy.replace('lconst', '').replace('*', '').strip()
                dy = -1 if factor == '-' else 1 if factor in ('', '+') else int(factor)
            else:
                dy = int(dy)
            entries.append((dy, int(mask[:-1], 16)))
        assert len(entries) == int(count)
        blob.extend(struct.pack('<H', int(count)))
        for dy, mask in entries:
            blob.extend(struct.pack('<hH', dy * 80, mask))
        small.append((len(rows), len(entries)))
        rows += entries
    assert len(small) == 17
    assert exe.find(blob) >= 0
    square = (research / 'analysis/original_source/CSQRT.ASM').read_text()
    values = [int(v) for v in re.findall(r'^\s*(?:sqrttbl\s+)?dw\s+(\d+)\s*$', square, re.M)]
    assert len(values) == 1024
    assert exe.find(struct.pack('<1024H', *values)) >= 0
    lines = ['#pragma once', '', '#include <array>', '#include <cstdint>', '',
             'namespace corncob::graphics {', '', 'struct disc_pattern_row {',
             '  int y;', '  uint16_t mask;', '};', '', 'struct disc_pattern {',
             '  unsigned int first_row;', '  unsigned int row_count;', '};', '',
             '// original small-disc masks and CSQRT table, byte-verified against the 3.42 executable',
             f'inline std::array<disc_pattern, {len(small)}> constexpr small_discs{{']
    for first, count in small:
        lines += ['  disc_pattern{', f'    .first_row{{{first}}},', f'    .row_count{{{count}}},', '  },']
    lines += ['};', '', f'inline std::array<disc_pattern_row, {len(rows)}> constexpr small_disc_rows{{']
    for y, mask in rows:
        lines += ['  disc_pattern_row{', f'    .y{{{y}}},', f'    .mask{{0x{mask:04x}}},', '  },']
    lines += ['};', '', 'inline std::array<uint16_t, 1024> constexpr disc_square_root{']
    for i in range(0, len(values), 12):
        lines += ['  ' + ', '.join(map(str, values[i:i + 12])) + ',']
    lines += ['};', '', '} // namespace corncob::graphics', '']
    (ROOT / 'src/graphics/disc_tables.h').write_text('\n'.join(lines))
    print('Byte-verified all 17 native small-disc masks and all 1,024 CSQRT values')


if __name__ == '__main__':
    main()
