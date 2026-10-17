# Corncob C++

The **Lost Engines Project**'s reconstruction of **Corncob 3D (1991–1992)**,
Pie in the Sky Software's DOS combat flight game. Its alien-invasion setting
lets you land anywhere, walk around on foot, steal aircraft and remotely fly
your own plane.

This is a ground-up engine rewrite in modern C++26, combining analysis of the
original assembly source with reverse engineering of the DOS executables.
The original game and **The Other Worlds Campaign** are included in one native
executable, with embedded artwork, worlds, music and sound effects. No DOSBox
or original installation is needed.

Visit [Lost Engines](https://lostengines.com) to explore our other
reverse-engineered games.

## Build and run

Requires CMake 3.30+, a C++26 compiler, Boost 1.85+ with Program_options, and
OpenGL and window-system development headers.

```sh
cmake -B build
cmake --build build --parallel
./build/corncob
```

On Windows with a Visual Studio generator, run `build/Debug/corncob.exe`.
The in-game documentation and flight help explain the controls. Create a named
pilot to save your career; the default trainee's progress is temporary.

## Command-line options

| Option | Purpose |
| --- | --- |
| `--help`, `-h` | Show available options. |
| `--version` | Show the version. |
| `--skip-intro` | Open the main menu without the title sequence. |
| `--slalom` | Launch the recovered source-only flying course, with six checkpoints and player weapons disabled. |
| `--graphics-mode ega\|ega-low\|hercules` | Flight display: 640×350 colour (default), 320×200 colour, or 720×348 monochrome. |
| `--theatre NAME` | Select a world, e.g. `white_sands` or `other_worlds/iceworld`. |
| `--list-theatres` | List all world identifiers and exit. |
| `--flight-model beginner\|advanced` | Override the saved pilot's flight model; otherwise defaults to beginner for new pilots. |
| `--joystick auto\|none\|N` | Select automatic joystick discovery, keyboard controls, or device index 0–15. Otherwise uses the saved control choice. |
| `--list-joysticks` | List connected joystick indices and names, then exit. |
| `--no-music` | Start with title and menu music disabled. |
| `--no-sound` | Start with sound effects disabled. |
| `--scale N` | Set the initial integer enlargement; normally the largest that fits the desktop. |
| `--aspect square\|crt` | Use square pixels (default) or a 4:3 CRT display aspect. |
| `--save-dir PATH` | Choose where careers and worlds are saved; defaults to the platform's user-data directory. |

The optional slalom course comes from an experimental branch in the preserved
source. Run `./build/corncob --slalom`, optionally with
`--graphics-mode=ega-low` or `--graphics-mode=hercules`. Pass the six markers
in order, alternating sides; the six cockpit lights show progress. Leaving the
course closes that run without saving it into a pilot's career. Alternate flight
modes use the recovered projection settings with adapted cockpit artwork.

## Differences from DOS

The aim is to retain the original gameplay, flight model and visual style.
The main changes are:

- Earth, Other Worlds, practice worlds and recorded demonstrations share one
  menu, with separate careers for the two campaigns.
- A resizable window scales the original pixels without smoothing. Joysticks use
  modern device input without DOS calibration.
- Saves live in your user-data directory rather than alongside the executable.
- Music plays embedded recordings of the original AdLib driver; sound effects
  are synthesised live. The title animation uses a fixed playback speed.
- Documentation exports as readable plain text. The integrated mission builder
  saves and exports worlds directly.

See the [reconstruction notes](docs/reconstruction.md) for fidelity details and
known gaps, and [Other Worlds](docs/other_worlds.md) for campaign and builder details.

## Development

Automated tests are disabled by default. Enable them explicitly when needed:

```sh
cmake -B build-tests -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

For an optimised build, configure with `-DCMAKE_BUILD_TYPE=Release`.
`RelWithDebInfo` retains assertions and developer tools while enabling optimisation:

```sh
cmake -B build-rel -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-rel --parallel
```

Additional CMake options:

| Option | Purpose |
| --- | --- |
| `-DBUILD_TESTING=ON` | Build automated tests; disabled by default. |
| `-DGLFW_BUILD_WAYLAND=OFF` | Disable the Wayland backend on Linux. |
| `-DGLFW_BUILD_X11=OFF` | Disable the X11 backend on Linux. |
| `-DCORNCOB_STATIC_RUNTIME=ON` | Link compiler runtimes statically; enabled by default for MinGW. |

Developer command-line options are available in Debug and RelWithDebInfo builds:

| Option | Purpose |
| --- | --- |
| `--free-camera` | Inspect scenery with a freely movable camera. |
| `--no-cockpit` | Use the original full-screen flight viewport. |
| `--cockpit-preview` | Display the raw cockpit artwork. |
| `--screenshot FILE` | Save the initial flight scene as PPM and exit without opening a window. |
| `--seconds N` | Close automatically after N seconds; zero disables the limit. |

[Reconstruction reference](docs/reconstruction.md) ·
[Resource provenance](assets/README.md) · [C++ style guide](style-guide.md)
