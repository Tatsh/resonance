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

| Measure                   | Count  |
| ------------------------- | ------ |
| Functions in the program  | 15,528 |
| Excluded by rule          | 6,278  |
| Reconstructable           | 9,250  |
| Declared or defined       | 3,112  |
| Share declared or defined | 33.64% |
| Defined, with a body      | 1,573  |
| Share implemented         | 17.01% |
| Remaining, with a name    | 1,682  |
| Remaining, unidentified   | 4,456  |

Two shares are recorded because they measure different things and the larger one was quoted alone
for most of this project's history. The audit counts an address as accounted once any file in the
tree annotates it, and a header declaration carries the same annotation a body does. So 1,539 of
the 3,112 are declared with their address, their signature, and their evidence recorded, and have no
implementation. 1,573 have a body.

Implementation is the figure the project's goal is stated against, so treat 17.01% as the answer to
"how much is reconstructed" and 33.64% as the answer to "how much is accounted for". A pass that
writes a header moves the larger share and not the smaller one, and a pass that writes bodies for an
already-declared class moves neither, because the addresses were annotated when the header landed.

Both figures come from `.wiswa-ci/freq/implemented_report.py`, which intersects the body markers
under `src` with the reconstructable set so that the two shares use one denominator. Do not measure
the implemented share by grepping for address literals. A `.cpp` mentions an address in ordinary
commentary as well as at a body marker, and that method returned 1,956 where the marker scanner
returns 1,605, of which 1,573 fall inside the reconstructable set and 32 belong to excluded
routines.

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
| Compiler-generated             | 823   | Type functions, their unfolded per-unit copies, static-init glue |
| Vendored upstream              | 1,712 | CPython 2.0, identified by diagnostic literal                    |
| Per-translation-unit duplicate | 2,312 | Bodies proven byte-identical to another routine of the image     |
| Template library               | 669   | Container instantiations                                         |
| Platform SDK                   | 338   | `sce` entry points and kernel syscalls                           |
| C++ runtime                    | 184   | Exception, cast, and unwinding support                           |
| C runtime                      | 220   | String and memory routines, and the floating-point library       |

## Verification

Every figure below is produced by a command rather than asserted. No target compiler is available.
Verification therefore stops at syntax and formatting.

| Check                                 | Status       |
| ------------------------------------- | ------------ |
| Headers compiling standalone          | 488/513      |
| Sources compiling                     | 335/341      |
| Address annotations with no function  | 0            |
| Lines over 100 characters             | 0            |
| `clang-format` differences            | 0            |
| `cspell`                              | 0 issues     |
| Declared virtuals resolving to a base | 0 mismatches |

Both shortfalls are the same gap and neither is a defect. 25 headers under `include/script` and 6
sources under `src/script` need the embedded interpreter's own `Python.h`, and this tree carries only
the interpreter's differences rather than its headers. The host's Python 3 headers would report
errors against Python 2 API use that describe nothing about the reconstruction. Every failure is
confined to that one subsystem, which was verified by listing the failures and finding none outside
it. Both checks are scripts rather than hand-written compiler lines,
`.wiswa-ci/freq/syntax_check.sh` for sources and `.wiswa-ci/freq/header_check.sh` for headers, and
both report their skip count so a partial run cannot read as a whole one.

Sources are measured by `.wiswa-ci/freq/syntax_check.sh`, which compiles each one with `-Wall
-Wextra` against ps2sdk and a small set of stand-ins for the Sony SDK headers ps2sdk lacks. Use that
script rather than a hand-written compiler line. Two bands reported a clean result from their own
line, and neither reproduced: one depended on stand-ins that existed only in its own scratchpad, and
the other was not passing the warning flags over reconstructed code at all.

The figures read the working tree rather than the commit history, so they lead it. A routine counts
as accounted the moment its annotation is written to disk, which is before the change is committed
and often before the agent that wrote it has reported. A commit landing several hundred lines can
therefore move the share by almost nothing, because the measurement had already seen the file.

The override check is `.wiswa-ci/freq/check_overrides.py` over `freq-src/include`. It reports a
declared virtual whose name matches a base name only in letter case, and one whose name matches
exactly while taking a different number of parameters. Neither shape produces a compiler
diagnostic: both declare a new virtual, the class silently stops overriding, and its table grows
past the entry count the image shows. Pass the directory rather than a file list, because with no
base header in scope nothing can fail to match and the pass means nothing.

