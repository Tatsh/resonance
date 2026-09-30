<!-- markdownlint-configure-file {"MD024": { "siblings_only": true } } -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.1/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [unreleased]

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

## [0.0.1] - 2026-00-00

First version.

[unreleased]: https://github.com/Tatsh/resonance/compare/v0.0.0...HEAD
[0.0.1]: https://github.com/Tatsh/resonance/releases/tag/v0.0.0
