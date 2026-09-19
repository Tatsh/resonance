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

| Area                               | State                                        |
| ---------------------------------- | -------------------------------------------- |
| Start-up and the application shell | Recovered                                    |
| Memory, archives, and file loading | Recovered                                    |
| Renderer object model and streams  | Recovered                                    |
| Meshes, materials, and textures    | Substantially recovered                      |
| Game messages                      | Partially recovered                          |
| Embedded Python interpreter        | Characterised, with its differences recorded |
| Gameplay, audio, and networking    | Not started                                  |

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
A field whose purpose is unknown is titled `mUnknown1c` and retains its offset, and a routine that
is understood but not yet written is described in its class documentation with its address. Gaps are
marked deliberately, because an invented detail is harder to find later than a missing one.

## Compiler-generated code is never written

A virtual function table pointer, a virtual base pointer, a type function, the per-unit
static-initialisation glue, and an implicit copy or assignment body are all emitted by the compiler
rather than written by a programmer. None of them appears in this tree as a declaration or a body.
A layout comment records where the compiler placed a pointer, and nothing declares one.

The toolchain emits an inline function into every translation unit that needs it, and the linker
folds none of the copies. One class has 45 identical copies of its type function. An address count
is therefore not a function count, and a routine whose body repeats one already reconstructed adds
no source.

## STL container layout

The template library is the SGI implementation that g++ 2.9x shipped, and its layouts differ from a
modern one. A `std::list` is one four-byte pointer addressing a single self-linked dummy node.

The payload offset inside that node depends on the alignment of the element. A four-byte element
places the value at `+0x08` in a 16-byte node, which the `Rnd::Object` constructor at `0x0053e0d8`
proves. A 16-byte-aligned element pads the node instead, placing the value at `+0x10` in a
0x50-byte node, which `Rnd::MultiMesh` proves over its 0x40-byte transform. Read the node size and
the element size from the allocation rather than assuming either.

A container instantiation is library code, so it is expressed as the operator or the algorithm call
the original wrote, never as a reconstructed body.

## Access specifiers are inferred

Access control survives nowhere in a compiled image, so every specifier in this tree is an
inference from how the code reaches a member.

A data member of a class with behaviour is private by default. It becomes protected when the code of
a derived class touches it, and public when code outside the hierarchy does. Where an access appears
from an unrelated class, a friend declaration fits the image equally well as a promotion to public,
and the documentation states that ambiguity rather than presenting one reading as settled.

## Platform division

The original targets the PlayStation 2, and a class whose name begins `Ps` is the platform
implementation of the portable class above it. A portable class declares the interface and the
platform subclass supplies the hardware path.

Reconstruction covers the portable and the PlayStation 2 sides. Where a stub for another port makes
the division legible it is marked as a stub, and nothing invents a second platform's behaviour.
Nothing the PlayStation 2 SDK supplies is reconstructed.

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
