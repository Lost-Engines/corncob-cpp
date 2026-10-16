# Reconstruction reference

The target is **normal Corncob 3D 3.42**, shared by the retained shareware and
Personal Companion retail flight engines. `3.EXE` has SHA-256
`c411dde7d18e1dc19d718327b70d1e69321448520344d3baa4a0c8e68a71e81e`.
Research inputs, disassembly and original DOS captures live in the sibling
`corncob-reverse` workspace. Browser previews are inspection aids; unchanged
executable routines supply the numerical reference.

The application is C++ throughout. It does not execute original x86 instructions,
launch DOS helpers or require an external installation. CMake embeds the original
data and artwork; Python, Unicorn, DOSBox and the research workspace are development
inputs only. Sound effects are included; music remains deliberately deferred.

## Original source and provenance

The public [foone/Corncob3D repository](https://github.com/foone/Corncob3D) contains
original assembly for flight, objects, rendering, instruments, sound and SRATS.
The **Initial Import** commit `2759414da1e6e645c3640288577d1e0cc8c32be4` is
preserved under `corncob-reverse/analysis/original_source/`, without subsequent
MASM edits or uploader documentation. The source distinguishes shareware and
registered builds but omits the C menu program and a complete build environment.
Some paths differ from the retained executable; target-binary behaviour takes
precedence.

The upstream account describes an original 2001 release, republication in 2018
and informal permission from Kevin Stokes, with no formal source licence. That
account is not a new licence grant. Embedded input hashes and origins are recorded
in [assets/provenance.json](../assets/provenance.json); third-party terms remain
under `third_party/`.

## Connected game

One session retains the aircraft, on-foot pilot, world, weapons and active actors.
It supports walking/jumping, boarding and re-entry, powered flight and landing,
ejection, free fall, parachuting and remote flight. Four original theatres and
procedurally generated training are available. Fixed five-unit simulation steps
advance forty fast PIT ticks; T changes physical time while retaining frame timers.

Weapons use the original separate 32-bullet and 320-effect pools: guns/cannon
mode, finite pistol ammunition, unguided missiles, replenishing bombs and the
planted assassin charge. Effects include debris, incendiaries, mortar barrages,
concealed sites, orbiting/swaying enemies, forcefields, generator attraction and
mindbender palette effects. Aircraft damage retains its separate engine, wing,
rudder and stability channels rather than generic hit points.

Aircraft damage/ejection, actors and weapon constructors share one gameplay random
stream. The saucer's modes select retreat, circling, approach and coasting around
the current airbase. Its death uses debris-before-spin random ordering; ground
impact produces upward fragments and fifteen incendiaries. Barrage balloons
survive ordinary contacts. Bees retain the accidental RAND_W carry-flag dependency
that sometimes skips their vertical update. Dead aliens disappear outside their
near tier. Actor equivalence beyond the independently tested handlers remains
under comparison.

The original autopilot includes pitch-by-speed feedback and altitude capture.
Optional beginner aerodynamics adds the original weaker PITCHLOCK correction.
When both are enabled, the two original calls remain. Advanced is the default;
`--flight-model beginner` selects assistance. The controls setup selects the same
model and keyboard/joystick choice before a new flight. Settings currently last
for the process; mapping them to original pilot configuration bytes is unfinished.

Digital joysticks supply two calibrated axes and two buttons through GLFW. The
original squared response and clipping are retained, without analogue-port timing
or calibration prompts. Arrows override both stick axes; head-view keys leave the
physical stick available for flight. Buttons select guns/missiles; Space requests
missiles in joystick mode. `--joystick auto|none|N` selects a device, and
`--list-joysticks` reports indices/names. Polling is restricted to focused flight.
Device discovery detects the development host's T.16000M; physical control motion
has not been exercised during automated checks.

## Unified interface and campaign

The executable joins title/story, roster, theatre selection/intelligence, training,
controls/help, flight, outcomes, reports and the interactive Earth-saved ending.
The menu is a functional reconstruction of the missing C front end, rather than
a claim of matching every original menu pixel.

F1 retains BOSS.TXT's DOS-prompt disguise. F2 exposes intelligence and stored Sxyth
airborne starts without rebuilding the world; damaged aircraft retain the original
jump restriction and message. F3 reports the retained sortie without recording it.
F4 traverses ten original help pages. Eleven BLACKBOX records supply native warning
rectangles, text and placements. Flight Escape warns once when messages are enabled
and the pilot is beyond the tower's 5,200-unit range; a later Escape ends the sortie.
Control-C follows the distinct uncounted abort path.

Walking into a tower opens intelligence and pauses the retained simulation.
DOTOWER continuation places the pilot outside the doorway, repairs the retained
aircraft and recharges its rocket, preserving historical worst damage. It does
not end or record a sortie. Invulnerable ground crashes level and bounce the
plane through RESETPLANE/BOUNCEVERT instead of killing the pilot. Rescue calls
create one friendly van; hostile vehicles draw an exposed pilot toward actual
capture collision bounds.

Named pilots retain changed worlds independently. Roster operations create,
activate, deactivate, rename and resurrect pilots; records display career
statistics and original medal glyphs/colours. Three native high-score lists cover
missions, careers and integer points-per-sortie careers with at least three
sorties. The three required normal theatres grant Combat Master clearance; the
experimental aircraft's altitude condition leads to the Earth-saved ending.
Invulnerable and aborted flights leave career bytes and high-score lists unchanged.
World persistence remains separate from career credit.

PILOT.SCR retains opaque 623-byte records, both observed header layouts and the
original Borland random mask. PILOT.HSC retains its 5,040-byte layout and padding.
THT uses the native 671-byte container and compact 64-tile world stream. Both plain
and coordinate-XOR type words are accepted only after a complete world validates;
new saves use XOR. All four original definitions round-trip byte for byte. Save
replacement uses a completed temporary write. Current filenames use stable roster
slots; original automatic filename selection and all bookkeeping are not yet fully
interoperable.

## Rendering, presentation and sound

The software renderer retains 640 x 350 indexed EGA output, three distance/detail
tiers, authored octant painter lists, fixed-point projection and focal-plane
clipping. OpenGL only presents the CPU framebuffer. Floating window sizing fits
available desktop space and preserves the intended 4:3 CRT shape. VGA presentations
use 320 x 200; menus use 640 x 400 BIOS text. Nearest-neighbour sampling preserves
pixel boundaries. The original cockpit mask's stray diagonal is retained.

Original instruments, propeller, ground points, stars, sun directives, radar,
homing pointer and animated model geometry are connected. Wire forcefields pulse
with matching collision bounds; saucer discs pulse in opposition. Quality-zero
filled barriers reveal their distant representation briefly on contact. Null
model pointers intentionally draw nothing. Radar retains its unusual square-root
range compression and draws relative to the aircraft after ejection. Num Lock and
Control select eased head views. The overhead map retains depth reduction, stored
survey/player markers, pan/zoom/rotation and frozen sortie state.

Original VGA/IMG data and presentation scripts supply two story panels, fifteen
outcomes and ten help screens. MEDAL.FNC replaces sixteen BIOS glyphs; ten award
entries use MOAG's original pair/colour table at file offset `0x1fa82`. Full animated
title/reflection/fade playback remains unfinished; the current title is static.

The Earth-saved transition retains the indexed cockpit, shrinks the Earth in the
rear window and fades only the sky before the SRATS starfield. Enter toggles warp
trails; Escape continues to outcome/report. The target flashes white after 270
warp ticks, unlike the later registered-build source conditional. Both EGA pages
retain independent trails; BIOS animation time continues while flight is stopped.

The twelve original 3.ADL patches drive Nuked OPL2; miniaudio supplies 48 kHz stereo
PCM. This is mono AdLib duplicated into two channels, not new spatial stereo.
Engine state, launches, explosions and nearby threats drive the original channels.
Crash timing continues through eighteen BIOS ticks and a 750 ms wait independently
of the menu loop. Register batches cross to the PCM callback without sharing live
world state. Muting does not queue stale events. Full combined DOS soundscape and
interrupt modulation still need broader comparison.

## Verification

Fixtures execute unchanged target routines in development. Their JSON metadata
records executable/fixture hashes, addresses, scope and any explicit stubs. Ordinary
CTest runs are C++ only; checks do not merely compare two translations of a formula.

| Area | Independent native coverage |
| --- | --- |
| Fixed maths | All 65,536 sine inputs; 8,228 airflow angles; 512 forward/inverse rotation sets; 512 physics and fast vertex transforms each; 128 matrix compositions |
| Landing maths | 1,024 magnitudes; 256 wheel clearances; 512 suspension/brake passes; 512 invulnerable bounce cases |
| Flight | 256 force passes, including intermediate accumulators; 128 position integrations; 512 controls and attitude passes each |
| Autopilot/beginner | 512 feedback cases covering both models, stall state and engagement combinations |
| Joystick | 512 SPKE/JOYTD response cases, plus keyboard/button/head-view integration |
| Person | 512 free-fall damping and 512 parachute swing cases |
| Motion/collision | 512 generic movement, 512 strict boxes, 512 fast and 512 ordinary projectile contacts |
| Damage/actors | 256 aircraft damage passes; 512 KLA and 512 living saucer passes; 1,536 AAA/balloon/bee/alien updates, including shell initialization |
| Debris/training | 530 particles from 64 constructor runs; 2,157 generated placements from 32 training runs |
| Raster/resources | All 224,000 cockpit pixels; 30 complete disc masks and 12 polygon hashes; four original definition round trips; one 252-object plain saved world |
| Model/map/radar | 512 geometry-animation, 512 map reductions, 512 radar positions; all 32,768 compressed radar-distance inputs |
| Ending | All 768 initial stars and 64 complete indexed native star frames |
| Score/persistence | 512 score calculations; 16,384 original mask bytes; native roster/high-score round trips and restart scenarios |
| Sound | 512 engine/register-write cases and finite, non-silent PCM from the actual engine patch |

Connected scenarios cover takeoff, ejection/parachute/remote control, weapons and
persistent objectives, rescue/capture, damaged-aircraft tower visits, settings,
career clearance and the ending. A sustained run exercises 32 original-theatre
starts with mixed controls/weapons/ejection, more than 31,000 updates and 250
rendered frames, then round-trips resulting worlds/results. It exposed the hidden
far-model crash before interactive playtesting.

Private GLFW/X11 checks traversed pilot creation, mission selection, radio boxes,
map, boss screen, help, score, abort/report, tower intelligence, repair/resume and
Escape warnings. Captures were inspected without changing supplied DOS installs.
The complete 32-check suite passes under address and undefined-behaviour
sanitizers, including the longer scenarios. The preceding 31-check Release
suite also passed; optimized coverage is repeated as implementation changes. Wire rasterisation, complete frame ordering,
Windows/macOS runtime and interactive Wayland sizing are not yet independently
established.

## Remaining fidelity work

- Translate the animated title, palette transitions and reflection playback.
- Complete original front-end/bookkeeping details: pilot deletion, high-score
  retirement/renaming, original configuration bytes, scanner/award qualifications
  and automatic saved-world naming.
- Verify remaining ground-vehicle, launcher and special-site handlers against
  full native executions, and broaden complete campaign comparisons.
- Finish unconnected cockpit/visual feedback, including the full high-G fade and
  any remaining auxiliary instrument states; compare wire rasterisation and whole
  scene painter/collision ordering.
- Check capture fading, portal placement and other connected special transitions
  against longer DOS sessions. Scripted coverage is not proof of every transition.
- Reconcile rendering's separate visual random stream with original complete-frame
  draw order before claiming identical whole-session random continuation.
- Compare the complete soundscape and confirm platform/device behaviour beyond
  the Linux development host. Music remains outside the current implementation.