Two changes to what the audit counts are recorded here rather than folded into the figure
silently. The function list grew by 393 routines, because a vtable slot points at its class's
virtual function whether or not the disassembler has claimed the bytes, and claiming those raises
the reconstructable total. And the annotation scanner now recognises the bare comment a file-local
routine is tied by, not only the Doxygen tag. The tag belongs on a public declaration, so a routine
with no header declaration cannot carry one, and 165 fully reconstructed routines were invisible to
the audit for that reason. The scanner requires the comment to sit on its own line with a
definition following it, which is what separates a routine marker from a comment merely mentioning
an address.

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

Networking beyond the packet records, and the game modes.

### Parked questions

Fifteen bands were paused mid-round, and these decisions were open at that point. Each is recorded
with what the band established, so none has to be re-derived.

`Renderer` has no placement evidence. Both levers that settled placement elsewhere came back nil
for the region 0x410000 to 0x43ffff: not one of the 30 anonymous-namespace markers falls inside it,
and every string recovered from the assert arguments there is the template library rather than a
game path. Sibling convention puts it in `app/` beside `RendererBase`, and that is a convention
argument rather than evidence, so no file was created.

Two addresses are the highest-yield pair in that same region, because each heads an unbroken run of
unidentified routines as well as blocking `Overlay`'s destructor: 0x0042aa18 heads a run of 14 and
0x0041ac18 heads a run of 12.

`Phrase` has no header anywhere in the tree, and `PhrasePacket` is otherwise fully recovered and
blocked only on that. `Phrase` has its own save, load, and print, so it belongs to whoever owns
gameplay rather than to the message band that needs it.

The class stored at +0x40 by `NotePitcher`, `SingleCatcher`, `MultiCatcher`, `Scratcher`, and
`Voxer` is unidentified, and so is the time-conversion object at its own +0x1c. Identifying both
makes bodies writable across all five classes at once. The entry points are 0x00127548,
0x00127628, and 0x00105de8.

`PowerupPlacer` and `JamPowerupPlacer` were declared by the message band because three of another
band's bodies were blocked on them, and neither is a message class. A sibling `GamePowerupPlacer`
sits beside them at 0x007e4ca0 with no header, and `GrooveWorld` is the likely owner.

`include/app/rendererbase.h` needs an owner. The additive change is well evidenced: the table at
0x007d2d20 runs eleven entries against `MsgSink`'s four, so slots 4 through 10 are `RendererBase`'s
own seven virtuals and the class is abstract. Slots 3, 7, and 8 all hold 0x005381a8, the pure
virtual stub. The placeholder spelling is preferred over verbs taken from `MetRenderer`, because
naming an interface from one subclass is the same error as reading a signature off a call site.

Four worklist lines are titled for the wrong class. 0x00372c10, 0x00373808, 0x00374208, and
0x00374b58 are titled for `MetRemixDelScreen` but sit inside `MetSaveRemix`'s address region, and
`MetRemixDelScreen`'s own code runs from 0x00339000 to 0x00344000.

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

A second pass of the same kind was over-applied and is also corrected. Titling a routine as a
class's allocation operator required only that it call the tagged allocator with a class tag, and
that is not sufficient: a clone allocates through that allocator and a destructor releases through
it, so both reference the tag without being the operator. Of 287 routines titled that way, 138 are
the genuine eight-instruction forwarder, 95 occupy a vtable slot that names their real owner and
were retitled from it, and 54 had no evidence behind any title and were returned to placeholders.

One further correction goes beyond the duplicate rule. Where a trivial destructor is implicitly
declared, the source owes no definition at all rather than one, because the compiler generates it.
The eighty-five identical copies of one such destructor in the message subsystem therefore owe
nothing, and forty-seven of them are separate classes' destructors that coincide byte for byte
through storing the shared base pointer.

One rule behind that marking was wrong and is corrected. The pass accepted a cluster as one routine
emitted many times whenever the body was long enough for coincidence to look implausible. Length
does not decide it. A trivial destructor stores its class's vptr and nothing else, so every class
sharing one base compiles a byte-identical destructor, and 47 identical bodies can be 47 distinct
methods. Occupying a vtable slot is the test that separates the two cases, and 147 addresses the
pass had marked hold one. All 147 are restored to placeholder names and are back in the
denominator. The figure published before that correction was overstated by those 147.

