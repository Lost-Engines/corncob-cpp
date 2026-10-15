#!/usr/bin/env python3
"""Import verified 3.42 model data and base-game worlds for compilation into Corncob.

Development only. Imported files and generated profiles are committed; CMake
embeds them without Python or access to the reverse-engineering workspace.
"""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REFERENCE_HASH = 'c411dde7d18e1dc19d718327b70d1e69321448520344d3baa4a0c8e68a71e81e'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--research', type=Path, default=ROOT.parent / 'corncob-reverse')
    args = parser.parse_args()
    install = args.research / 'installs/deluxe_dnk'
    exe = (install / '3.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == REFERENCE_HASH
    models = json.loads((args.research / 'analysis/resources/models/deluxe_dnk.json').read_text())
    assets = ROOT / 'assets'
    records = []

    def save(name, contents, source):
        (assets / name).write_bytes(contents)
        records.append({'path': name, 'source': source, 'bytes': len(contents), 'sha256': hashlib.sha256(contents).hexdigest()})

    save('engine.dat', exe[models['data_file_base']:], '3.EXE data segment at file offset 0x11490; original model, template and tuning data, no executable instructions')
    for target, source in (('white_sands', 'WHTSANDS'), ('morning', 'MORNING'), ('dusk', 'DUSK'), ('wasteland', 'WASTELND')):
        save(target + '.cct', (install / 'DEF' / (source + '.CCT')).read_bytes(), 'DEF/' + source + '.CCT')
        save(target + '.twr', (install / 'DEF' / (source + '.TWR')).read_bytes(), 'DEF/' + source + '.TWR')
    records.append({'path': 'cockpit.img', 'source': 'IMG/3D.IMG', 'bytes': (assets / 'cockpit.img').stat().st_size,
                    'sha256': hashlib.sha256((assets / 'cockpit.img').read_bytes()).hexdigest()})
    manifest = {'reference': 'normal Corncob 3D 3.42', 'engine_sha256': REFERENCE_HASH, 'files': records,
                'note': 'Original model/template/world data embedded at build time. No DOS engine is executed or required at runtime. Music is not included.'}
    (assets / 'provenance.json').write_text(json.dumps(manifest, indent=2) + '\n')
    lines = ['#pragma once', '', '#include <array>', '#include <cstdint>', '', 'namespace corncob::resources {', '',
             'enum class colour_rule_kind { random, alternate, radar, flag_blink, quality, quality_random };', '',
             'struct colour_rule {', '  unsigned int address;', '  colour_rule_kind kind;',
             '  std::array<uint16_t, 8> values;', '  uint64_t types;', '  uint64_t random_types;',
             '  uint16_t flag_mask;', '  uint16_t timer_mask;', '};', '',
             '// generated from the native-verified colour profile; addresses refer only to embedded data',
             f'inline std::array<colour_rule, {len(models["runtime_colours"]["rules"])}> constexpr colour_rules{{']
    for rule in models['runtime_colours']['rules']:
        values = list(rule.get('values', []))
        values += [0] * (8 - len(values))
        types = sum(1 << kind for kind in rule.get('types', []))
        random_types = sum(1 << kind for kind in rule.get('random_types', []))
        lines += ['  colour_rule{', f'    .address{{0x{rule["address"]:04x}}},',
                  f'    .kind{{colour_rule_kind::{rule["kind"]}}},',
                  '    .values{' + ', '.join(map(str, values)) + '},',
                  f'    .types{{0x{types:x}ull}},', f'    .random_types{{0x{random_types:x}ull}},',
                  f'    .flag_mask{{0x{rule.get("flag_mask", 0):04x}}},',
                  f'    .timer_mask{{0x{rule.get("mask", 0):04x}}},', '  },']
    lines += ['};', '', '} // namespace corncob::resources', '']
    (ROOT / 'src/resources/colour_rules.h').write_text('\n'.join(lines))
    stars = json.loads((args.research / 'analysis/resources/renderer/stars.json').read_text())
    lines = ['#pragma once', '', '#include <array>', '#include "maths/fixed.h"', '', 'namespace corncob::graphics {', '',
             '// original generated star bank: drawing consumes the anchor and the first 79 generated records',
             'inline std::array<maths::world_position, 80> const original_stars{']
    for record in stars['records'][:stars['draw_count']]:
        lines.append('  maths::world_position{' + ', '.join(map(str, record['position_cm'])) + '},')
    lines += ['};', '', '} // namespace corncob::graphics', '']
    (ROOT / 'src/graphics/stars.h').write_text('\n'.join(lines))
    tables = json.loads((args.research / 'analysis/gameplay/tables.json').read_text())['quality_tables']
    lines = ['#pragma once', '', '#include <array>', '#include <cstdint>', '', 'namespace corncob::resources {', '',
             '// generated from the original 3.42 quality tables; qualities are stored as indices 0..7']
    for name, table in tables.items():
        if name == 'orb_objective':
            continue
        fields = [field for field in table['rows'][0] if field != 'quality']
        lines += ['', 'struct ' + name + '_parameters {']
        for field in fields:
            signed = any(row[field] < 0 for row in table['rows'])
            lines.append('  ' + ('int16_t' if signed else 'uint16_t') + ' ' + field + ';')
        lines += ['};', '', 'inline std::array<' + name + '_parameters, 8> constexpr ' + name + '_qualities{']
        for row in table['rows']:
            lines.append('  ' + name + '_parameters{')
            for field in fields:
                lines.append('    .' + field + '{' + str(row[field]) + '},')
            lines.append('  },')
        lines.append('};')
    lines += ['', '} // namespace corncob::resources', '']
    (ROOT / 'src/resources/actor_tables.h').write_text('\n'.join(lines))
    print('Imported embedded model data and four base-game theatres; generated traced colour rules')


if __name__ == '__main__':
    main()
