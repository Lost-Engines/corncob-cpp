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
The animation word restarts at 128 on reaching 16,384, as in the native loop;
512 unchanged counter cases and a connected long-flight check cover the restart.

Weapons use the original separate 32-bullet and 320-effect pools: guns/cannon
mode, finite pistol ammunition, unguided missiles, replenishing bombs and the
planted assassin charge. Effects include debris, incendiaries, mortar barrages,
concealed sites, orbiting/swaying enemies, forcefields, generator attraction and
mindbender palette effects. Aircraft damage retains its separate engine, wing,
rudder and stability channels rather than generic hit points.

Destruction retains each target's own wreck, fragments, weapon restrictions and
score credit. Surviving saucers tilt when struck; launch sites reset their rest
intervals and vehicles retain the same pursuit reset used by ordinary movement.
Shards retain their authored initial tilt, 6,200-frame lifetime and original
per-slot red/yellow flicker, becoming black on the ground. Explosions retain their
random drift. Anti-aircraft shells use the original nine-fragment damaging burst;
bombs preserve the native crater transition and time-adjusted lifetime.

All 704 independent hit callbacks and 128 paired SLAM contacts match unchanged
native executions: both object records/models, all 130 score bytes, 3,772 emitted
effects and complete PRNG continuation. A second collision callback sees the
first object's changed state. TETTIM also leaves its scratch pointer at a harmless
fragment, so SLAM can skip the original projectile's second callback; this quirk
is explicit rather than accidentally depending on a C++ reference. The fixture
supplies effect allocation only. Separate comparisons cover 192 debris/shard/
explosion constructors and 512 shard colour passes. These checks do not establish
identical ordering of all contacts within a complete moving scene.

Aircraft damage/ejection, actors, weapon constructors and scene drawing share one
random stream. Each simulation pass draws once, after nearby handlers and before
portal transfer/player physics; additional host refreshes reuse the retained
flight page. Paused/developer previews copy visual state and randomness instead
of advancing the sortie. Connected comparisons verify identical gameplay and
simulation frames despite extra previews. Exact interleaving of individual
original painter-table handlers and primitives remains under investigation. The saucer's modes select retreat, circling, approach and coasting around
the current airbase. Its death uses debris-before-spin random ordering; ground
impact produces upward fragments and fifteen incendiaries. Barrage balloons
survive ordinary contacts. Bees retain the accidental RAND_W carry-flag dependency
that sometimes skips their vertical update. Dead aliens disappear outside their
near tier. Launch sites retain quality-specific reset/retry behaviour and update admission
counts immediately, including failed allocation attempts. Vehicle sites wait
three reset periods; saucers retain their original 32,000-frame lifetime and
unshifted constructor position. A pass captures its existing active actors before
handlers run, so a newly constructed child first updates when admitted on the
following pass, as with EXPMOV's original painter-table insertion. The new-object
bit is consumed by effect movement. Completed destroyed orbs retain DHQTIM's repeated spin/gravity assignment and
settle at 3,100 cm after crossing its 11,200 cm threshold. Stored objects use their
mutated motion fields, so the drop actually progresses across frames. Near/far
orbs and mindbenders overwrite their shared visual effects in handler order;
objective callbacks retain the previous census and one-time credit. All 768 native
orb/objective/mindbender cases match without stubs, including preceding-orb effects,
all qualities, attraction, regenerated health and PRNG continuation. Remaining
actor equivalence is under comparison.

Portal quality rows now drive their original transfer, damage, repair and objective
bits. Transfers occur on the following pass, preserve full coordinate words, consume
four random draws in the original order, and arrive at 229,376 cm. Repair preserves
historical worst damage and invulnerability. Noncollision portal updates retain
the original timer, completed-white/stippled flash colours and PRNG continuation;
each portal retains the colour selected by its handler for its model primitives.
The destination block matches 512 native cases and the complete noncollision
handler matches 256; connected cases cover objective credit, delayed transfer,
repair, and a damaging portal followed by a repair portal.

The original autopilot includes pitch-by-speed feedback and altitude capture.
Optional beginner aerodynamics adds the original weaker PITCHLOCK correction.
When both are enabled, the two original calls remain. The standalone engine defaults to advanced; MOAG creates pilots with beginner
aerodynamics. The unified launcher follows MOAG; `--flight-model advanced`
overrides the saved pilot setting. The controls setup selects the same
model and keyboard/joystick choice before a new flight. The global keyboard/joystick choice uses header byte two of PILOT.SCR; the
pilot’s model uses bit `0x20` at `0x23b` (set for advanced). New pilot setup
bytes match an unchanged MOAG-created record. Explicit device indices remain
startup choices because DOS only stores keyboard versus joystick.

