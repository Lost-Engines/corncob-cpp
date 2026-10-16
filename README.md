# Corncob 3D

A native C++26 reconstruction of Corncob 3D, beginning with the normal 3.42 game.
One executable will join the original front end, flight, scoring and ending.

**Work in progress:** the four original theatres now support walking, flight,
ejection and remote control, player weapons, active enemies, persistent named
pilots, three high-score tables and sortie reports. The original story, outcome artwork and flight help
and interactive Earth-saved starfield are integrated into the same executable. Several original systems are still
being completed and checked against the DOS game. Music is deferred.

## Build and run

Requires CMake 3.30+, a C++26 compiler, Boost 1.85+ with Program_options, and OpenGL
and window-system development packages. CMake fetches pinned GLFW, miniaudio and
Nuked OPL sources.
On Linux, GLFW enables both X11 and Wayland; disable an unwanted backend with
`-DGLFW_BUILD_WAYLAND=OFF` or `-DGLFW_BUILD_X11=OFF`.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/corncob
ctest --test-dir build --output-on-failure
```

Original cockpit, world and model resources are embedded. No DOS executable, external game
installation or resource download is needed at runtime.

The window uses the largest integer multiple of its 640 x 480 display that fits
within the available desktop. The native 640 x 350 EGA framebuffer is presented
at the intended 4:3 CRT aspect ratio, using nearest-neighbour sampling. The window
floats and remains resizable. `--scale 1` selects the initial scale explicitly;
Escape asks to end the current sortie; F1 opens the original boss screen, F2 opens
airbase intelligence and airborne mission starts, F3 shows the current score, F4 opens flight help,
and R toggles the rear view. `--skip-intro` opens the menu directly.

You start on foot. Keypad 5 walks forward and keypad 0 walks backwards; arrows
turn. Walk into an aircraft to board. +/− controls throttle, arrows fly, Z/X
operate the rudder, F toggles flaps, I uses rocket boost, and . brakes. E leaves
the aircraft; Space opens a parachute while falling. Caps Lock enables remote
flight after ejection. A toggles the original autopilot. L toggles the cockpit, G ground detail, P pauses,
M opens the original overhead map; keypad arrows pan, keypad 9/3 changes height,
Shift turns the view, Home recentres and Space returns. Num Lock or held Control
switches the keypad to looking around. V calls a rescue van while stranded on
foot. S toggles AdLib sound effects. `--no-sound` starts with effects disabled.
Original message boxes explain mission selection, map controls and warnings; O
toggles them. The homing pointer along the viewport border points back to base.
Control-C aborts a sortie without awarding points.
The experimental aircraft at White Sands requires campaign clearance before
its throttle operates. Create or activate a pilot through the duty roster to save
your career and changes to each theatre. The default trainee does not save progress.
`--save-dir` overrides the platform user-data directory.

The training menu generates the original practice world with selectable defence
counts and qualities. Combat theatres also retain their original intelligence
briefings and airborne starts; mission jumps require an aircraft with less than
16 damage and keep the current world intact.

Choose a world in the menu, or start directly with `--theatre white_sands`, `morning`, `dusk` or `wasteland`.
Debug and RelWithDebInfo also provide `--free-camera` for scenery inspection:
WASD moves, arrow keys turn, Page Up/Down changes height, Shift moves faster and
Home resets the camera. `--no-cockpit` uses the original full-screen view.

For optimised developer builds, use `-DCMAKE_BUILD_TYPE=RelWithDebInfo` and a
separate build directory. This retains assertions and developer screenshot/time
options. Release excludes those developer options. No automatic CI is enabled.

[Reconstruction notes](docs/reconstruction.md) record the reference version,
original-source discovery and the next implementation milestones.
