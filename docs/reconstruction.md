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

The current scene preview draws stored objects without running their gameplay
update routines. Destruction/model swaps, AI, forcefield motion, instruments,
propeller and ground detail remain to be connected. Its free camera is an
inspection aid, not a replacement flight model. Walking is the original game's
initial state; the current cockpit preview is deliberately not a faithful
startup sequence yet.

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
aircraft's fourfold position scaling. This is a force/integration foundation;
control response, ground contact and the rest of the flight loop still need to
be translated before it becomes interactive flight.

The original 1,024-entry arctangent table matches the target binary too. Tests
compare 8,228 airflow angles, 256 native force passes (including intermediate
forces, acceleration, stall and rocket state) and 128 position integrations.
The first eight force passes reproduce the established research scenarios;
others exercise rotated axes, variable time steps, atmosphere, aircraft variants
and ground flags. HUD/audio calls are stubbed only in the native fixture harness.
The ordinary build runs pure C++, without emulator dependencies.
