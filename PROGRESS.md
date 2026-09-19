# Progress

Reconstruction status for FreQuency (PlayStation 2, `SCUS-97125`). Figures come from
`rctool audit coverage`. That command compares the address annotations in this tree against the
function list of the disassembler project. Update this file whenever a subsystem lands.

## Coverage

| Measure                     | Count  |
| --------------------------- | ------ |
| Functions in the program    | 14,666 |
| Excluded by rule            | 1,449  |
| Reconstructable             | 13,217 |
| Accounted for in source     | 725    |
| Share of the reconstructable| 5.49%  |
| Remaining, with a name      | 921    |
| Remaining, unidentified     | 11,574 |

Exclusions are keyed on the name a function has. A routine therefore has to be identified before it
can be excluded, and the reconstructable figure falls as identification proceeds. That figure is
still overstated. A large part of the 11,580 unidentified routines belongs to the platform SDK, the
compiler runtime, the template library, and the embedded interpreter. None of those is
reconstructed.

### Breakdown of exclusions

| Category           | Count | Basis                                                         |
| ------------------ | ----- | ------------------------------------------------------------- |
| Compiler-generated | 823   | Type functions, their unfolded per-unit copies, static-init glue |
| Vendored upstream  | 414   | CPython 2.0, identified by diagnostic literal                 |
| Platform SDK       | 15    | `sce` entry points and kernel syscalls                        |
| C++ runtime        | 16    | Exception and cast support                                    |
| C runtime          | 15    | String and memory routines                                    |
| Template library   | 170   | Container instantiations                                      |

## Verification

Every figure below is produced by a command rather than asserted. No target compiler is available.
Verification therefore stops at syntax and formatting.

| Check                               | Status  |
| ----------------------------------- | ------- |
| Headers compiling standalone        | 164/164 |
| Sources passing a syntax check      | 110/110 |
| Address annotations with no function | 0       |
| Lines over 100 characters           | 0       |
| `clang-format` differences          | 0       |

A header is checked on its own, through a translation unit that includes nothing else. That catches
a break in a header which no implementation file happens to include.

```shell
g++ -fsyntax-only -D_EE -DHAVE_LIMITS_H -DSIZEOF_LONG=8 -I include -I src/python/PC -I ../.wiswa-ci/freq/compat -I ../.wiswa-ci/freq/Python-2.0/Include -I ../ps2sdk/common/include -I ../ps2sdk/ee/kernel/include -I ../ps2sdk/ee/rpc/cdvd/include -I ../ps2sdk/ee/rpc/sdr/include -I ../ps2sdk/ee/rpc/memorycard/include <file>
```

One flag on that line is a host accommodation rather than a fact about the target, and it is
recorded here rather than hidden. The interpreter's portability header rejects a 32-bit target
configuration on a 64-bit host, refusing to agree that a long is four bytes wide while the host
says eight, so the host width is asserted for the syntax check alone. That affects nothing the
check is for, since the binding headers are checked for C++ validity rather than for the
interpreter's own arithmetic, and the alternative is four headers checked by nothing at all.

The PlayStation 2 SDK is available and every SDK call is checked against its real declaration. The
original was built against Sony's official SDK, whose header names differ from the open-source
ps2sdk in places, and a forwarding shim resolves the difference rather than a redeclaration. Nothing
from the SDK is reconstructed.

## Subsystems

### Complete

| Area                     | Notes                                                              |
| ------------------------ | ------------------------------------------------------------------ |
| Entry point              | `main` and the loading screen                                      |
| Asynchronous file layer  | Submission, the drive callback, the request and job records        |
| Animation base           | `Rnd::Animatable` with all five nested filters                     |
| Collision base           | `Rnd::Collideable` with its hit and sink types                     |
| Message and packet family| 73 concrete classes, 22 of them packets, over `Message`, `Packet`, `CmdMsg`, `MuseMsg` and seven routing intermediates. `ScriptMsg` alone is still partial |
| Material, PlayStation 2  | `Rnd::PsMat` in full, including the blend mode table                |
| Streams                  | File, buffer, memory, and tool streams                              |
| Mesh, PlayStation 2      | `Sync` and all four draw paths, software and VU1                    |
| GIF packet buffer        | Reservation, tag closing, and the scratchpad double buffer          |

