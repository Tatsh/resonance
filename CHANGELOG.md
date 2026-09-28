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
- The `libdev` console heap allocator uses the original masks.
- `libvifpk` packet alignment no longer shifts by 32 bits when the boundary mask is empty.
- `sceFsReset` drops the file-service binding after the IOP reboot. The next file call binds the
  service again.
- Boot-option flags match the original, with `ScreenMessagesEnabled` set to 1.
- The initial current zone is -1.
- The stack is 512 KiB at 0x01F80000, as in the original.

## [0.0.1] - 2026-00-00

First version.

[unreleased]: https://github.com/Tatsh/resonance/compare/v0.0.0...HEAD
[0.0.1]: https://github.com/Tatsh/resonance/releases/tag/v0.0.0