Parked aircraft now run the original placement/repair handler. Eligible standard
and reserve aircraft line up 7,200 cm apart; a new arrival takes a positioned
normal aircraft to the runway automatically. Experimental and permanently
crashed aircraft retain their distinct paths. The tower keeps parked poses within
4,200 scaled distance units and resets them outside that radius; its separate
5,200 limit controls home/radar status. A manually retained or abandoned aircraft
keeps its pose until the original reset clears its placement lock. All 512 native
parked-aircraft cases and 256 tower reset cases match; connected checks cover the
normal arrival, stepping out and avoiding immediate automatic re-entry.

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
jump restriction and message. An airborne start explicitly releases the stopped
runway latch so movement begins without another throttle input. F3 reports the retained sortie without recording it.
F4 traverses ten original help pages. Pause also accepts Space for help and
P/Escape to resume, as documented on those pages. Eleven BLACKBOX records supply native warning
rectangles, text and placements. Flight Escape warns once when messages are enabled
and the pilot is beyond the tower's 5,200-unit range; a later Escape ends the sortie.
Control-C follows the distinct uncounted abort path.

Permanent scenery retains its original timer refreshes and nearby-update gate.
Ground crew keep their latched gesture or corpse pose; spy contact awards credit
once, and radar rotation follows the original fast tick word. Local contacts use
EVALDIST’s word-coordinate gate and integer magnitude, including aircraft boarding
and the separate departure/re-entry thresholds. All 576 complete native scenery
handlers and 512 distance cases match without stubs.

Walking into a tower opens intelligence and pauses the retained simulation.
DOTOWER continuation places the pilot outside the doorway, repairs the retained
aircraft and recharges its rocket, preserving historical worst damage. It does
not end or record a sortie. Invulnerable ground crashes level and bounce the
plane through RESETPLANE/BOUNCEVERT instead of killing the pilot. Rescue calls
create one friendly van; hostile vehicles draw an exposed pilot toward actual
capture collision bounds.

Named pilots retain changed worlds independently. Roster operations create,
activate, deactivate, rename, delete and resurrect pilots; records display career
statistics and original medal glyphs/colours. Three native high-score lists cover
missions, careers and integer points-per-sortie careers with at least three
sorties. The three required normal theatres grant Combat Master clearance; the
experimental aircraft's altitude condition leads to the Earth-saved ending.
Invulnerable and aborted flights leave career bytes and high-score lists unchanged.
World persistence remains separate from career credit.

All 5,040 bytes of the high-score update match 96 native insertion cases. Equal
points use objective, ground-kill and air-kill word totals as successive tie
breakers; equal integer career rates use total points. Signed mission counts,
retired names, empty rows and padding are retained. Existing careers only move
upward, including the original odd ordering after a declining average. Original
theatre titles are recorded. White Sands exceeds the original 22-byte theatre
field and overwrites the first statistics bytes; this saved-record quirk is
reproduced explicitly within bounds, without unsafe string writes.

PILOT.SCR retains opaque 623-byte records, both observed header layouts and the
original Borland random mask. PILOT.HSC retains its 5,040-byte layout and padding.
THT uses the native 671-byte container and compact 64-tile world stream. Both plain
and coordinate-XOR type words are accepted only after a complete world validates;
new saves use XOR. All four original definitions round-trip byte for byte. Save
replacement uses a completed temporary write. New worlds use numbered `THT/N.THT`
files linked by the pilot’s original twenty-entry table (`0x23c` count, `0x23d`
active index, `0x23e` file numbers). References preserve ownership even when DOS
has left the embedded pilot name stale. Earlier native slot-named files remain
readable. Header campaign words are preserved, though their full bookkeeping
is still being investigated. Renaming updates live high-score names; deletion
removes owned worlds and sets the original retired word in all three score lists.
A reused name starts a distinct career. Training counts/qualities and invulnerability
also use the original per-pilot setup bytes. New pilots retain the native five KLA,
six AAA and two vehicle sites, with strength-four vehicles and random KLA/AAA
strengths. The original menu limits each site count to twenty-five.

Career records retain distinct latest sortie, highest-scoring sortie, individual
peaks and cumulative totals; PgUp/PgDn switches between them. Native comparisons
cover all 540 bytes of counters/statistics/scores, including signed peaks and word
wrapping. Cumulative damage adds each sortie's maximum, rather than storing a
second peak. MOAG's campaign score is intentionally separate from ISCORE's flight
report: it adds successful suicide-objective credit and its ejection-factor test
contains an always-false mask. Both original paths are preserved.

