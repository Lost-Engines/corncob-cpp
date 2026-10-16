#!/usr/bin/env python3
"""Build lossless masters and embedded Vorbis music from original-driver DRO captures.

Development only: requires the sibling research workspace, render_music_capture,
flac and oggenc. No sequencer, DOS program or recording tool runs in the game.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SONGS = ('destiny3', 'ridingt', 'dreamso2')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def decode_capture(path):
    data = path.read_bytes()
    if len(data) < 26 or data[:8] != b'DBRAWOPL' or struct.unpack_from('<HH', data, 8) != (2, 0) or data[20:23] != b'\0\0\0':
        raise ValueError(f'Expected uncompressed DOSBox DRO v2 OPL2 capture: {path}')
    commands, duration = struct.unpack_from('<II', data, 12)
    table = data[26:26 + data[25]]
    stream = data[26 + len(table):]
    if len(stream) != commands * 2:
        raise ValueError(f'Capture was not closed cleanly: {path}')
    milliseconds, events = 0, []
    for command, value in struct.iter_unpack('BB', stream):
        if command == data[23]:
            milliseconds += value + 1
        elif command == data[24]:
            milliseconds += (value + 1) * 256
        else:
            events.append((milliseconds, table[command & 127] + (256 if command & 128 else 0), value))
    if milliseconds != duration:
        raise ValueError(f'Capture duration mismatch: {path}')
    return events


def note_on(event):
    _, register, value = event
    return 0xb0 <= register <= 0xb8 and value & 32 or register == 0xbd and value & 31


def score_boundary(events):
    """Identify the original driver's full reset and the next complete score, rather than guessing a duration."""
    first_reset = next(i for i, e in enumerate(events) if e[1:] == (8, 0))
    second_start = next(i for i in range(first_reset, len(events)) if note_on(events[i]))
    second_reset = next(i for i in range(second_start, len(events)) if events[i][1:] == (8, 0))
    first_notes = [e[1:] for e in events[:first_reset] if note_on(e)]
    second_notes = [e[1:] for e in events[second_start:second_reset] if note_on(e)]
    if not first_notes or first_notes != second_notes:
        raise ValueError('Capture must contain two complete, matching passes of one score')
    return events[second_start][0], events[first_reset][0], len(first_notes)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--research', type=Path, default=ROOT.parent / 'corncob-reverse')
    parser.add_argument('--captures', type=Path, required=True, help='directory containing destiny3.dro, ridingt.dro and dreamso2.dro')
    parser.add_argument('--renderer', type=Path, required=True, help='compiled tools/render_music_capture.cpp')
    args = parser.parse_args()
    install = args.research / 'installs/deluxe_dnk'
    output = args.research / 'analysis/resources/audio'
    archive = output / 'native-driver'
    archive.mkdir(parents=True, exist_ok=True)
    embedded = ROOT / 'assets/music'
    embedded.mkdir(exist_ok=True)
    previous = json.loads((output / 'music.json').read_text())
    records = []
    for index, song in enumerate(SONGS):
        capture = args.captures / (song + '.dro')
        end_ms, reset_ms, notes = score_boundary(decode_capture(capture))
        saved_capture = archive / capture.name
        if capture.resolve() != saved_capture.resolve():
            shutil.copyfile(capture, saved_capture)
        flac = output / (song + '-c95a84ee.flac')
        with subprocess.Popen([str(args.renderer.resolve()), str(capture), str(end_ms)], stdout=subprocess.PIPE) as render:
            result = subprocess.run(['flac', '-8', '--silent', '--force', '--force-raw-format', '--endian=little',
                                     '--sign=signed', '--channels=1', '--bps=16', '--sample-rate=48000',
                                     '-T', 'ARTIST=James L. Collymore', '-T', 'ALBUM=Corncob 3D',
                                     '-T', 'TITLE=' + song.upper(), '-o', str(flac), '-'], stdin=render.stdout)
            render.stdout.close()
            code = render.wait()
            if code or result.returncode:
                raise RuntimeError('Native music rendering failed: ' + song)
        subprocess.run(['flac', '-t', '--silent', str(flac)], check=True)
        ogg = embedded / (song + '.ogg')
        subprocess.run(['oggenc', '-Q', '-q', '6', '-s', str(0x434f524e + index), '-o', str(ogg), str(flac)], check=True)
        source = install / (song.upper() + '.ROL')
        record = {
            'song': song, 'sequence_sha256': sha(source), 'bank_sha256': sha(install / 'CCMUSIC.BNK'),
            'capture': 'audio/native-driver/' + song + '.dro', 'capture_sha256': sha(saved_capture),
            'score_reset_ms': reset_ms, 'repeat_boundary_ms': end_ms, 'verified_note_ons_per_pass': notes,
            'flac': 'audio/' + flac.name, 'flac_sha256': sha(flac),
            'embedded': 'music/' + ogg.name, 'embedded_bytes': ogg.stat().st_size, 'embedded_sha256': sha(ogg),
            'frames': end_ms * 48, 'sample_rate': 48000, 'channels': 1,
        }
        records.append(record)
        for entry in previous:
            if Path(entry['song']).stem.lower() == song and entry['bank_sha256'] == record['bank_sha256']:
                entry.update(samples=record['frames'], seconds=end_ms / 1000, channels=1, release_tail_seconds=0,
                             renderer='Original MOAG/Ad Lib V1.51 register capture + pinned Nuked OPL3 (OPL2)',
                             timing='Complete original-driver pass to the next native note-on; includes the native restart gap',
                             native_capture=record['capture'], gain='Original mono emulator DAC output; no normalisation')
        print(song.upper(), f'{end_ms / 1000:.3f}s', ogg.stat().st_size, 'embedded bytes', flush=True)
    metadata = {
        'reference': 'normal Corncob 3D 3.42',
        'player_sha256': sha(install / 'MOAG.EXE'),
        'driver_sha256': hashlib.sha256(bytes(v ^ 0xa5 for v in (install / 'MOAGDATA').read_bytes())).hexdigest(),
        'composer': 'James L. Collymore', 'copyright': 'Music copyright 1986-1991 James L. Collymore; original terms remain applicable',
        'capture_method': 'Unchanged MOAG.EXE and bundled Ad Lib driver in private DOSBox 0.74-3 copies; single-song playlist repeated twice; virtual-time turbo preserves PIT playback timing',
        'timing_resolution_ms': 1, 'masters': '48 kHz, mono, signed 16-bit FLAC -8; no gain changes or normalisation',
        'encoding': 'Ogg Vorbis quality 6, Xiph libVorbis 1.3.7 / oggenc 1.4.3',
        'renderer_source_sha256': sha(ROOT / 'tools/render_music_capture.cpp'),
        'nuked_opl3_commit': '765ec962e473aeb767e4cba74ffdc8f588ffbfe8',
        'menu_playlist': ['ridingt', 'dreamso2'], 'recordings': records,
    }
    (archive / 'recordings.json').write_text(json.dumps(metadata, indent=2) + '\n')
    (embedded / 'provenance.json').write_text(json.dumps(metadata, indent=2) + '\n')
    (output / 'music.json').write_text(json.dumps(previous, indent=2) + '\n')


if __name__ == '__main__':
    main()
