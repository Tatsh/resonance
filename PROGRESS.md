# Progress

Reconstruction status for FreQuency (PlayStation 2, `SCUS-97125`). Figures come from the command
below, which compares the address annotations in this tree against the function list of the
disassembler project. Update this file whenever a subsystem lands.

The function list is a fresh dump from the disassembler bridge rather than a stored file, because
identification advances continuously and a stale list understates the denominator.

```shell
curl -s 'http://127.0.0.1:8089/list_functions?limit=30000' > .wiswa-ci/freq/funcs.txt
uv run --project recon-tools python .wiswa-ci/freq/coverage_report.py .wiswa-ci/freq/funcs.txt freq-src
```

## Coverage

| Measure                      | Count  |
| ---------------------------- | ------ |
| Functions in the program     | 14,954 |
| Excluded by rule             | 3,914  |
| Reconstructable              | 11,040 |
| Accounted for in source      | 1,632  |
| Share of the reconstructable | 14.78% |
| Remaining, with a name       | 998    |
| Remaining, unidentified      | 8,410  |

The identified remainder rises as well as falls, because identifying a routine moves it out of the
unidentified column before any source accounts for it. A rise there is progress rather than
regression.

Exclusions are keyed on the name a function has. A routine therefore has to be identified before it
can be excluded, and the reconstructable figure falls as identification proceeds. That figure is
still overstated. A large part of the unidentified routines belongs to the platform SDK, the
compiler runtime, the template library, and the embedded interpreter. None of those is
reconstructed.

The identified but unwritten routines are the cheapest remaining work, because the analysis behind
each one is already done and only the source is missing. Two cautions apply to that column. A
routine's recorded title comes from an earlier pass and is sometimes wrong, and in one band
seventeen of twenty-one entries were titled for the wrong class entirely. And an entry may already
exist in the source under its real C++ name, because the column is built from unannotated addresses
rather than from absent code, which accounted for thirty-six of another band's fifty entries. Both
are fixed by confirming the owning class from the type-info accessor and by searching the tree
before writing.

A third caution outranks both. A title can identify a class that does not exist. Five renderer
class names invented by an earlier pass appear on game code, and no descriptor among the 574 in the
image bears any of them. Two bands were handed worklists grouped by those names, one of them 40
entries of 50. The harvest's accessor field records the same invented names, because it ran after
the pass that applied them. `.wiswa-ci/freq/make_band.py` now refuses a prefix that names no
descriptor, and rejecting the three prefixes that caused the damage is its regression check.

### Breakdown of exclusions

| Category                       | Count | Basis                                                            |
| ------------------------------ | ----- | ---------------------------------------------------------------- |
| Compiler-generated             | 820   | Type functions, their unfolded per-unit copies, static-init glue |
| Vendored upstream              | 508   | CPython 2.0, identified by diagnostic literal                    |
| Per-translation-unit duplicate | 2,320 | Bodies proven byte-identical to another routine of the image     |
| Template library               | 190   | Container instantiations                                         |
| Platform SDK                   | 33    | `sce` entry points and kernel syscalls                           |
| C++ runtime                    | 24    | Exception, cast, and unwinding support                           |
| C runtime                      | 19    | String and memory routines, and the floating-point library       |

## Verification

Every figure below is produced by a command rather than asserted. No target compiler is available.
Verification therefore stops at syntax and formatting.

| Check                                | Status  |
| ------------------------------------ | ------- |
| Headers compiling standalone         | 349/349 |
| Sources passing a syntax check       | 181/181 |
| Address annotations with no function | 0       |
| Lines over 100 characters            | 0       |
| `clang-format` differences           | 0       |

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

| Area                          | Notes                                                                                                                                                                                                          |
| ----------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Entry point                   | `main` and the loading screen                                                                                                                                                                                  |
| Asynchronous file layer       | Submission, the drive callback, the request and job records                                                                                                                                                    |
| Animation base                | `Rnd::Animatable` with all five nested filters                                                                                                                                                                 |
| Collision base                | `Rnd::Collideable` with its hit and sink types                                                                                                                                                                 |
| Message and packet family     | 73 concrete classes, 22 of them packets, over `Message`, `Packet`, `CmdMsg`, `MuseMsg` and seven routing intermediates. `ScriptMsg` alone is still partial                                                     |
| Material, PlayStation 2       | `Rnd::PsMat` in full, including the blend mode table                                                                                                                                                           |
| Streams                       | File, buffer, memory, and tool streams, plus the nine-class byte stream family over the input and output interfaces                                                                                            |
| Mesh, PlayStation 2           | `Sync` and all four draw paths, software and VU1                                                                                                                                                               |
| GIF packet buffer             | Reservation, tag closing, and the scratchpad double buffer                                                                                                                                                     |
| Scheduler command base        | `Sch::Command`, `Sch::TimedCommand`, and `Sch::Tick`. The command factory is dead in the shipped build                                                                                                         |
| Transform and animation base  | `Rnd::TransAnim` with its keyframe channels, over the transform and animatable bases                                                                                                                           |
| View, camera, and environment | `Rnd::Blur`, `Rnd::View` with its five class keys, and the camera and environment serialisation                                                                                                                |
| Mesh animation and instancing | `Rnd::MeshAnim` with its three keyframe channels, `Rnd::MultiMesh`, and `Rnd::PsMultiMesh`                                                                                                                     |
| Exception runtime             | Identified rather than reconstructed. The scheme is DWARF, and the unwinding driver, the frame-state builder, the handler-chain accessor, the terminate path, and the `dynamic_cast` entry point are all named |

