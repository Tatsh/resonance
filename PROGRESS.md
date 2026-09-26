# Progress

Reconstruction status for FreQuency (PlayStation 2, `SCUS-97125`). The figures compare the address
annotations in this tree against the function list of the disassembler project. Update this file
whenever a subsystem lands.

The function list is a fresh dump from the disassembler bridge. A stored list understates the
denominator as identification advances.

```shell
curl -s 'http://127.0.0.1:8089/list_functions?limit=30000' > .wiswa-ci/freq/funcs.txt
uv run --project recon-tools python .wiswa-ci/freq/coverage_report.py .wiswa-ci/freq/funcs.txt freq-src
```

## Coverage

| Measure                   | Count  |
| ------------------------- | ------ |
| Functions in the program  | 15,647 |
| Excluded by rule          | 8,725  |
| Reconstructable           | 6,922  |
| Declared or defined       | 6,459  |
| Share declared or defined | 93.30% |
| Defined, with a body      | 6,093  |
| Share implemented         | 88.01% |
| Remaining, with a name    | 464    |
| Remaining, unidentified   | 0      |

The table measures a clean export of the commit `ff765e8`.

The audit counts an address as accounted once any file in the tree annotates it, and a header
declaration takes the same annotation a body does. Treat 88.01% as the answer to "how much is
reconstructed" and 93.30% as the answer to "how much is accounted for".

Of the 6,459 accounted routines, 6,093 have a body the scanner counts. Of the other 366, the
great majority are reconstructable routines declared with their address, signature, and evidence
(inline in headers, template instances, split signatures, and defaulted or vendored glue), and the
rest are annotated library and vendored routines whose titles fall outside the body count.

Of the 464 routines with no annotation, 249 are implicit special members, 91 are static
initialiser and exit stubs, 57 are ezmpeg sample routines, 21 are interpreter bindings and their
wrappers, 20 are library routines labelled with their upstream names, 11 are script workers (ten
unreferenced duplicates of live entry points, two building the play-map test probe, and one
scheduler kill with no caller), six are SDK routines the open-source SDK provides or links (three
interrupt-context entries, two interrupt toggles, and one stream-input helper), eight are recorded
exceptions, and one is a measurement gap. The canvas factory
`ACanvas::CreateForSubBitmap()` at `0x005e8e38` and the `ABitmap` sub-rectangle constructor at
`0x00558dd8` it calls, found in code the disassembler had not defined, both have bodies in the
tree since the script-layer push.

### Measurement history

| Commit    | Share implemented | Main change                                                           |
| --------- | ----------------- | --------------------------------------------------------------------- |
| `ff765e8` | 88.01%            | Script command layer plus test-map builders, spew, test, and clock    |
| `bb0b785` | 87.91%            | Script command layer plus spew and test commands                      |
| `451cec1` | 87.91%            | Script command layer: scene, cheat, toggle, tunnel, HUD, and hx units |
| `064615f` | 87.50%            | Faithfulness review against the disassembly, four functions found     |
| `5e32890` | 87.52%            | Every declared routine gained a body or a classification              |
| `a40a970` | 85.21%            | Markers added to bodies after a disassembly comparison                |
| `d7e688b` | 73.39%            | libvu0 and ezmpeg identified as SDK code                              |
| `c35fd29` | 64.19%            | Front end, tunnel, and message container instantiations identified    |
| `7b43c85` | 48.36%            | Front end screens, player, play map, and renderer bodies              |
| `c715051` | 44.79%            | Front end container instantiations identified                         |
| `b7bc378` | 37.58%            | Template library, interpreter, and runtime identification             |
| `72f44cb` | 31.68%            | Template library identification                                       |
| `27cc069` | 27.78%            | Library routines identified by normalised body matches                |

