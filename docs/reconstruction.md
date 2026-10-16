# Reconstruction reference

The behavioural reference is normal-game Corncob 3D 3.42, whose flight executable
is shared by the shareware and Personal Companion retail distributions. The
research inputs are in the sibling `corncob-reverse` workspace. Its native checks,
resource hashes and recorded force scenarios are the verification reference;
browser previews are inspection tools rather than pixel-exact renderer models.

The first playable milestone is White Sands with the original software renderer
and aircraft dynamics. Walking, ejection, remote flight, combat, persistent
campaigns and the unified front end follow. Sound effects are in scope; music
is deferred. The core will not execute x86 code or depend on DOSBox at runtime.

## Original source discovery

During project setup we found the public
[foone/Corncob3D source repository](https://github.com/foone/Corncob3D), inspected at
Initial Import commit `2759414da1e6e645c3640288577d1e0cc8c32be4`, now preserved
in `corncob-reverse/analysis/original_source/` without the later MASM edits.
It contains original assembly for flight, object definitions, rendering,
instruments, sound and the SRATS ending. Its makefile distinguishes shareware
and registered builds. The snapshot's exact relationship to the retained 3.42
binaries still needs to be established. It does not supply the C menu program
or a complete standalone build environment.

The repository reports an original 2001 release, republication in 2018 and
informal permission from Kevin Stokes; it has no formal source licence.
That account is not a licence grant from this project. The recovered source is
an additional research reference, not a substitute for checking the target
binary or a reason to import third-party assembly-library code blindly.

## Display and resource foundation

The software framebuffer remains 640 x 350; presentation corrects the EGA pixel
aspect ratio to 4:3. OpenGL only uploads and presents CPU-generated pixels.
Window sizing follows Darker: fit the monitor's work area, or query the Wayland
compositor's maximised size and restore to a fitted floating window.

The first resource is the unmodified compressed cockpit mask, embedded by CMake.
The decompressor is checked against the native-verified decoded mask. Its stray
diagonal line is part of the original asset and is deliberately retained.

The four original theatres and the 3.42 data segment are now embedded too.
Typed loaders decode the 61 object templates, shared polygon/wire/point/disc
primitives and eight authored painter lists. The renderer retains the original
three distance/detail tiers, fixed-point projection, focal-plane clipping,
viewport frames and direction-dependent painter order. Sky and ground colours
come from the tile headers, rather than a guessed default palette.

The native session now owns live placements, the aircraft and the on-foot pilot.
It starts on foot, supports boarding and re-entry, flight, landing, ejection,
parachuting and remote flight. Original instruments, propeller, ground dots,
radar rotation, crew gestures and theatre sun/atmosphere directives are connected.
Combat, AI, campaign progression, the unified front end and sound remain underway.
The free camera is a developer inspection aid.

## Initial validation

Linux Debug and Release builds pass the embedded-resource checks. The native
headless screenshot matches all 224,000 decoded cockpit pixels. A private X11
display confirms a 1,280 x 960 floating window within a 1,600 x 1,200 desktop and
clean Escape handling. Release help excludes the developer screenshot and time
limit options. Windows/macOS runtime checks and Wayland interactive sizing have
not yet been performed.

## Fixed-point translation

The original 257-word sine table exactly matches 3.42 at file offset `0x10280`.
The C++ signed sine matches native execution for every one of the 65,536 angle
inputs. Forward/inverse rotation generation matches 512 angle sets; saturated
physics transforms and fast vertex transforms match 512 cases each, and matrix
composition matches 128 cases. Fixtures come from execution of unchanged DOS
routines, not from a second implementation of the same formulas.

The translation preserves the original word wrapping, product truncation,
pairwise MADD rounding and its JS/JNS sign-test overflow quirk. Model vertices
use the original rotate-high-word multiplication, which differs from HMUL.
Inverse matrix generation is translated separately because transposing an
already rounded forward matrix does not produce the same coefficients.

`tools/generate_native_reference.py` regenerates the fixtures using Unicorn
and the sibling research inputs. These are development dependencies only.
The normal CMake build and executable require neither.

## Software raster verification

The C++ disc routine matches every indexed pixel from 30 executions of original
DRAWDisc: all 17 small masks, larger CSQRT-based discs and viewport-edge cases.
Twelve polygon fixtures likewise match original DRAWPoly, including reversed
winding, viewport clipping, degenerate surfaces and a rejected concave shape.
The scan converter preserves the negative-slope fractional-word quirk, rounded
division and EGA row-address clipping. Wire rasterisation and the complete scene
are not yet claimed to match every original pixel.

The fixture generators use Unicorn and a small EGA plane/latch/write-mode adapter
in development only. Normal tests run entirely in C++ with committed fixture
hashes. An additional sweep renders from every placed object's neighbourhood in
all four theatres, testing cockpit/full-screen layouts and front/rear clipping.

## Aircraft force foundation

The first aircraft simulation routines are translated and tested independently
of the scene preview: atmosphere/density, gravity in body axes, thrust and rocket
boost, longitudinal/lateral drag, lift, flap/stall limits and position integration.
They preserve the original previous-pass density dependency, wrapping force
accumulators, rounded dword division, word velocity limits and the experimental
aircraft's fourfold position scaling. Control response and ground contact now connect this foundation to the interactive session.

The original 1,024-entry arctangent table matches the target binary too. Tests
compare 8,228 airflow angles, 256 native force passes (including intermediate
forces, acceleration, stall and rocket state) and 128 position integrations.
The first eight force passes reproduce the established research scenarios;
others exercise rotated axes, variable time steps, atmosphere, aircraft variants
and ground flags. HUD/audio calls are stubbed only in the native fixture harness.
The ordinary build runs pure C++, without emulator dependencies.

## Controls, landing and live-session checks

A further 512 native control passes and 512 attitude/matrix passes verify the
speed- and density-dependent angular response, ground steering and stall limiter.
The autopilot, optional stability assistance and damage-induced jitter are not
yet connected. Ground checks cover 1,024 original approximate vector magnitudes,
256 wheel-height cases and 512 suspension/braking passes. The source disables its
optional suspension-damping branch, as does the reference binary.

Scripted input walks to the actual White Sands aircraft, boards, verifies its
clearance lock and pause behaviour, then grants clearance to exercise a powered
takeoff and sustained climb. It also ejects, opens the parachute and flies the
abandoned aircraft remotely. Captured frames are inspected during development.
This checks the connected path without claiming every pilot/flight transition
has yet been matched against a complete DOS session.

Generic object motion is checked against 512 original OBJMOV executions, including
PRNG drift, quadratic drag, rotation, time adjustment, the special gravity-55
constant-descent convention and ground stopping. A further 512 CHKBOX cases verify
strict overlap, zero-X disabling and carries in summed unsigned half-extents.
The successful-overlap callbacks are stubbed in those geometry-only fixtures.

## Combat and active actors

Player guns/cannon mode, finite pistol ammunition, unguided missiles, replenishing
bombs and the planted assassin charge now use separate original-capacity bullet
and effect pools. Timed movement precedes actor handlers and collision callbacks.
Original fast-projectile contacts match 512 native QVELON cases; ordinary contacts
match 512 native QCOLLIS1 cases. Aircraft damage matches 256 original DAMAGEPLANE
passes, including random-stream continuation, capped wing/rudder channels and
engine throttle loss. Native KLA launch-rest, steering, speed/noise and low-ground
avoidance match another 512 reference passes.

A scripted weapon scenario destroys a fuel dump through actual gun launch,
movement and contact, tests protected orbs and mindbenders, and exercises bomb
recharge and charge release. Another scenario checks local KLA spawning, hostile
AAA shells, persistent destroyed-orb objectives and repeated-credit prevention.
The engine now restricts scenery activation to the original four-tile area.

The remaining actor translations are still under inspection: their connected
state is not a claim of exhaustive native equivalence. In particular, capture/rescue handoffs, procedural training, some special effects and remaining
helper-program UI remain to be finished. Sound/music are not yet connected.

## Unified presentation and sortie results

The original unmodified VGA artwork, IMG help, story/outcome scripts and reference
BIOS fonts are embedded in a named immutable collection. Runtime rendering decodes
the original formats directly. The two story panels and fifteen outcomes retain
their stored image/text placements. One generated story composition matches the
research PNG byte for byte in RGB. Ten original in-flight help pages form one
sequence, returning to the retained flight.

VGA compositions use 320 by 200 pixels; flight/help retain 640 by 350, while BIOS
menu text uses 640 by 400 and the original 8-by-16 font. The presenter switches
texture size without changing the floating window's intended CRT aspect ratio.
The current menu is a functional native reconstruction; roster, training setup,
complete title animation and career/report flow are still being connected.
BIOS font provenance and GPL terms are retained under `third_party/`.

The original 130-byte sortie result structure now has explicit typed fields and
lossless serialization. Score arithmetic matches 512 unchanged ISCORE executions,
including signed word/dword wrapping and status-priority cases. The world writer
also reproduces all four original CCT files byte for byte after decoding, providing
the basis for persistent campaign worlds without a new packed map format.

A private X11 GUI run traverses combat selection, briefing, flight, help and quit
cancellation. Original DOS front-end inspection uses a copied installation in a
private DOSBox/Xvfb session; supplied installations remain unchanged.

## Persistent pilots and live worlds

The unified duty roster creates, activates, renames and resurrects pilots and
displays career records. Confirmed sortie endings and terminal flight states
feed the native result/score path, then show original outcome artwork and a report.
Named pilots retain changed scenery independently in each theatre across restarts.
The disposable trainee does not save a career or world. Saves use the platform
user-data directory, with a command-line override for isolated testing.

PILOT.SCR preserves the original 623-byte opaque records and both observed header
layouts. Its Borland random mask matches 16,384 bytes generated by the unmodified
MOAG routine. Existing unknown fields survive a decode/encode round trip. Newly
written THT files retain the original 671-byte container and CCT world encoding;
local filenames identify stable roster slots. Original automatic save-name
selection and every career bookkeeping field are not claimed fully interoperable
yet. Four definition files round-trip exactly, and persistence checks reopen a
changed theatre, verify rank/score, rename its pilot and switch between independent
careers. Files are replaced after a complete temporary-file write.

## Generated training and intelligence starts

INITPERMS is translated directly rather than replacing training with a saved
example world. Its fixed airstrip, random fuel targets and defence sites,
quality selection, obstacles and supporting scenery reproduce 2,157 placements
from 32 original generator runs. The native fixture supplies object allocation
only; construction and random arithmetic execute unchanged. Training begins
aboard the runway aircraft, retaining a separate spare aircraft in the world.

TWR mission names and Sxyth start coordinates are decoded as explicit data.
The F2 selector relocates the retained aircraft rather than recreating a theatre.
DONEWMISS's aircraft/damage gate, forward airflow, full throttle and retained
pitch/roll are preserved. F1 retains the original BOSS.TXT DOS-prompt disguise, F2 opens current
airbase intelligence/airborne starts, and F3
inspects the current score without ending or recording a sortie.

## Special sites and effects

Mortars launch two dangerous upward fragments on their original recharge clock.
Concealed shells reveal their quality-selected contents when struck. Generator
orbs distort the projection nearby, with special orbs also attracting aircraft;
mindbenders accumulate the original triangular palette cycling without changing
the indexed artwork. Destroyed aircraft remain wrecks when saved/reloaded or
entered, and ejection restores the original carried pistol/charge supply.

Debris no longer uses provisional counts or velocities. Its constructors match
530 particles from 64 original STRTFLT/STRTINCEND runs, covering outward/upward
emissions, mortars, incendiaries, variable time steps and random seeds. This
includes the otherwise surprising discarded random draw and low-word altitude
addition without a carry. Special-site scenarios also verify first-visit census
priming, protected objectives, attraction, barrages and shell conversion.
The connected special effects still need broader full-session comparison with DOS;
constructor equivalence alone does not establish whole-frame timing equivalence.

## Autopilot and campaign clearance

A toggles the original autopilot, including its pitch-by-speed table, feedback,
altitude capture and low-speed disengagement. All 512 isolated feedback runs
match the original code without arithmetic or helper stubs. This preserves two
quirks: a repeated word in the vertical-airflow correction and an unsigned
vertical-speed test that prevents a descending aircraft from capturing altitude.
The cockpit AUTO light follows actual engagement.

The three required normal combat theatres now feed completion medals and Combat
Master clearance. A scripted campaign persists all three, verifies that clearance
is withheld until the third, boards the White Sands experimental aircraft, reaches
the original altitude condition and records the Earth-saved/Major result. This
is a state-transition test, not a claim that an autonomous pilot completed every
objective through combat. Repeated visits do not re-award theatre medals. Detailed
original scanner bookkeeping, resurrection qualifications and reporting remain
under investigation.

## Live AdLib sound effects

The twelve original 3.ADL register patches feed Nuked OPL in OPL2 mode, with
miniaudio supplying 48 kHz stereo PCM output. Engine pressure/airflow pitch and
VOICEON packing match 512 unchanged native engine-write cases. A headless check
also synthesizes the actual engine patch and verifies finite, non-silent PCM.
Original AdLib is a mono sound path; two PCM channels do not imply new spatial
stereo effects. Dependency source pins, authorship and licenses are retained.

Successful weapon launches and explosions supply event counters. Current aircraft
state drives engine, stall and damage sounds, while local KLAs, orbs and saucers
feed their original shared channels and proximity parameters. Original native
crash-noise timing (eighteen BIOS ticks, then a 750 ms wait) continues independently
of the menu loop. Register batches cross to the PCM callback without exposing
mutable world state. Muting does not accumulate an unconsumed event queue.

The pressure/pitch protocol is independently verified; the complete combined
soundscape and interrupt-driven proximity modulation still need broader DOS
comparison. Music remains deliberately deferred. S toggles effects; --no-sound
avoids opening an output device until sound is enabled. Failure to open a PCM
device reports a warning and leaves the game usable.

## Native high-score tables

PILOT.HSC retains the original 5,040-byte layout: ten mission records and ten
entries each for cumulative and fastest careers. Fastest careers require three
sorties and use integer points per sortie. Later sorties update a named career's
existing entry. All fixed-width name padding and opaque result fields survive
loading and saving. The main menu exposes the three tables together. Codec and
ranking checks cover round trips, eligibility, separate rankings and restart
persistence; ranking is not yet independently compared with native MOAG execution.

## Unified Earth-saved ending

The experimental-aircraft altitude transition now enters the original SRATS
starfield before the Earth-saved artwork and sortie report. It retains the
indexed cockpit and current palette, shrinks the Earth in the rear window and
fades only the sky colour over eight seconds. Enter toggles accumulating star
trails; Escape continues to the outcome. The retained 3.42 executable flashes
white after 270 warp ticks, then fades out, a path that differs from the later
registered-build conditional in the published source. Both EGA pages retain
their own trails. BIOS-tick animation is independent of stopped flight state.

All 768 initial star coordinates and 64 complete indexed star frames match
unchanged SRATS execution using the development EGA adapter. The combined
window fills, fade and handoff also have scripted coverage; this does not claim
a pixel comparison of the entire original ending with inherited instruments.

## Animated forcefields and saucers

Wire barriers now follow the three original independent size cycles, with matching
collision dimensions for each frame. The saucer's upper and lower lights pulse
in opposition. Geometry bindings are decoded once from the original pointer
tables; static model data stays cached. All changed wire vertices, collision
components and saucer diameters match 512 unchanged native geometry updates.

Quality-zero filled barriers hide their distant representation until struck,
while retaining their nearby surface and collision. Contact reveals them briefly
instead of destroying them. Forcefield shutdown uses the preceding local generator
census. Scripted checks cover the reveal and retained barrier state.

## Radar, views and map inspection

The cockpit radar now shows active KLAs and saucers, relative to the aircraft
even after ejection. Its unusual square-root distance compression, word product
packing and three-pixel contact mask follow the original routines. All 512
coordinate cases and 32,768 possible compressed-distance inputs match native
execution. The existing flight magnitude checks still pass after sharing DSQRT.

Num Lock and held Control select the original eased keypad head views without
steering the aircraft. The scripted flight check verifies that looking sideways
changes the camera while preserving runway heading, then restores the forward view.

M enters the original overhead map, starting at 210 high words of altitude. It
uses the existing software renderer, all stored tiles, survey-marker blinking
and the authored player marker. Its apparent-depth cap at 35 high words matches
512 unchanged REL3D executions. Map controls pan in proportion to altitude, zoom,
rotate with Shift and centre Home while the sortie remains frozen. Training
retains the original restriction. This is not a full native map pixel comparison.

V now calls a quality-seven rescue van when stranded on foot, bringing a distant
spawn into range as the original does. A scenario calls one van, rejects duplicate
calls, walks to it and reaches the rescued sortie result. Hostile vehicles now
pull the pilot toward them instead of declaring capture at the larger approach
radius; capture itself uses the original object collision bounds.

## In-flight guidance and exit paths

Eleven BLACKBOX message records now decode directly from the embedded data segment,
retaining the original red/black rectangles, BIOS positions and text. Mission
selection, objective completion, low-altitude and ground-steering events queue
their corresponding messages. Map instructions acknowledge into the map instead
of dropping the request. O toggles messages, including while dismissing a box.
The viewport's original black homing chevron and home-range radar-dish blink are
connected. Control-C has a distinct abort result and zero native score.

A private X11 run inspected the actual mission box, map, boss screen, help, score,
abort outcome and report without changing supplied DOS installations. Headless
interface checks decode/render every message and verify acknowledgements return
to the retained flight. These checks do not establish every original warning's
full-session timing, and the existing explanatory Escape confirmation is still
a native-interface adaptation.

## Ejection and parachute motion

Airborne ejection now retains the original tumble and saved attitude. Opening
the parachute waits eight frames, restores that attitude and applies the original
settling swing; moving the controls interrupts it. One source quirk is retained:
the first tumble component is added to a register after yaw has already been
copied elsewhere, so only the second changes the view. The native 100-unit walking
heading wobble replaces the earlier provisional value.

Free-fall damping and parachute swing each match 512 original executions, including
word truncation, even-only horizontal velocity, the swing envelope and interruption
threshold. The connected walking/flight/ejection and rescue scenarios still pass.

## Saucer movement

The full living saucer handler now matches 512 native executions across all eight
qualities, including stationary starts and the objective saucer's protected rest
state. The stored modes select retreat, circling, approach and coasting in that
order. These checks cover altitude correction, wave motion, transition randomness
and speed limiting; weapon constructors and radar/audio are isolated explicitly.
Active control towers also update the current airbase coordinates, as in TOWERTIM.

## Ground guns, balloons, bees and aliens

Another 1,536 unchanged native handler executions cover all eight qualities,
AAA tracking and target leading, shell construction, balloon bounds and random
reversals, bee orientation, and alien activation/hopping/removal. The target
component cutoff is eight high words (524,288 cm), rather than the provisional
doubled range. Ordinary contacts do not destroy barrage balloons.

Bees consume three random angles before their shared balloon update. The final
RAND_W carry flag accidentally decides whether that update takes its contact
return, skipping vertical motion; this original dependency is retained explicitly.
The reference harness supplies effect allocation and isolates AAA score reporting;
movement, angle math, relative vectors and random arithmetic execute unchanged.

An initial complete run of the existing 29 checks passed with both address and
undefined-behaviour sanitizers, including the all-theatre rendering sweep. This
is memory/arithmetic coverage, not a claim of full game equivalence.

## Shared gameplay stream and sustained inspection

Aircraft damage/ejection, actors and weapon/effect constructors now consume one
shared native random stream instead of restarting separate streams. Saucer death
uses the original debris-before-spin random order; ground impact emits upward
fragments and fifteen incendiaries rather than a placeholder explosion.

A sustained check runs 32 original-theatre starts with mixed flight controls,
guns, missiles, bombs, ejection, parachuting and remote control, rendering along
the way and round-tripping the resulting worlds/results. It exposed a crash
when a hidden far-tier barrier supplied the original null model pointer; null
models now correctly contribute no drawing commands. All 31 checks, including
this longer run, pass under address and undefined-behaviour sanitizers.
The renderer retains a separate visual random stream, and complete original
frame ordering is still under comparison.
