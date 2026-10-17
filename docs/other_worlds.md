# Other Worlds Campaign

The native executable includes the normal 3.42 game and the standalone Other
Worlds 3.42 distribution. Resources are embedded; no second installation or DOS
helper program is needed. The grouped selectors extend the original menus while
retaining the original worlds and edition-specific campaign rules.

## Worlds and careers

Earth retains its original four identifiers. Expansion identifiers begin with
`other_worlds/`: `milkrun`, `flatland`, `quari`, `iceworld`, `midnight`, `tundra`,
`newatkin`, `thin`, `exp` (Arrakis), `morning`, `dusk`, `wastelnd` and `whtsands`.
The separate practice group contains `default`, `demo1` and `mtuniv` under the same
prefix. These are sixteen supplied definitions, not sixteen newly added planets.
Original Earth and expansion variants retain separate identities even when their
geometry matches; briefings, atmosphere, reserve stocks and completion flags can differ.

Names, renaming and deletion are shared across the two pilot rosters. Career
statistics, deaths, resurrections, ranks, medals, scores and opened worlds belong
to the selected campaign. Earth continues using the existing DOS-compatible
`PILOT.SCR`, `PILOT.HSC` and numbered `THT/*.THT` files. Other Worlds uses the same
containers below `other_worlds/`. Changing campaign never converts an existing
Earth career. Training follows the selected edition.

The original expansion continues promotion through Major, Lieutenant Colonel,
Colonel and General. Twenty objectives in two or three **different definitions**
qualify for the intermediate senior ranks: multiple copies of one world do not
count as different theatres. General requires completion of the required world
set. Practice/examples are not extra required campaigns. The original Master of
Corncob celebration, including its resurrection-dependent text, follows completion.

Multi-base theatres use the original nine airfield identities and the INT tile
partition. Starting-base selection, local aircraft/objective counts, tower text and
intelligence reports refer to that base. Returning to another base preserves its
runway start for the next sortie. Single-base Earth theatres retain their original
whole-world survey.

## Planetary flight and presentation

The TWR environment controls gravity, density, atmosphere height, sun and stars.
Older six-value headers retain the engine's default atmosphere height, as the DOS
reader does. Iceworld's weak gravity and deep atmosphere, Flatland's dense shallow
atmosphere and New Atkinsville's strong gravity therefore affect actual flight.

The original Other Worlds launcher always appends experimental-flight permission
(`MOAG.EXE` file `0x13cd7..0x13ce5`); Earth checks Combat Master instead. The
expansion also enters the space sequence in any aircraft above half the atmosphere
height. Earth requires the cleared experimental aircraft at three fifths. Its
outcome is the original orbit-and-rescue panel, rather than the Earth-saved panel.
Alt-I enables invulnerability in Earth; the expansion permits toggling it both ways.

Each edition uses its own outcome artwork, layouts, documents and sound patches.
Other Worlds has its own single-logo title presentation and the original scores:
VOYAGE3 for its title, HEAVENS then DAYBREAK in the menus. The title animation is
available from the campaign selector. All six scores use complete original-driver
passes, including the native restart gaps. Lossless FLAC masters remain in the
research workspace; the executable streams mono Vorbis copies from memory.

## Recorded demonstrations

The campaign selector offers the original Basic Mission, Junky Corridor and
Vertical Mission recordings. All three use DEMO1.DEF, as specified by DEMO.NDX.
Playback restores the original full/delta aircraft states and gun, missile, bomb,
flap and ejection events. Ten-unit recorded steps run across the native port's
two five-unit simulation steps. Escape stops playback; no career, score or saved
world is updated. Original ejected-pilot physics continue after the recorded
ejection, matching the original reader's decision to stop overwriting aircraft state.

## Mission builder

Choose Mission Builder to edit a fresh empty nine-airfield world. Alternatively,
highlight a world and press B to edit a fresh copy. The embedded originals remain
unchanged. Object handlers and flight stay stopped while the builder is open.

| Control | Action |
| --- | --- |
| Arrows | Turn the view |
| Keypad 5 / 0 | Move forwards / backwards |
| 1–8 | Movement speed |
| Page Up / Page Down | Change height |
| Left Shift | Place the cursor, or commit the selected object |
| Left Control | Hide the cursor |
| C | Create an object |
| Space | Select the object nearest the cursor |
| Alt + arrows / Page Up / Page Down | Move the selected object or cursor |
| A + arrows | Rotate the selected object |
| + / − | Cycle object types |
| [ / ] | Change object quality |
| F9 / F10 | Toggle special / objective-complete bits |
| Caps Lock | Level the view |
| Alt-H / H | Remember / apply an object's height |
| B, M or Escape | Toggle the overview |
| F5 | Save and return to the menu |

The builder uses original type definitions, qualities, flags and the expansion's
six-sphere cursor. The core editing operations are integrated into the native
window; its save action and on-screen instructions replace the DOS manual's
copy/rename workflow.

For named pilots the edited copy remains in the open-theatre list and can be
flown later. F5 also exports a plain native `.def` and matching original `.twr`
under `custom_worlds/` in the current campaign's save directory. The slot number
in the filename distinguishes different copies. Trainee worlds remain temporary,
but their explicit exports are still written. The cursor is never exported.

## Evidence and development tools

[Engine comparison](expansion-engine-comparison.json) records matching physical
object defaults for all 61 types, all eight ordered model drawing lists at every
detail level with initial colours, twelve actor/quality tables and twenty-one
runtime colour rules. Relocated pointers are excluded. The expansion's extra
model is the editor cursor. Shared data does not imply identical executable code;
the edition-specific launch, replay, editor, atmosphere and campaign paths remain
explicit.

The complete expansion promotion/medal/suspension scan is checked against 512
executions of its unchanged MOAG.EXE, including complete campaigns, duplicate
worlds, signed boundaries, resurrections and medal-count wrapping. Tests also
exercise every supplied world and airfield through loading, rendering and flight,
separate career persistence and ownership, builder edits/exports, all recorded
motion frames and both music playlists.

Development-only import and reference tools:

- `tools/import_other_worlds.py`: resource pack and per-file source hashes.
- `tools/compare_expansion_engine.py`: shared-data comparison against both originals.
- `tools/generate_other_worlds_career_reference.py`: native campaign reference cases.
- `tools/import_music.py --edition other_worlds`: original-driver masters and Vorbis.

These tools are unnecessary for normal builds or gameplay. Source hashes are in
[the expansion resource record](../assets/other_worlds_provenance.json) and
[the expansion music record](../assets/music/other_worlds/provenance.json).