### Partial

| Area                   | What remains                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| ---------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Texture, PlayStation 2 | Upload and bind bodies, pending the GS video memory manager                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| Graphics device        | Packet submission, pending the GS video memory manager                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| Art library            | `ABitmap` layout is recorded from the disassembler, not yet verified                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| Sound                  | `Synth` and `Ps2HardSynth` declared with the interface mapped slot by slot, and `midi_main` has its whole bank path: the load entry point, both transfer starters, both transfer classes, the claim table, the command dispatcher, the driver submit, the core and voice report, and the SPU2 bring-up. There is no voice table, because the module drives the hardware through libsdr. Thirteen interface slot titles are unrecoverable, the reverb configuration waits on the data-array query at `0x00509110`, and 27 routines remain |

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

A `CopyNN` suffix records the result of that test rather than a guess from the name. All 451 such
routines were re-verified against the image by `.wiswa-ci/freq/verify_copies.py`, which compares the
actual bytes against the unsuffixed sibling. 407 matched exactly. The 41 that did not are the
tagged `operator new` and `operator delete` of one message class, whose bodies differ only in which
per-unit copy of the identical tag literal `MSG` the unit referenced, and which are therefore
duplicates as well. The last three had no unsuffixed sibling because the numbering started at one,
and all three proved byte-identical to each other. Nothing in the set was a distinct routine, so the
whole set is excluded.

That result is not derivable from the fingerprint groups, because the fingerprint discards the
immediates. Two template instantiations differing only in an element size share a fingerprint and
are different functions. Byte comparison against the image is the only test that separates them.

The same test then applied to clusters whose every member was unidentified. 382 such clusters hold
2,268 addresses, and marking the surplus removed 1,843 of them from the denominator, with a further
142 removed where an unidentified body matched one already titled. Three safeguards make that
honest rather than convenient. The lowest address of each cluster is deliberately untouched, because
the one definition it stands for is still owed and still unidentified. An address the source already
annotates is never retitled and always wins as its cluster's representative, which caught one case
where the annotated routine would otherwise have been titled a copy of its own duplicate. And 25
clusters were held back because their bodies are too short for identity to prove duplication, the
shortest being two instructions, since two byte-identical trivial bodies are usually two distinct
overrides of one pure virtual.

The surplus members carry titles of the form `UnidentifiedBody<address>Copy<n>`. That form asserts
nothing about behaviour. It records which address the body repeats, and nothing more.

### Identification levers, including the exhausted ones

Three patterns in this image identify a routine from its own contents, with no inference. A class's
allocation operator is a short body that calls the tagged allocator with its own class name as a
string. A type_info accessor passes its class's length-prefixed mangled name to the descriptor
constructor. And slot zero of a class's vtable is that accessor, so an 8-byte entry pointing at a
known accessor locates the table and the walk to its zero terminator enumerates the class's
virtuals.

The first of those named 375 routines across 33 tags. The other two are now **exhausted**, and the
word is measured rather than assumed. Each pass was checked against routines whose answer was
already established before its result was believed, because a detector that reports nothing looks
exactly like a subsystem with nothing left to find. The accessor pass initially reported nothing
because its demangler read a two-component qualified name as a twenty-three-component one and
because it took the last name in the body rather than the one belonging to the final constructor
call; five of six controls pass after both were fixed, and the result is still nothing, so every
accessor in the image already carries its class's name. The vtable pass reports nothing for the
same kind of reason and the controls confirm the walk: both control tables parse correctly, their
terminators are found, and every slot of both is already titled.

So the routines still unidentified are none of those things. They are non-virtual members reached
by direct call, file-local helpers, free functions, and vendored library code, and each needs
reading rather than a pattern.

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

A trailing all-zero vtable entry is a terminator rather than a null slot. That settled two
secondary table counts which otherwise differed by one entry, and it decided whether the output
stream interface declares a virtual destructor. It does not.

Two byte-identical short bodies at different addresses are usually two distinct trivial overrides
of one pure virtual, rather than one routine emitted twice. Three such bodies of two instructions
each occur in the stream family. The contrasting case is one address appearing in three different
vtables, which is a genuine shared base body.

A reported success from the disassembler bridge is not evidence that a write persisted. Three
batches reported success and then read back reverted, roughly 60 of 68 renames surviving over twenty
minutes, while individual checks in between showed the names applied. Concurrent writers are the
likely cause. Every write is now re-read, and a batch is applied in a loop until the read-back
agrees.
