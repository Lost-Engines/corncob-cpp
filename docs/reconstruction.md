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

Next: reconcile original renderer symbols with the reference disassembly,
translate fixed-point transforms/projection and rasterisation, embed White Sands
and model data, then bring up the original aircraft update with native comparisons.

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
