# Corncob 3D

A native C++26 reconstruction of Corncob 3D, beginning with the normal 3.42 game.
One executable will join the original front end, flight, scoring and ending.

**Work in progress:** the first build displays the embedded original cockpit
mask to verify resource decoding and window presentation. Flight, scenery,
campaign play and sound are not implemented yet. Music is deferred.

## Build and run

Requires CMake 3.30+, a C++26 compiler, Boost 1.85+ with Program_options, and OpenGL
and window-system development packages. CMake fetches a pinned GLFW 3.4 archive.
On Linux, GLFW enables both X11 and Wayland; disable an unwanted backend with
`-DGLFW_BUILD_WAYLAND=OFF` or `-DGLFW_BUILD_X11=OFF`.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/corncob
ctest --test-dir build --output-on-failure
```

The original cockpit resource is embedded. No DOS executable, external game
installation or resource download is needed at runtime.

The window uses the largest integer multiple of its 640 x 480 display that fits
within the available desktop. The native 640 x 350 EGA framebuffer is presented
at the intended 4:3 CRT aspect ratio, using nearest-neighbour sampling. The window
floats and remains resizable. `--scale 1` selects the initial scale explicitly;
Escape closes the current preview.

For optimised developer builds, use `-DCMAKE_BUILD_TYPE=RelWithDebInfo` and a
separate build directory. This retains assertions and developer screenshot/time
options. Release excludes those developer options. No automatic CI is enabled.

[Reconstruction notes](docs/reconstruction.md) record the reference version,
original-source discovery and the next implementation milestones.
