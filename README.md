# Corncob 3D

A native C++26 reconstruction of Corncob 3D and **The Other Worlds Campaign**.
One executable includes both games, their worlds, training, music and sound effects.

Fly, land anywhere, walk around, eject and remotely control your aircraft. Named
pilots retain their careers, medals, scores and changes to each world. The original
story, animated introductions, help, mission reports and endings are included.

[Reconstruction notes](docs/reconstruction.md) describe the comparisons with the
original DOS programs. [Other Worlds notes](docs/other_worlds.md) cover the combined
campaigns, demonstrations and mission builder.

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

## Campaigns and worlds

Choose **Select campaign / world group** from the main menu, or press **S**:

- **Earth:** the four original theatres.
- **Other Worlds:** all thirteen supplied campaign theatre definitions, including
  the expansion's versions of the Earth worlds.
- **Practice and examples:** the empty airfield universe, demonstration world and
  empty universe.

Use Left/Right in the world selector to change groups. Long lists scroll with the
arrow keys. Open a theatre, then choose **Fly mission**; worlds with several bases
let you choose your starting airfield. Landing at another base retains that location
for your next sortie.

Pilot names are shared between campaigns, with separate ranks, medals, scores,
worlds and completion requirements. Existing Earth saves stay in their original
location; Other Worlds careers live in its `other_worlds` subdirectory.

The same menu includes the three original recorded demonstration flights, the
Other Worlds title animation, gameplay hints and mission-builder instructions.
Demonstrations do not alter your career. **Mission Builder** opens a fresh empty
world; **B** in the world selector edits a fresh copy of any selected theatre.
**F5** saves an edited world and returns to the menu. See the
[builder controls](docs/other_worlds.md#mission-builder) for editing and export details.

`--list-theatres` lists all twenty world identifiers. For example:

```sh
./build/corncob --theatre other_worlds/iceworld
```

## Playing

The window starts at the largest integer scale that fits the available desktop,
using square pixels like DOSBox with `aspect=false`. Menus are 640 x 400, flight
is 640 x 350, and presentations are 320 x 200. Enlarged pixels use nearest-neighbour
sampling and remain uniform when you resize the window; spare space forms a border.
`--scale 3 --skip-intro` opens a 1920 x 1200 menu window. `--aspect crt` enables
4:3 CRT aspect correction instead. The window floats and remains resizable.
Escape ends the sortie, showing the original one-time warning first when away
from base; F1 opens the original boss screen, F2 opens
airbase intelligence, F3 shows the current score, F4 opens flight help,
and R toggles the rear view. Walking into a control-tower doorway opens
intelligence and repairs the retained aircraft before returning to the same flight.
In intelligence, use arrows or PgUp/PgDn to scroll; Enter selects the first
highlighted mission start currently visible. Escape returns without selecting one.
`--skip-intro` opens the menu directly.

White Sands starts on foot; the other combat theatres automatically board an available normal aircraft.
Keypad 5 walks forward and keypad 0 walks backwards; arrows
turn. Walk into an aircraft to board. +/− controls throttle, arrows fly, Z/X
operate the rudder, F toggles flaps, I uses rocket boost, and . brakes.
A connected joystick is selected automatically for flight. Button 1 fires guns,
button 2 fires missiles, and Space also fires missiles in joystick mode. Arrow
keys override the stick unless used for head views. `--joystick none` keeps
keyboard controls; `--list-joysticks` lists devices for `--joystick N`.
The original menus support arrow keys, Enter and the highlighted letter shortcuts.
At an airfield with no aircraft, P draws a lower-powered reserve plane from storage;
Enter continues on foot. The detailed airfield survey lists nearby groups and objectives.
Keyboard/joystick setup selects the flight controls; Combat Missions selects the
original beginner or advanced flight model. Beginner gently assists the pitch toward level
flight and is the original launcher default. Control choice and each named pilot’s
flight model are saved; `--flight-model advanced` overrides the model at startup. E leaves
the aircraft; to re-enter it, first walk more than 40 metres away, then return
within 20 metres, as required by the original game. Space opens a parachute while falling. Caps Lock enables remote
flight after ejection. A toggles the original autopilot. L toggles the cockpit, G ground detail, P pauses.
While paused, Space opens help; P or Escape resumes the flight. M opens the original overhead map; keypad arrows pan, keypad 9/3 changes height,
Shift turns the view, Home recentres and Space returns. Num Lock or held Control
switches the keypad to looking around. V calls a rescue van while stranded on
foot. S toggles AdLib sound effects. `--no-sound` starts with effects disabled.
Title and menu music has its own saved Music toggle in the main menu;
`--no-music` starts with music disabled.
Original message boxes explain mission selection, map controls and warnings; O
toggles them. The homing pointer along the viewport border points back to base.
Control-C aborts a sortie without awarding points.
In the Earth campaign, the experimental aircraft at White Sands requires campaign
clearance before its throttle operates. Other Worlds permits it from the outset. Create or activate a pilot through the duty roster to save
your career and changes to each theatre. The duty roster also renames and deletes
pilots; deleting a pilot removes their worlds and retires their high scores.
Pilot records use Insert to switch between decorations and the full statistics
table, and Up/Down to browse pilots. Invulnerable and
Control-C-aborted flights do not count toward careers or high scores. The default trainee does not save progress.
`--save-dir` overrides the platform user-data directory.

The training menu generates the original practice world with selectable defence
counts and qualities, saved separately for each pilot. Combat theatres also retain their original intelligence
briefings and airborne starts; mission jumps require an aircraft with less than
16 damage and keep the current world intact.

Open or activate a theatre under Combat Missions, then choose Fly mission.
Training Missions has separate number and wickedness settings and a preflight
briefing. The original documentation is available from the main menu; its Save as plain text
entry saves a readable `CORNCOB.txt` in the save directory, with the full location
shown afterwards. This removes the game's highlighting markers and replaces DOS
chapter page breaks with blank lines.
You can also start directly with `--theatre white_sands`, `morning`, `dusk` or `wasteland`.
Debug and RelWithDebInfo also provide `--free-camera` for scenery inspection:
WASD moves, arrow keys turn, Page Up/Down changes height, Shift moves faster and
Home resets the camera. `--no-cockpit` uses the original full-screen view.

For optimised developer builds, use `-DCMAKE_BUILD_TYPE=RelWithDebInfo` and a
separate build directory. This retains assertions and developer screenshot/time
options. Release excludes those developer options. No automatic CI is enabled.