Elapsed flying time is recorded in sortie and career statistics. Map inspection
counts toward flying time; pause, help, radio, tower and menu handoffs commit
whole seconds and stop the clock. Time advance affects physics without multiplying
flying time. The original elapsed-time helper ignores hours within each uninterrupted
segment and wraps its flight result at sixteen bits; both quirks are retained.
The two adjacent saved words are treated as a cumulative dword by MOAG.

The original campaign scan is now executed independently for 512 promotion,
medal and court-martial reference cases, including cached theatre lookup and
native string comparisons. Saved theatres keep their own career totals instead
of a copy of the global career. The completion word at header offset 46 prevents
repeat awards; offset 40 stores remaining objectives. A completion needs at least
one credited objective, and only the current, newly completed theatre grants
Theatre Combat/Distinguished Flying Cross medals. The latter requires no aircraft
lost across that theatre, rather than merely a clean final sortie. Combat Master
requires all three normal theatres and a surviving pilot. Friendly losses suspend
a living pilot for one day per loss, extend an existing sentence and skip that
sortie's awards. The release date survives restarts. Killed/captured pilots remain
eligible for the original resurrection operation; living pilots do not.

## Rendering, presentation and sound

The software renderer retains 640 x 350 indexed EGA output, three distance/detail
tiers, authored octant painter lists, fixed-point projection and focal-plane
clipping. Gauge lines use EGALINE's direction-normalised pixel decisions; model
wires use KLINE's horizontal spans and long shallow-line runs. Both paths match
304 complete native indexed frames. Wire viewport clipping retains the original
inclusive borders and X-first/Y-second order, verified against 512 native cases. OpenGL only presents the CPU framebuffer. Floating window sizing fits
available desktop space and preserves the intended 4:3 CRT shape. VGA presentations
use 320 x 200; menus use 640 x 400 BIOS text. Nearest-neighbour sampling preserves
pixel boundaries. The original cockpit mask's stray diagonal is retained.

Ground-level runway pieces use CHECKFORRUNWAY's preliminary drawing pass;
elevated runway pieces retain ordinary distance ordering. Forty-eight scene
regions match unchanged DRAWNEAR calls across all three runway types, both
altitudes and eight view directions. The grey gunsight also appears on foot,
hides while walking and returns when stopped. Both viewport layouts match
complete original DRAWGS frames.

The control-tower door showing through its side/back walls is also an original
rendering quirk. The tower and doorway are separate objects: TBSORT compares their
centre distances, and the elevated tower centre causes its walls to be drawn
before the ground-level doorway. The doorway's polygon is drawn from either side.
Executing unchanged DISTEVAL, TBSORT and D3DOBJ reproduces this at eight ground-level
viewpoints; sixteen central scene regions (with and without the door) match the
reconstruction exactly after normalizing sky/ground to the probe's flat background.
[The fixture metadata](../tests/reference/native_tower.json) records the routine
addresses and scope. This quirk is retained for fidelity; it does not establish
identical painter-table ordering for complete moving scenes.

Cockpit buttons retain all fourteen native search masks and colour substitutions;
56 complete native frames cover their exact bounds. Bomb-supply annunciators,
flashing low-altitude/eject warnings, high-altitude marks and the rocket needle's
charging/ready/firing colours follow live state. Low-altitude hysteresis remains
active when explanatory message boxes are disabled. Autopilot and objective lamps
retain their original blue and bright-blue backgrounds.

Rocket charge follows the original BIOS clock, including time-advance scaling and
its overflow guard. The original leaves the stored charge unchanged when the
next increment crosses the readiness threshold; the green lamp and ignition
condition follow that branch. Discharged rockets recharge and cannot restart
on a partial charge. Native comparisons cover 1,024 clock updates, with a full
connected burn/recharge/reignition scenario.

The original per-frame particle block changes all thirty-five large-cloud,
small-cloud and flotsam discs, including their individual positions and diameters.
It consumes 141 shared random draws after physics and supplies the following
frame, with unchanged authored colour references. All 512 complete native blocks
and the model-resource bindings match. Previously these clouds retained static
geometry. The shared random headquarters colour now also comes from that original
block rather than an extra draw during display refresh.

Ground dots and stars now use their runtime generators rather than a fixed star
list. Complete startup retains all three eighty-point banks, including the unused
far bank and final generated records beyond the draw loop. The 512 ground-point,
256 star and 64 complete initialisation cases match native positions and PRNG
continuation, including signed range wrapping and underground no-ops.