Identifying library and vendored code is what moved the exclusions from 3,801 to 4,691 in one
round. The platform SDK entry grew eightfold once Sony's MPEG, DMA, configuration, disc, and DECI2
libraries were recognised from their own diagnostics, and the C runtime entry grew once a
third-party arbitrary-precision package, two formatting engines, and the signal set were. Every one
of those is code the reconstruction does not owe, and leaving it unidentified overstated the work
remaining rather than the work done.

One residual risk in those exclusions is measured rather than left open. A platform-SDK title
hides a routine twice over, out of the reconstructable total and out of the list still needing
identification, so a wrong one is never asked about again. Two have been found and corrected:
seven file routines carried a graphics-library title and one cache-maintenance routine carried an
input-pad title, both inferred from a neighbour's prefix rather than from a body.

An audit bounds what is left. 337 routines carry such a title, 146 of them in the placeholder form
that records a proven library with an unrecovered role. Of those, 109 reference a hardware register
or call another routine of the same library in their own body, and **37 rest on their band's cluster
argument alone**. That is the exposure, and it is small enough to state exactly.

The exclusion rules are keyed on prefixes a band applies deliberately: `Stl` and `std_` for the
template library, `Cxx` and the iostream names for the C++ runtime, `Lib` with a following c, k, or
m for the vendored C library, kernel glue, and floating point, `sce` for the platform SDK, `Py` and
`Python__` for the interpreter, and `Gzip` and `Netflow` for two further vendored packages. The
stdio set is spelled out name by name rather than given a prefix, because a prefix there would
catch a project's own routines. A test covers both directions of that, and it caught one rule that
was too loose before it shipped.

A fifth lever exists, and it works where the other four do not. The vendored interpreter publishes
a type object per type whose slots address that type's static functions in a fixed order, and a
method table per module and per type pairing each method's text with its function. Both sit in the
data section, and the upstream tree supplies the same two orders by name, so the join recovers exact
upstream names with no literal and no call-graph inference. It recovered 578 across the image.

The names are the smaller half of that result. 75 of those addresses had no function defined at
all, because an over-long body swallows several upstream routines at once and every one of them is
invisible to a count until something points at its entry. That is also why the function total rises
here rather than staying fixed.

Two limits are recorded with the tool. A type whose name reads through a size expression defeats
the upstream side of the match. And the embedded interpreter is not exactly the release the tree
records: its dictionary type fills a rich-comparison slot the release leaves empty, and leaves empty
two the release fills, so the build has rich comparison for dictionaries and no cycle collector.

Three of the bulk passes had a defect repaired in the generator rather than in the output, which is
the difference between fixing a fault and cleaning up after it. The allocation-operator pass now
requires the body to be a short forwarder before it will title anything, because referencing a
class tag is not the same as being that class's operator; on a re-run that rejected 93 of 94
candidates, every one of which would have been a wrong title. The duplicate pass now refuses to
mark any address that occupies a vtable slot, which is the guard whose absence produced 147
markings that had to be undone. Both changes cost one condition each and remove a whole class of
error rather than an instance of it.

### Identification levers, including the exhausted ones

Three patterns in this image identify a routine from its own contents, with no inference. A class's
allocation operator is a short body that calls the tagged allocator with its own class name as a
string. A type_info accessor passes its class's length-prefixed mangled name to the descriptor
constructor. And slot zero of a class's vtable is that accessor, so an 8-byte entry pointing at a
known accessor locates the table and the walk to its zero terminator enumerates the class's
virtuals.

The allocation-tag pass named 375 routines across 33 tags. The accessor pass is exhausted: every
type_info accessor in the image already carries its class's name. It first reported nothing through
two defects of its own, a demangler reading a two-component qualified name as a
twenty-three-component one and a detector taking the last name in the body rather than the one
belonging to the final constructor call, and five of six controls pass after both were fixed.

The vtable pass was recorded here as exhausted too, and that was wrong. Its walk computed each
table's base one word below the true address, so it located tables and then misread every entry.
An earlier control validated the entry layout using a separately computed table address, which
meant the control never exercised the faulty arithmetic. With it corrected the same pass attributes
1,069 routines to their owning class and 1,068 read back. A slot title records the owner and the
slot index and nothing else, so ownership is measured and the verb stays open.

Two lessons sit behind that. A pass reporting nothing looks exactly like a subsystem with nothing
left to find, so a negative result needs a control before it is believed. And a control has to
exercise the part that can be wrong, not merely the part that is easy to check.

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

### Compiler-generated code is never written

