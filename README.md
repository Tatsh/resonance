# FreQuency

Reconstructed source code for _FreQuency_, the 2001 PlayStation 2 rhythm game developed by Harmonix
Music Systems and published by Sony Computer Entertainment.

The game shipped as a compiled disc, and its source code was never released. This project rebuilds
that source by reading the shipped program instruction by instruction and writing back the C++ it
was compiled from. Nothing here is decompiler output: every file is written by hand to match what
the original program does, and every routine records the address it was recovered from.

## Status

This is an active, partial reconstruction. It does not build a playable game yet, and it is not a
port. What exists today is the start-up path and several complete subsystems.

| Area | State |
| --- | --- |
| Start-up and the application shell | Recovered |
| Memory, archives, and file loading | Recovered |
| Renderer object model and streams | Recovered |
| Meshes, materials, and textures | Substantially recovered |
| Game messages | Partially recovered |
| Embedded Python interpreter | Characterised, with its differences recorded |
| Gameplay, audio, and networking | Not started |

## Layout

```
include/   headers, grouped by subsystem
src/       implementations, mirroring include/
3rdparty/  third-party code the game linked against
```

Within those, `rnd` is the renderer, `os` the memory and file layer, `app` the application shell,
`msg` the message classes, `math` the vector and transform types, and `gfx` the display device.
`src/python` records the differences between the game's embedded Python interpreter and the public
release it was built from.

## Reading the source

Two conventions make the tree navigable.

Every reconstructed routine records the address it came from, as
`@ghidraAddress 0x004b7ad0`, so any claim in this tree can be checked against the shipped program.

Where something could not be recovered with confidence, the source states so rather than guessing.
A field whose purpose is unknown is titled `mUnknown1c` and retains its offset, and a routine that is
understood but not yet written is described in its class documentation with its address. Gaps are
marked deliberately, because an invented detail is harder to find later than a missing one.

## Building

There is no build yet. The original was compiled for the PlayStation 2 with Sony's toolchain, which
this tree cannot reproduce, and several subsystems are still missing. The source is checked for
syntax as it is written.

## Provenance and licence

This is an independent reverse-engineering effort for preservation and study. It includes no code
and no assets copied from the game. _FreQuency_ and its assets remain the property of their
respective rights holders, and this project is not affiliated with or endorsed by Harmonix or
Sony Interactive Entertainment.

Third-party code the game linked against, including the Python interpreter and its C++ binding
layer, remains under its own licence and is identified as such under `3rdparty/` and `src/python/`.