Original instruments, propeller, ground points, stars, sun directives, radar,
homing pointer and animated model geometry are connected. Wire forcefields pulse
with matching collision bounds; saucer discs pulse in opposition. Quality-zero
filled barriers reveal their distant representation briefly on contact. Null
model pointers intentionally draw nothing. Radar retains its unusual square-root
range compression and draws relative to the aircraft after ejection. Num Lock and
Control select eased head views. The overhead map retains depth reduction, stored
survey/player markers, pan/zoom/rotation and frozen sortie state.

High-G vision filters the original vertical force accumulator before reducing the
six-bit DAC palette. Capture retains its fractional fade word, including native
word wrapping; ejection and tower return restore the intensity byte. Each scene
uses the resulting original quantised palette rather than an RGB overlay.

Original VGA/IMG data and presentation scripts supply two story panels, fifteen
outcomes and ten help screens. MEDAL.FNC replaces sixteen BIOS glyphs; ten award
entries use MOAG's original pair/colour table at file offset `0x1fa82`.

The story now precedes the animated CCTITLE sequence, with its sixteen-step DAC
fades. The flyby retains reflected terrain scrolling, plane and saucer sprites,
flak, muzzle flashes and the initial single pass. Later repeats retain the
original three-pass continuation. Lettering keeps five moving cloud-detail bits
and replaces the upper three index bits from the authored letter masks; its
separate GRADSKY palette, threshold fade and scrolling credits are retained.
The per-frame motion, state transitions and Borland PRNG continuation match
1,024 unchanged native passes; 6,144 composited pixels match original VGA writes.
The original CRTC page setter does not wait for retrace, so playback here uses a
stable 70 Hz clock rather than depending on CPU or DOSBox cycle speed. Original
presentation sound/music is not part of this visual reconstruction. Whole title
frames, including sprite clipping and text rasterisation, are not yet claimed
pixel-exact.

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
world state. Muting does not queue stale events. Proximity inputs now come from
the actual actor handlers, preserving their overwrite order, KLA launch rest and
persistent quality. They are latched for the following frame, as in the original
pre-DRAWNEAR sound block. Radar-derived saucer attenuation uses compressed
horizontal range relative to the retained aircraft after ejection. Saucer pitch
tracks remaining health before objective-saucer repair; orb and KLA ranges keep
their distinct original scales. Orb-to-saucer transitions restore the intervening
screech patch, and KLA shutdown retains native VOICEOFF packing. All 10,535 OPL
writes in 512 connected native proximity frames match; 384 ordered full actor
passes independently verify the range/pitch inputs with real RADARPROCESS maths.
Audio is updated per simulation pass, so catch-up frames retain their events.
Complete combined DOS soundscape and interrupt behaviour still need broader comparison.

## Verification

Fixtures execute unchanged target routines in development. Their JSON metadata
records executable/fixture hashes, addresses, scope and any explicit stubs. Ordinary
CTest runs are C++ only; checks do not merely compare two translations of a formula.

