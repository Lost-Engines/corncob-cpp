# Embedded original resources

`cockpit.img` is the unmodified `IMG/3D.IMG` from Corncob 3D 3.42.
SHA-256: `ead2f72e60ee99ab84ff47e8e0e00d7ed5aca67e0bb109923dc0d4a90cfa7c48`.
The shareware and Personal Companion retail 3.42 distributions share this resource.
Original game graphics and data are by Pie in the Sky Software and retain their
original copyright; any licence for the new C++ code does not relicense them.

CMake embeds the compressed bytes in the executable. No game installation or
Python interpreter is required to build or run it. The decoder follows RIMG's
repeat/literal stream; the regression check matches the 28,000 decoded bytes
previously verified against execution of the original DOS decoder.

`engine.dat` contains the original 3.42 data segment: model primitives, object
templates and tuning tables. It contains no executed original engine code.
The four `.cct` files are the unmodified base-game theatres. Their start positions,
objects and packed six-bit sky/ground colours are decoded by the C++ loader.
[provenance.json](provenance.json) records every source and SHA-256.

`tools/import_assets.py` refreshes these files from the sibling research workspace;
it is a development tool, not a build or runtime dependency.
`music/*.ogg` contains compact mono Vorbis recordings of the three original
AdLib scores. They are rendered offline from unchanged DOS music-driver commands,
with lossless FLAC masters retained in the research workspace. The recordings are
streamed from memory at runtime; the flight effects are still synthesized live.
Music is by James L. Collymore, copyright 1986–1991, and retains its original terms.
[music/provenance.json](music/provenance.json) records the source hashes, native
score boundaries and encoding settings.
