<!-- markdownlint-configure-file {"MD024": { "siblings_only": true } } -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.1/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [unreleased]

### Added

- An `iso` CMake target writes a bootable disc image with the built executable and `EZMIDI.IRX`
  through `scripts/build-iso.py`. Set `RESONANCE_DISC_IMAGE` to an original disc image to enable
  it.

### Fixed

- The rebuilt executable no longer crashes in `InitIop`. The `libdma`, `libgraph`, and `libdev`
  reconstructions read data at the original image's addresses, and those addresses fall inside the
  heap in the rebuilt layout. They now use tables with the original contents.
- `sceDmaReset` returns the original value.
- `libgraph` now matches the original in these areas:
  - The vertical-blank wait polls the correct register.
  - The frame page count test checks the correct mode.
  - GIF waits in `sceGsPutDrawEnv` and `sceGsExecLoadImage` allow 0x1000000 polls instead of 256.
  - `sceGsExecStoreImage` recovers from a transfer timeout as the original does.
  - Error messages, including the `sceGsSyncPath` message, match the original.
  - `sceGsSetDefClear` sets the second clear address.
  - `sceGsSetDefDBuff` writes the second buffer's page into the display frame buffer register.
  - `sceGsSetDefDispEnv` selects the display offsets by the interlace mode and places the display
    width in its field.
  - `sceGsSetDefDrawEnv` sets the dither register address for every pixel format.
  - `sceGsSetDefLoadImage` and `sceGsExecStoreImage` size transfers for the correct pixel formats.
  - `sceGsSetDefStoreImage` sets the register count in the GIF tag.
  - `sceGsExecStoreImage` masks the image height and receives the trailing quadwords.
- The `libdev` console heap allocator uses the original masks.
- `libvifpk` packet alignment no longer shifts by 32 bits when the boundary mask is empty.
- `sceFsReset` drops the file-service binding after the IOP reboot. The next file call binds the
  service again.
- Boot-option flags match the original, with `ScreenMessagesEnabled` set to 1.
- The initial current zone is -1.
- The stack is 512 KiB at 0x01F80000, as in the original.
- `GetConnectStateMCT` and `GetAllConnectStatesMCT` test for a multitap correctly. They call
  `sceMtapGetConnection` (0x0053a920), previously misidentified as a slot-count query, and no
  longer invert the result.
- Pad reads and actuator calls go through the SDK pad library like the rest of the pad API. A
  partial copy of the Sony pad library read a table that was never filled.
- `libscf` now matches the original in these areas:
  - `sceScfEnsureRomVersionRead` returns the original value after the first call. The time zone
    and summer-time queries previously always reported the tool defaults.
  - The ROM version, the defaults, and the assertions no longer read fixed addresses.
- `libmpeg` now matches the original in these areas:
  - The default quantiser matrices are tables with the original contents instead of fixed
    addresses.
  - The picture timestamp adds 0x400 instead of storing a misread constant.
- `sceSdRemote` sends a full 0x40-byte packet from real storage through the SDK RPC client.
- The Sony libraries assert in every build type.
- File opens succeed after the IOP reboot. The SDK file client sent the ROM file server's packet
  layout to the multi-threaded file server in the replacement image, and paths arrived 16 bytes
  late (for example `Unknown device 'ING.ARK;1'`). The game now uses the reconstructed original
  file-service client.
- Standard output goes through the DECI2 TTY console as in the original and continues after the
  IOP reboot.
- C library file calls (`fopen` and C++ streams) go through the game's file layer and open ark
  members and disc paths.
- The VU0 and VU1 microprograms are reconstructed as DVP assembly, and the build assembles the DMA
  chains that upload them. VU1 previously started on empty microcode and never stopped.
- The tunnel arm emitters run for 7680 frames after their trigger, not 1920, as in the original.
- An unhandled game manager message reports its name in the fatal error, as in the original.
- `rec.bin` recordings begin with the original banner text.
- The movie stream error table has all twelve original messages. Error codes below -7 previously
  read past the end of the table.
- The PSS movie demultiplexer is reconstructed. Its bit reader and pack and packet parsers were
  stubs, and the intro movie delivered no video or audio packets. The stream key table was also
  empty, and every packet matched the first registered stream.
- The file service's init call sends its result-area pointer from a quadword-aligned slot. The
  server previously read the wrong word and wrote the first open's result to an invalid address
  (`DMA error: 1f400000` in the emulator), and the game stopped.
- The sound driver's reply buffer is 64-byte aligned, as in the original.
- `sceCdSearchFile` sends Sony's packet layout. The SDK's version placed the name 4 bytes early for
  the replacement IOP image's server, every disc search failed, and `LOADING.ARK` did not open.
- The gzip checksum no longer fails every compressed file. The inverted register the original stores
  was passed to zlib as the plain value.
- A finished asynchronous read without a callback waits for its caller to collect it. The pump
  discarded it, and the loading screen polled forever.
- With patches enabled, console output falls back to the serial port when no DECI2 host is present,
  as after the IOP reboot.
- The intro movie plays. The MPEG picture decoder is reconstructed, frame buffers no longer overwrite
  the program, and the movie's display handlers, GIF packets, and GS registers match the original.
- After the intro movie the game reaches the title screen. Message type identities have their
  original values, so the front end recognises the finished message.
- Float constants round to nearest as in the original instead of truncating.
- Global initial values match the original, including the async, MIDI, and gzip state.
- The loading screen no longer draws a white surround.
- The title screen draws its 3D city background. `sceVu0InversMatrix` computed the wrong
  translation row and placed the camera incorrectly.
- Long stray lines no longer cross the title screen city. Mesh edge uploads to VU1 sent half of the
  edge indices.
- Loading a saved game from the memory card no longer hangs:
  - `Memcard::Cancel` no longer deletes the operation in progress.
  - The minimum save space check no longer charges new-file space for files already on the card.
  - The directory entry buffer is 64-byte aligned for DMA.
- The ACCEPT and BACK help labels no longer overlap.

## [0.0.1] - 2026-00-00

First version.

[unreleased]: https://github.com/Tatsh/resonance/compare/v0.0.0...HEAD
[0.0.1]: https://github.com/Tatsh/resonance/releases/tag/v0.0.0