| Area | Independent native coverage |
| --- | --- |
| Fixed maths | All 65,536 sine inputs; 8,228 airflow angles; 512 forward/inverse rotation sets; 512 physics and fast vertex transforms each; 128 matrix compositions |
| Landing maths | 1,024 magnitudes; 256 wheel clearances; 512 suspension/brake passes; 512 invulnerable bounce cases |
| Flight | 256 force passes, including intermediate accumulators; 128 position integrations; 512 controls and attitude passes each |
| Portals | 512 unchanged destination/PRNG cases and 256 complete timer/colour updates; connected delayed objective transfer and damage/repair |
| Rocket recharge | 1,024 native BIOS-clock updates, including time advance, wrapping and readiness; connected discharge/recharge/reignition |
| Autopilot/beginner | 512 feedback cases covering both models, stall state and engagement combinations |
| Joystick | 512 SPKE/JOYTD response cases, plus keyboard/button/head-view integration |
| Person | 512 free-fall damping and 512 parachute swing cases |
| Motion/collision | 512 generic movement, 512 strict boxes, 512 fast and 512 ordinary projectile contacts |
| Damage/actors | 256 aircraft damage passes; 512 KLA and 512 living saucer passes; 512 vehicle updates with capture/rescue flags and PRNG continuation; 1,536 AAA/balloon/bee/alien updates, including shell initialization; 768 launch-site passes and 133 complete child constructors, including exhausted pools; 768 orb/objective/mindbender passes without stubs; 256 mortar passes and 104 complete emitted particles |
| Destruction | 704 hit callbacks and 128 paired contacts; complete object/models, 130 score bytes, 3,772 emitted effects and PRNG continuation |
| Scenery | 512 parked-aircraft handlers, 256 tower reset cases, 576 complete permanent handlers, 512 local interaction ranges and connected tower/boarding/contact checks |
| Debris/training | 192 complete debris/shard/explosion constructor runs; 512 shard colour/PRNG cases; 2,157 generated placements from 32 training runs |
| Raster/resources | All 224,000 cockpit pixels; 30 complete disc masks, 12 polygon hashes, 304 line frames and 512 wire-clip cases; four original definition round trips; one 252-object plain saved world |
| Ground/stars | 512 point regenerations, 256 stars and 64 complete three-bank initialisations with PRNG continuation |
| Particles | 512 complete 35-disc/shared-colour frame blocks and PRNG continuations, plus resource bindings across the original models |
| Model/map/radar | 64 tower/doorway/runway scene regions; 512 geometry-animation, 512 map reductions, 512 radar positions; all 32,768 compressed radar-distance inputs |
| Visual feedback | 512 force/fade states and 512 complete DAC palettes, including capture and frame wrapping |
| Title | 1,024 original animation-state/PRNG passes and 6,144 VGA letter-mask pixels |
| Ending | All 768 initial stars and 64 complete indexed native star frames |
| Animation clock | 512 native word-counter transitions and a connected 16,384-pass restart |
| Sortie clock | 512 unchanged two-segment DOS-clock conversions, including pause boundaries and wrapping; connected flight/pause/map timing |
| Score/persistence | 512 ISCORE calculations, 256 complete MOAG career updates and 512 complete promotion/medal decisions; 16,384 original mask bytes; original control/model/new-pilot fixtures; 96 full 5,040-byte native high-score insertion/tie/padding cases and exact retirement; roster/world restart, rename and deletion scenarios |
| Sound | 512 engine/register-write cases; 10,535 proximity writes across 512 connected frames; 384 ordered actor-proximity passes; finite, non-silent PCM |

Connected scenarios cover takeoff, ejection/parachute/remote control, weapons and
persistent objectives, rescue/capture, damaged-aircraft tower visits, settings,
career clearance and the ending. A sustained run exercises 32 original-theatre
starts with mixed controls/weapons/ejection, more than 31,000 updates and 250
rendered frames, then round-trips resulting worlds/results. An additional
`endurance_checks --draw-every-frame` run completed 31,802 full simulation/render
passes across the same 32 starts in Release and under AddressSanitizer/UBSan.
The original sustained run exposed the hidden
far-model crash before interactive playtesting.

Private GLFW/X11 checks traversed pilot creation, mission selection, radio boxes,
map, boss screen, help, score, abort/report, tower intelligence, repair/resume and
Escape warnings, followed by both story fades and the animated title/credits. A further window
traversal checked the saved training defaults, live bomb-supply indicators, flap
state and cockpit warning colours. Pause/Space help, help-page continuation
and P/Escape return were also inspected in the actual window. A clean Release
installation contains only the embedded executable and opens without the research
workspace or external game assets.
Captures were inspected without changing supplied DOS installs.
The complete 52-check Debug, Release and sanitizer suites pass, including the
longer scenarios, vehicle/launch/orb handlers, visual feedback and animated title.
A local MinGW GCC 16 Release build also passes all 52 checks under Wine; its actual
window was inspected through menus, training, flight and stepping out on foot.
It imports only Windows system DLLs. This does not replace testing on Windows
hardware. Complete frame ordering, macOS runtime and interactive Wayland sizing
are not yet independently established.

Ground vehicles use the original renderer distance tiers for speed/pursuit, with
actual distance checked separately for capture. Near stopped vehicles bypass the
random reset draw; other resets preserve the original follow/random ordering.
Friendly and hostile stopped vehicles have distinct paths, and expired wrecks
are removed. All 512 native movement, capture/rescue and PRNG-continuation cases
match without stubs; the connected rescue scenario still passes.

## Remaining fidelity work

- Extend whole-frame title comparisons beyond native motion, palette masking and
  the inspected presentation captures.
- Complete remaining front-end/bookkeeping details: remaining theatre-header words and original configuration bits; broaden
  original configuration comparisons.
- Broaden complete campaign comparisons against native executions.
- Broaden complete cockpit/gauge-frame comparisons and whole scene painter/
  collision ordering.
- Broaden connected special transitions against longer DOS sessions. Scripted
  coverage is not proof of every transition.
- Refine original painter-table handler/primitive interleaving before claiming
  identical whole-session random continuation; display refreshes now preserve the
  shared simulation stream.
- Compare the complete soundscape and confirm platform/device behaviour beyond
  the Linux development host. Music remains outside the current implementation.