### Partial

| Area                      | What remains                                                        |
| ------------------------- | ------------------------------------------------------------------- |
| Texture, PlayStation 2    | Upload and bind bodies, pending the GS video memory manager          |
| Graphics device           | Packet submission, pending the GS video memory manager               |
| Art library              | `ABitmap` layout is recorded from the disassembler, not yet verified |
| Sound                     | `Synth` and `Ps2HardSynth` declared with the interface mapped slot by slot, and `midi_main` has its whole bank path: the load entry point, both transfer starters, both transfer classes, the claim table, the command dispatcher, the driver submit, the core and voice report, and the SPU2 bring-up. There is no voice table, because the module drives the hardware through libsdr. Thirteen interface slot titles are unrecoverable, the reverb configuration waits on the data-array query at `0x00509110`, and 27 routines remain |

### Not started

The front end screens, the metagame, networking beyond the packet records, and the game modes.

## Duplicated routines

An address count is not a function count in this image. Fingerprinting every routine by its opcode
and register sequence, with the immediates discarded, puts 7,814 of them into groups that share a
shape. Those groups divide into two kinds, and the division matters.

A group whose members are **byte-identical** is one routine emitted many times. 164 such groups
cover 1,557 addresses. Those need at most 164 source definitions between them, and none at all
where the routine belongs to the template library. The largest is an allocator inline with 168
copies.

A group whose members share a shape but **differ in their immediates** is many distinct routines.
724 such groups cover 6,257 addresses. A class destructor and another class's destructor look alike
while referencing a different vtable and a different tag string, and each one is its own source.

Byte equality over the shared body length is the whole test. Applying it propagated 311 names from
a representative to its copies. That is sound precisely because the copies are the same routine.

Four clusters of 148 copies each remain unidentified, at `0x00107d28`, `0x00107eb8`, `0x001080a8`,
and `0x00108738`. They are members of one map keyed by object name: the comparator is
`HxStr::SortsBefore`, and `Rnd::Object::SetName()` and its destructor are the callers. The specific
algorithm of each is not pinned, and they are left unidentified rather than given a name that
claims more than has been established. One caution for a later reader: the node colour of this
container is an `int` rather than a byte, and the absence of byte operations is therefore not
evidence against a tree.

## Conventions

Two conventions matter to a reader.

An `@ghidraAddress 0x...` tag on a declaration ties it to the routine it was reconstructed from. The
address is relative to the image base.

A gap is marked rather than filled. A member whose purpose is undetermined has a placeholder name
and an offset comment, an inferred identifier is stated to be inferred, and a reserved run records a
span of a structure that has not been recovered. None of those is a settled field.

## Methodology notes

Findings that repeatedly decide questions. Recording them here avoids rediscovering each one.

A routine that prints its own name in a diagnostic is authoritative about what it is called. That
recovered `RestoreSurfaces` after this tree had titled it `OnAllMipsLoaded`.

A value in the return register is a return only when every exit agrees on it and the value is not an
address. Applying that test corrected two reported signatures. One of them returned a pointer on
its early exit.

A parameter's existence and its use are separate questions. A call site materialising an argument
proves the prototype declares it. Only the callee proves whether the body reads it.

This compiler emits an inline function into every translation unit that needs it, and the linker
folds none of the copies. One class has 45 identical copies of its type function. An assumption of
one address per function is therefore false here.

A type function builds its bases before itself. Reading the first constructor call inside one
therefore identifies a base rather than the owner. The owner is the class whose descriptor the
routine guards on at entry. That correction re-attributed 75 accessors.

A routine testing its second argument against `0xffff` and branching on its first is the
per-translation-unit static-initialisation glue of this compiler. Ninety-eight exist. None is
source. Reconstructing one produces a type the program does not have.