Since `5e32890`, cross-reviews that traced every argument to its producer and placed every
destructor corrected bodies in every reviewed subsystem (a remix deleted by the wrong key, a
feedback sprite drawn sixteen times too deep, a heap split that did not link its free node, signed
and unsigned comparisons, message lifetimes, and loops that reload a vector's end). Three bodies
moved into headers as inline definitions where another unit expands them. That lowered the counted
bodies by three.

### Known gaps in the measurement

These routines count as remaining although the tree handles them by rule:

- An implicit destructor, copy constructor, or assignment of a project class, labelled
  `<Class>__Destruct`, `__ConstructCopy`, or `__AssignImplicit`. The compiler generates it.
- The ezmpeg sample units (`disp.c`, `vobuf.c`, `readbuf.c`, `strfile.c`, `audiodec.c`,
  `videodec.c`, and `vibuf.c`), linked as shipped.
- The interrupt-context SDK entry points labelled `isce…`. The `sce` pattern does not match them.
- An inline member defined in a header with its `// 0x...` marker (for example
  `Cam::ProjectToUnit`), and a function template instance whose body is the template in a header.
  The body count reads `src` only.
- A definition whose return type clang-format places on a separate line (`CheckPalEqual`,
  `MetJukeboxEditPlaylistScreenLowerLeft::New`). A marker counts only when the next line with code
  identifies the function.
- Static initialiser stubs (`__StaticCtor`, `__StaticDtor`, `__GlobalCtors`) and the exit handlers
  of function-local statics (`AtExitDestroy…`, `__StaticDestroy`).
- The `hx.*` script bindings (the `HxScript__X` bodies and their `HxScript__XEntry` wrappers) and the
  PyCXX wrappers. The interpreter and its C++ binding are not yet in the tree.
- Library routines labelled with their upstream names rather than a family prefix (`getenv`,
  `_findenv_r`, `toupper`, the SIO printf engine, `_sceVu0ecossin`, the `libio` stream slots, and
  the `type_info` and `exception` members).
- Recorded exceptions without a separate body: the `TunnelEvent::DrawFiltered` copy, three
  unreferenced return-zero stubs, the unreferenced send copies in the `Delayer`,
  `MidiDisabler`, and `Renderer` units, and the `Print()` at `0x003d67f0` of the unused class the
  `Q23Mid3MBT` descriptor belongs to.

The body count, `.wiswa-ci/freq/body_share.py`, intersects the body markers under `src` with the
function list after the default exclusions. Its set is 141 routines smaller than the audit's
reconstructable figure (6,781 against 6,922). The implemented share divides by the audit's figure
and therefore understates by at most 141 routines.

Do not measure the implemented share by grepping for address literals. An implementation file
mentions addresses in commentary as well as at body markers.

### Breakdown of exclusions

| Category                       | Count | Basis                                                            |
| ------------------------------ | ----- | ---------------------------------------------------------------- |
| Compiler-generated             | 899   | Type functions, their unfolded per-unit copies, static-init glue |
| Vendored upstream              | 1,978 | CPython 2.0, identified by diagnostic literal                    |
| Per-translation-unit duplicate | 1,950 | Bodies proven byte-identical to another routine of the image     |
| Template library               | 2,877 | Container instantiations                                         |
| Platform SDK                   | 484   | `sce` entry points and kernel syscalls                           |
| C++ runtime                    | 214   | Exception, cast, and unwinding support                           |
| C runtime                      | 323   | String and memory routines, and the floating-point library       |

Exclusions are keyed on the title a function has. A routine is identified before it is excluded,
and the reconstructable figure falls as identification proceeds. The rules match prefixes applied
deliberately: `Stl` and `std_` for the template library, `Cxx` and the iostream names for the C++
runtime, `Lib` with a following c, k, or m for the vendored C library, kernel glue, and floating
point, `sce` for the platform SDK, `Py` and `Python__` for the interpreter, and `Gzip` and `Netflow`
for two further vendored packages. The stdio routines are listed individually. A prefix there would
match project routines.

The platform SDK row still counts the Sony SDK routines that ps2sdk does not provide. Those are owed
code (see [Platform division](#platform-division)) and move out of the exclusion as they are
separated.

## Verification

Every figure below comes from a command. CI compiles every buildable source with the Emotion Engine
cross compiler in the ps2dev container and archives one static library per subsystem. The libraries
are not linked yet. CI reports two recorded warnings. `remixindex.h` reports five fields that
`RemixIndex::ReadFromStream()` copies before they are written, as the binary does. `mem.cpp` lacks
the sized `operator delete` forms, and the original compiler predates them.

| Check                                 | Status       |
| ------------------------------------- | ------------ |
| Headers compiling standalone          | 706/706      |
| Sources compiling                     | 627/627      |
| Address annotations with no function  | 0            |
| Lines over 100 characters             | 0            |
| `clang-format` differences            | 0            |
| `cspell`                              | 0 issues     |
| Declared virtuals resolving to a base | 0 mismatches |

Sources are measured by `.wiswa-ci/freq/syntax_check.sh` and headers by
`.wiswa-ci/freq/header_check.sh`. Each header compiles in a translation unit that includes only that
header. Both scripts compile with `-Wall -Wextra` against ps2sdk and the stand-ins for Sony SDK
headers, and both report their skip count. The 26 headers and 17 sources that include the
interpreter's `Python.h` are skipped by the scripts and checked with the line below, run from
`freq-src`. The first two rows count them.

```shell
g++ -fsyntax-only -std=c++17 -Wall -Wextra -D_EE -DHAVE_LIMITS_H -DSIZEOF_LONG=8 -I include -I compat -isystem src/python/PC -isystem ../.wiswa-ci/freq/Python-2.0/Include -isystem ../ps2sdk/common/include -isystem ../ps2sdk/ee/kernel/include -isystem ../ps2sdk/ee/rpc/cdvd/include -isystem ../ps2sdk/ee/rpc/sdr/include -isystem ../ps2sdk/ee/rpc/memorycard/include -isystem ../ps2sdk/ee/rpc/multitap/include <file>
```

`-DSIZEOF_LONG=8` is the original target's width. The interpreter's `PyInt_AsLong()` at
`0x00581c00` reads an integer object's `long` with an eight-byte `ld`. A current PlayStation 2
toolchain has a four-byte `long`.

The override check is `.wiswa-ci/freq/check_overrides.py` over `freq-src/include`. It reports a
declared virtual whose title matches a base virtual only in letter case, and one that matches exactly
with a different parameter count. Neither produces a compiler diagnostic. The class stops overriding
and its table grows past the entry count the image shows. Pass the directory rather than a file
list. Without the base headers in scope every virtual appears new.

The annotation scanner recognises the bare `// 0x...` comment above a file-local routine as well as
the Doxygen tag. The comment has to sit on a separate line with a definition after it.

## Subsystems

| Area                          | Notes                                                                                                                                                                                                               |
| ----------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Entry point                   | `main` and the loading screen                                                                                                                                                                                       |
| Asynchronous file layer       | Submission, the drive callback, the request and job records                                                                                                                                                         |
| Animation base                | `Rnd::Animatable` with all five nested filters                                                                                                                                                                      |
| Collision base                | `Rnd::Collideable` with its hit and sink types                                                                                                                                                                      |
| Message and packet family     | 73 concrete classes, 22 of them packets, over `Message`, `Packet`, `CmdMsg`, `MuseMsg`, and seven routing intermediates                                                                                             |
| Material, PlayStation 2       | `Rnd::PsMat`, including the blend mode table                                                                                                                                                                        |
| Streams                       | File, buffer, memory, and tool streams, and the nine-class byte stream family over the input and output interfaces                                                                                                  |
| Mesh, PlayStation 2           | `Sync` and all four draw paths, software and VU1                                                                                                                                                                    |
| GIF packet buffer             | Reservation, tag closing, and the scratchpad double buffer                                                                                                                                                          |
| Scheduler command base        | `Sch::Command`, `Sch::TimedCommand`, and `Sch::Tick`                                                                                                                                                                |
| Transform and animation base  | `Rnd::TransAnim` with its keyframe channels                                                                                                                                                                         |
| View, camera, and environment | `Rnd::Blur`, `Rnd::View`, `PsCam`, and `PsEnviron`, with camera and environment serialisation                                                                                                                       |
| Mesh animation and instancing | `Rnd::MeshAnim` with its three keyframe channels, `Rnd::MultiMesh`, and `Rnd::PsMultiMesh`                                                                                                                          |
| Art library                   | Every canvas class, the polygon fills, the stretch and clip routines, `APalette`, `ARleReader`, and the BMP, TGA, and GIF readers, apart from the two owed routines above                                           |
| Texture, PlayStation 2        | `Rnd::PsTex`, including surface restore, the upload and bind path, and render-target binding                                                                                                                        |
| Particles                     | `Rnd::ParticleSys`, including the simulation, the text dump, and revisions 0 to 6 of its file format                                                                                                                |
| Cutscene player               | The game's C unit around Sony's MPEG sample                                                                                                                                                                         |
| Debug console                 | The development console bring-up                                                                                                                                                                                    |
| Exception runtime             | Identified rather than reconstructed. The scheme is DWARF                                                                                                                                                           |
| Graphics device               | Every `GfxDevice` routine, with the VU1 setup, lighting, and clipping routines of the draw path. The vertical-blank handler's body is MIPS assembly                                                                 |
| Tunnel                        | `Rnd::Tunnel`, `Rnd::Generator`, the duration-gem trails, `AppTunnel`, and the tunnel effect classes                                                                                                                |
| Gameplay display              | `Renderer`, `Overlay`, the HUD panel and per-track display, `TnlArena`, and the screen animations                                                                                                                   |
| Gameplay world                | `GrooveWorld`, `Phrase`, `PhraseDatabase`, `PhraseMgr`, `TrackData`, `Catcher`, `AutoRiffer`, `NotePlayer`, the power-bar managers, the R250 generator, and the pitcher classes                                     |
| Messages and packets          | Every factory, printer, serializer, and stack constructor of the packets and messages, with the Standard MIDI File reader                                                                                           |
| Sound                         | `Synth` and `Ps2HardSynth` with the interface mapped slot by slot, and the bank path of `midi_main`. The module drives the hardware through libsdr. Thirteen interface slot titles are unrecoverable                |
| Script commands               | The scene, cheat, toggle, tunnel, HUD, record, test-map, and `hx` command implementations with their interpreter entry points, over the `Py::` binding. The input, powerup, ghost, and loop commands are still open |

### Parked questions

Two types compete for the class `Mid::MBT`. The image's `Q23Mid3MBT` descriptor belongs to an unused
0x10-byte polymorphic class (measure, beat, tick, and a vptr at +0xc, with its vtable at
0x008110e8, constructor at 0x003d6798, and `Print()` at 0x003d67f0). The four-byte position word
the messages use emits no RTTI, and its true identifier is not in the image. The tree retains
`Mid::MBT` for the four-byte word by convention until evidence of its real identifier appears.

## Duplicated routines

An address count is not a function count in this image. The toolchain emits an inline function into
every translation unit that needs it, and the linker does not fold the copies. One class has 45
identical copies of its type function.

A group of byte-identical routines is one routine emitted many times and needs at most one source
definition. A group whose members share an opcode shape but differ in their immediates is many
distinct routines (two classes' destructors referencing different vtables, for example). Byte
equality against the image is the only test that separates the two kinds.

A `CopyN` suffix records that byte test. A group's original, the copy callers or the unit's unwind
record identify, takes the plain title. A trivial destructor stores its class's vptr and does
nothing else, and every class sharing one base compiles an identical one. Occupying a vtable slot
therefore marks a distinct method, and the duplicate pass refuses any address in a vtable slot.

The vendored interpreter publishes a type object per type, whose slots address that type's static
functions in a fixed order, and a method table per module and per type. The upstream tree supplies
the same orders by name, and joining the two recovers exact upstream names. The embedded interpreter
differs from the 2.0 release. Its dictionary type fills a rich-comparison slot the release does not
fill and does not fill two the release fills.

### Identification levers

Three patterns identify a routine from its contents. A class's allocation operator is a short body
that calls the tagged allocator with its class name as a string. A type_info accessor passes its
class's length-prefixed mangled name to the descriptor constructor. Slot zero of a class's vtable
is that accessor, and walking the table to its zero terminator enumerates the class's virtuals.
Every type_info accessor in the image includes its class's name.

A pass that reports no findings looks exactly like a subsystem with no remaining work. A negative
result needs a control, and the control has to exercise the part that can be wrong.

The node colour of the template library's red-black tree is an `int` rather than a byte. The absence
of byte operations is not evidence against a tree.

## Conventions

An `@ghidraAddress 0x...` tag on a declaration ties it to the routine it was reconstructed from. The
address is relative to the image base.

A gap is marked rather than filled. A member whose purpose is undetermined has a placeholder
identifier and an offset comment, an inferred identifier is documented as inferred, and a reserved
run records an unrecovered span of a structure.

### Compiler-generated code is not written

A virtual function table pointer, a virtual base pointer, a type function, the per-unit
static-initialisation glue, and an implicit copy or assignment body are emitted by the compiler. The
tree does not declare or define any of them. A layout comment records where the compiler placed a
pointer.

### Access specifiers are inferred

A compiled image does not record access control, and every specifier in this tree is an inference
from how the code uses a member. A data member of a class with behaviour is private by default. It
becomes protected when a derived class uses it, and public when code outside the hierarchy does.

### STL container layout

The template library is the SGI implementation that g++ 2.9x shipped. A `std::list` is one
four-byte pointer to a single self-linked dummy node. A four-byte element places the value at
`+0x08` in a 16-byte node, and a 16-byte-aligned element places it at `+0x10` in a 0x50-byte node.
Read the node size and the element size from the allocation.

A container instantiation is library code. The tree writes the operator or the algorithm call the
original wrote, not a reconstructed body.

### Integer widths

The original toolchain's `long` is eight bytes wide and its pointers are four. The tree writes a
value the image stores in eight bytes as `long long`. It retains `long` only where it mirrors an
interface declared with `long` (the interpreter and its C++ binding, the `IBStream` and `OBStream`
overloads, and a `%ld` format argument).

### Platform division

A class whose title begins `Ps` is the PlayStation 2 implementation of the portable class above it.
Reconstruction covers the portable and the PlayStation 2 sides. A stub for another port is marked
as a stub and does not invent a second platform's behaviour.

The original was built against Sony's official SDK. Every SDK call is checked against its
declaration in ps2sdk, and a forwarding shim resolves header differences. Routines ps2sdk provides
are not reconstructed. Routines of the official SDK that ps2sdk lacks are owed code and are
reconstructed like game code.

## Methodology notes

A routine that prints its identifier in a diagnostic is authoritative about it. That recovered
`RestoreSurfaces` after this tree had called it `OnAllMipsLoaded`.

A value in the return register is a return only when every exit agrees on it and the value is not an
address.

A call site that materialises an argument proves the prototype declares it. Only the callee proves
whether the body reads it.

A type function builds its bases before itself. The owner is the class whose descriptor the routine
tests at entry, not the first constructor call inside it.

A routine testing its second argument against `0xffff` and branching on its first is the
per-translation-unit static-initialisation glue of this compiler. It is not source.

A trailing all-zero vtable entry is a terminator rather than a null slot.

Two byte-identical short bodies at different addresses are usually two distinct trivial overrides
of one pure virtual. One address appearing in several vtables is a shared base body.

A reported success from the disassembler bridge does not prove a write persisted. Every write is
re-read, and a batch is applied in a loop until the read-back agrees.

A forward declaration belongs inside the class's namespace. A global `class Mat;` for `Rnd::Mat`
declares a second unrelated type, and the failure appears in a distant implementation file.

A detector that pattern-matches source blanks the comments first. An unchanged count after adding an
input is a failure report rather than a stable baseline.

An address built by a `lui` and `addiu` pair is evaluated rather than read off. The low half is
signed, and `addiu v0,v0,0x88c0` subtracts 0x7740.

Where another unit expands a routine without calling it, the routine was inline in a header. The
compiler cannot inline a body it does not see.