A virtual function table pointer, a virtual base pointer, a type function, the per-unit
static-initialisation glue, and an implicit copy or assignment body are all emitted by the compiler
rather than written by a programmer. None of them appears in this tree as a declaration or a body. A
layout comment records where the compiler placed a pointer, and nothing declares one.

The toolchain emits an inline function into every translation unit that needs it, and the linker
folds none of the copies. One class has 45 identical copies of its type function. An address count
is therefore not a function count, and a routine whose body repeats one already reconstructed adds
no source.

### Access specifiers are inferred

Access control survives nowhere in a compiled image, so every specifier in this tree is an inference
from how the code reaches a member.

A data member of a class with behaviour is private by default. It becomes protected when the code of
a derived class touches it, and public when code outside the hierarchy does. Where an access appears
from an unrelated class, a friend declaration fits the image equally well as a promotion to public,
and the documentation records that ambiguity rather than presenting one reading as settled.

### STL container layout

The template library is the SGI implementation that g++ 2.9x shipped, and its layouts differ from a
modern one. A `std::list` is one four-byte pointer addressing a single self-linked dummy node.

The payload offset inside that node depends on the alignment of the element. A four-byte element
places the value at `+0x08` in a 16-byte node, which the `Rnd::Object` constructor at `0x0053e0d8`
proves. A 16-byte-aligned element pads the node instead, placing the value at `+0x10` in a 0x50-byte
node, which `Rnd::MultiMesh` proves over its 0x40-byte transform. Read the node size and the element
size from the allocation rather than assuming either.

A container instantiation is library code, so it is expressed as the operator or the algorithm call
the original wrote, never as a reconstructed body.

### Platform division

The original targets the PlayStation 2, and a class whose name begins `Ps` is the platform
implementation of the portable class above it. A portable class declares the interface and the
platform subclass supplies the hardware path.

Reconstruction covers the portable and the PlayStation 2 sides. Where a stub for another port makes
the division legible it is marked as a stub, and nothing invents a second platform's behaviour.
Nothing the PlayStation 2 SDK supplies is reconstructed.

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

A header needs only a forward declaration wherever it uses a pointer or a reference, and that
declaration belongs inside the class's own namespace. A global `class Mat;` for `Rnd::Mat` declares
a second unrelated type. The header still compiles, because a pointer to an incomplete type is
valid, and the failure appears in a distant implementation file as a conversion error between the
phantom type and the real one. Two further effects followed the pass that applied this across the
tree. An implementation file that had been receiving a definition transitively has to include it
itself, and an include is not replaceable when the including header uses a constant, an enum, or a
typedef from it rather than only the class.

A pass that rewrites headers has to exclude the directories other bands are writing, and a commit
from it has to exclude any file whose diff is not purely the rewrite.
`.wiswa-ci/freq/stage_include_only.py` builds that list by rejecting a file with a changed line
that is not an include, a forward declaration, a namespace brace, or blank. One met screen showed
228 added lines with the include additions mixed in, which would have committed unfinished work
under the pass's message.

A detector that pattern-matches source has to blank the comments first. The override checker
matched `class concrete:` inside a doc comment describing what "makes the class concrete", which
registered a phantom class and, because its base-list pattern ran to the next brace, consumed the
real declaration that followed. The verdict stayed correct by accident, since the phantom inherited
the right base from the declaration it had swallowed. The only symptom was a class count that
failed to increment after a file was added, which is why an unchanged count after adding an input
is a failure report rather than a stable baseline.

An address materialised by a `lui` and `addiu` pair has to be evaluated rather than read off. The
low half is signed, so `addiu v0,v0,0x88c0` subtracts 0x7740. Two separate readings in this project
have been wrong for that reason, one of them producing a buffer address a digit short, `0x008f9f0`
for `0x008f09f0`, which was then duplicated across two translation units and two header comments
and pointed at unrelated memory throughout.

Two destination meanings in one routine mean the reading is incomplete, not that the routine is odd.
`ACanvasLin4`'s row writer packs two source bytes into one destination byte in its bulk loop and
stores a literal 0 or 1 as a whole byte in its per-pixel paths, which was confirmed by decoding the
raw instruction words rather than by trusting a listing. The body is recorded as unresolved and its
three callers are written, because their signatures are settled independently of it.

A routine can be orphaned rather than merely unreferenced by a table. Eight members of the canvas
remap and blend families fill no vtable slot and the program lists no caller and no data reference
for any of them, so neither a slot nor a call site can attribute them. Their receiver argument and
the slots they dispatch to are the whole evidence.
