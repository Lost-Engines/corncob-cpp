#!/usr/bin/env python3
"""Convert unchanged TOWER.EXE screen/return-file captures into a C++ test fixture.

Run corncob-reverse/tools/capture_native_tower.py first. Expected pixels and
mission coordinates come exclusively from the original executable.
"""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--captures', type=Path, default=ROOT.parent / 'corncob-reverse/analysis/native-tower')
    args = parser.parse_args()
    data = json.loads((args.captures / 'captures.json').read_text())
    if data['executable_sha256'] != '9bfcf33fb27f6e35e17383c8cc19a6f125e287cd75fb9c7f0e282f639f110233':
        raise ValueError('unexpected original TOWER.EXE')
    lines = ['#pragma once', '', '// Generated from unchanged TOWER.EXE; see tools/generate_tower_briefing_reference.py.',
             '#include <array>', '#include <cstdint>', '#include <string_view>', '', 'namespace corncob::tests {', '',
             'struct tower_case {', '  std::string_view name;', '  std::string_view source;', '  std::string_view text;',
             '  unsigned int airfield;', '  std::string_view keys;', '  std::string_view close;', '  uint64_t rgb_hash;',
             '  std::string_view start;', '};', '', f'inline std::array<tower_case, {len(data["cases"])}> constexpr tower_cases{{']
    for case in data['cases']:
        lines += ['  tower_case{', '    .name{' + json.dumps(case['name']) + '},',
                  '    .source{' + json.dumps(case.get('source', '')) + '},',
                  '    .text{' + json.dumps(case.get('text', '')) + '},',
                  f'    .airfield{{{case["tower"]}}},',
                  '    .keys{' + json.dumps(' '.join(case['keys'])) + '},',
                  '    .close{' + json.dumps(case['close']) + '},',
                  '    .rgb_hash{0x' + case['rgb_fnv64'] + 'ull},',
                  '    .start{' + json.dumps(case['start'] or '') + '},', '  },']
    lines += ['};', '', '} // namespace corncob::tests', '']
    (ROOT / 'tests/reference/tower_briefings.h').write_text('\n'.join(lines))


if __name__ == '__main__':
    main()
