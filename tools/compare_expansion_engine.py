#!/usr/bin/env python3
"""Check which original runtime data can safely be shared between the two editions.

Development-only analysis; paths and addresses come from preserved extraction
records, with the original executable hashes retained in the resulting report.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--research', type=Path, default=ROOT.parent / 'corncob-reverse')
    args = parser.parse_args()
    sys.path.insert(0, str(args.research / 'tools'))
    from compare_models import model_shape
    directory = args.research / 'analysis/resources/models'
    earth = json.loads((directory / 'deluxe_dnk.json').read_text())
    other = json.loads((directory / 'other_worlds_download.json').read_text())
    source = [(args.research / 'installs' / name / '3.EXE').read_bytes()
              for name in ('deluxe_dnk', 'other_worlds_download')]
    segments = [data[profile['data_file_base']:] for data, profile in zip(source, (earth, other))]
    comparisons = []
    # Exclude function/model pointers: those are relocated, while physical defaults and actor parameters are retained.
    fields = list(range(20)) + list(range(24, 40)) + list(range(46, 74))
    for a, b in zip(earth['objects'], other['objects']):
        original, expansion = (bytes.fromhex(record['template_raw']) for record in (a, b))
        assert all(original[i] == expansion[i] for i in fields), a['type']
        for near, far in zip(a['model_variants'], b['model_variants']):
            assert model_shape(earth, near, True) == model_shape(other, far, True), a['type']
        comparisons.append(a['type'])
    strip = lambda rule: {key: value for key, value in rule.items() if key not in ('address', 'table_address')}
    assert list(map(strip, earth['runtime_colours']['rules'])) == list(map(strip, other['runtime_colours']['rules']))
    tables = json.loads((args.research / 'analysis/gameplay/tables.json').read_text())['quality_tables']
    matched = []
    for name, table in tables.items():
        if name == 'orb_objective':
            continue  # Includes executable handler addresses; colour values are verified by the runtime rules above.
        begin, size = table['ds_offset'], table['word_stride'] * 2 * 8
        raw = segments[0][begin:begin + size]
        located = segments[1].find(raw)
        assert located >= 0, name
        matched.append({'name': name, 'earth_ds': begin, 'other_worlds_ds': located, 'bytes': size})
    known = {json.dumps(model_shape(earth, key, True), sort_keys=True) for key in earth['models']}
    extra = [key for key in other['models'] if json.dumps(model_shape(other, key, True), sort_keys=True) not in known]
    assert extra == ['5c86'], extra
    report = {
        'executables_sha256': [hashlib.sha256(data).hexdigest() for data in source],
        'matching_templates': comparisons, 'matching_quality_tables': matched,
        'matching_runtime_colour_rules': len(earth['runtime_colours']['rules']),
        'extra_model': {'other_worlds_ds': '5c86', 'use': 'Mission-builder six-sphere axis cursor'},
        'shared_geometry_scope': 'All 61 template defaults and all eight ordered model drawing lists at every detail level, including initial resolved colours; relocated storage addresses excluded.',
        'behaviour_differences': [
            'Registered Other Worlds build enables map mission builder and recorded-flight replay.',
            'Other Worlds enters SRATS in any aircraft at half atmosphere height; Earth needs cleared experimental aircraft at three fifths.',
            'Other Worlds Alt-I toggles invulnerability both ways; Earth only enables it.',
            'Other Worlds MOAG unconditionally appends f at file 0x13cd7..0x13ce5 (mov ax,1); Earth checks Combat Master at 0x112f2..0x11307.',
            'World, airfield, required-theatre and career rules are selected by edition.',
        ],
        'scope': 'This checks shared runtime data, not a claim that the entire executable instruction streams are identical.',
    }
    (ROOT / 'docs/expansion-engine-comparison.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'61 template defaults and complete model lists, {len(matched)} quality tables and 21 colour rules match; one editor cursor model added')


if __name__ == '__main__':
    main()
