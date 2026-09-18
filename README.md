# FreQuency reconstruction

Reconstructed C++ source for Harmonix's _FreQuency_ (PlayStation 2, `SCUS-97125`, 2001), recovered
from the shipped ELF by reading its MIPS R5900 disassembly.

## Platform division

The target is the PlayStation 2. The shipped game was PS2 only, so the original source had no
platform conditionals whatsoever. Every `#if FREQ_PLATFORM_PS2` is therefore an addition rather
than a recovery, and one that misstates the original if it appears inside a reconstructed body.

A reconstructed function reproduces the original. It gains no conditional the original did not
have. Platform divergence belongs at the module boundary instead. The header stays common, a PS2
implementation file provides the real body, and a port supplies its own file for the same header
with the build selecting between them. `main` calls `InitIop()` and `LoadIopModules()`
unconditionally, exactly as the image does, and a port that has no IOP satisfies those two
declarations with its own file.

Threads, semaphores, files, heaps, and timers are not PS2-specific and must not be restricted. What
is genuinely PS2-only is IOP module loading, the GS and GIF register path, VU microcode, the EE
`Count` register, the scratchpad at `0x70000000`, and a `cdrom0:` path. Even for those, prefer a
separate implementation file over a conditional inside a recovered routine.

## Facts recovered from the binary

Every naming decision below rests on these recovered facts. They are recorded here to make the
reconstruction consistent across subsystems.

### Toolchain

The game was built with Sony's EE GCC (`/usr/local/sce/ee/gcc`, g++ 2.9x) and that compiler's STL
(`bastring.cc` and `stl_alloc.h` appear in assert strings). RTTI and exceptions are both enabled.
The image includes `.gcc_except_table`, `.ctors`, and `.dtors` sections.

### Virtual-function ABI (g++ 2.x "old" ABI)

A vtable is an array of 8-byte entries rather than an array of function pointers.

| Offset | Type    | Meaning                              |
| ------ | ------- | ------------------------------------ |
| `+0x0` | `short` | `delta`, the adjustment for `this`   |
| `+0x2` | `short` | `index`, unused in this build        |
| `+0x4` | `void*` | `pfn`, the function                  |

A virtual call therefore reads `vptr[n].pfn` at `8 * n + 4` and invokes it with
`this + vptr[n].delta`.

Slot 0 always holds the compiler-generated `GetTypeInfo`. Every slot after it holds one of the
class's own virtuals **in declaration order**, and the destructor occupies whichever slot its
declaration earned. The destructor is not fixed at slot 1. `Globals` declares its destructor first,
so its destructor is slot 1 and its two pure virtuals are slots 2 and 3. `Rnd::Stream` declares ten
transfer virtuals ahead of its destructor, so its eleven-entry table ends with the destructor at
slot 10, which the `Rnd::FileStream` table at `0x008261d8` confirms. Read the order off the table
rather than assuming it, and remember that a destructor in the table takes the g++ 2.x `__in_chrg`
argument.

A class with no base class stores its vptr **after** its data members rather than at offset 0.
`Globals` stores its vptr at `+0x1c`. Virtual base subobjects are placed at the end of the most
derived object.

### RTTI

Every polymorphic class has a lazily-initialising accessor of this shape.

```cpp
static type_info ti;              // in .bss or .scommon
if (ti.mName == NULL) {
    /* force base accessors */
    ti = type_info("<mangled name>", bases, nBases);
}
return &ti;
```

`type_info` is `{ const char *mName; void *mVptr; base_info *mBases; int mBaseCount; }`. The three
constructors in use are `__user_type_info` (`0x00478900`, leaf), `__si_type_info` (`0x004788e0`,
one non-virtual base), and `__class_type_info` (`0x004788b8`, general). A `base_info` is 8 bytes,
`{ const type_info *mBase; unsigned mOffset : 29, mVirtual : 1, mPublic : 1; }`.

**Class names come from these descriptors and are authoritative.** Each name is stored g++ 2.x
mangled. `Q23Rnd4Mesh` demangles to `Rnd::Mesh`, `Q33Rnd10Animatable6Filter` to
`Rnd::Animatable::Filter`, and `11Application` to `Application`. The renderer therefore lives in
namespace `Rnd` (`Rnd::Mesh`, `Rnd::Tex`, `Rnd::View`, `Rnd::Manager`, and the rest) rather than in
classes titled `RndMesh`.

A `dynamic_cast` compiles to a call to the runtime helper at `0x005570e0`, which receives the
source and target `GetTypeInfo` functions as arguments.

### Compiler-generated code is never written

The reconstruction includes only what a programmer wrote. Everything the compiler synthesised is
recognised, documented, and then omitted from the source. Modelling an artefact as a Ghidra
structure field or naming its function in the program is correct and helps the decompiler, but
none of it belongs in a header or an implementation file.

- The vptr. A class with no base stores it after its data members, and a polymorphic subobject
  stores one at its own offset. `Rnd::Drawable` stores its vptr at `+0x10`, which its header
  records in prose rather than as a member.
- The virtual-base pointer at offset 0 of a subobject that inherits virtually.
- Vtable slot 0, the `GetTypeInfo` accessor, and the `__in_chrg` argument on a destructor.
- The four-step virtual dispatch. A vptr load, a signed halfword `delta` read at `8 * n`, a `pfn`
  read at `8 * n + 4`, and a `this + delta` adjustment together are one source statement, a plain
  virtual call.
- A `this` adjustment onto a base subobject. A call that receives `pView + 0x18` is
  `pView->Draw()`, because `Rnd::View` places its `Rnd::Drawable` subobject at `+0x18`.
- The static initialisation stub for a global with a constructor, which has the g++ 2.x signature
  `(int __initialize_p, int __priority)` and tests `__priority == 0xffff`. The source is the global
  definition and its constructor arguments.
- `__main`, `__do_global_ctors`, and `crt0.s`.

### STL container layout

The STL is the SGI implementation that g++ 2.9x shipped, and its layout differs from a modern
library in a way that changes every recovered structure.

A `std::list` is **one pointer**, four bytes. The constructor allocates a single dummy node and
self-links it, so an empty list is a pointer to a 16-byte node whose `next` and `prev` both address
the node itself. A list member therefore occupies four bytes rather than eight or sixteen.
`Rnd::Object` proves it: the constructor at `0x0053e0d8` takes a 16-byte node from the pool at
`0x00667080`, writes it to `this + 0x00`, self-links it, and then constructs the `HxStr` name at
`this + 0x04`.

A list node is `{ next, prev, value }`, so the value of a node sits at `+0x08`. A node of a list
of pointers is allocated as 16 bytes, which is the 12 bytes of the node rounded up to the pool's
bucket. Node size follows the element type rather than a fixed bucket, so measure it: the
`Watchdog` list allocates 0x18 bytes per node, which puts a 16-byte value at `+0x08`.

Do not assume the size of any other container. Measure `std::map` and `std::vector` members
against the disassembly rather than against a modern layout.

The STL is toolchain code, so it is not reconstructed, for the same reason the PlayStation 2 SDK
and the embedded Python are not reconstructed. No replacement container library is written here.
The layout difference above is a fact to apply while recovering a structure, not code to produce.
Built with the real SCE toolchain, `<list>` produces the correct layout without help.

Every standard-library type in the image is mangled unqualified, `9bad_alloc`, `9exception`,
`7fstream`, and `16__user_type_info` among them, and none is mangled `Q23std...`. The build
therefore did not use `-fhonor-std`, so `namespace std` resolved to the global namespace and both
`list<T>` and `std::list<T>` named one type with one mangling. Which spelling the original source
used is not recoverable. The reconstruction writes `std::` for containers by convention, because
that spelling was legal and layout-identical in this build and also compiles under a modern
library, which is what makes the headers checkable here. The pre-standard iostreams are written
unqualified, as `ostream`, because `<iostream.h>` declared them in the global namespace in every
configuration.


### Access specifiers are inferred

Access control is resolved at compile time and survives nowhere in the image, so every specifier is
an inference rather than a recovered fact. These rules produce a consistent result, and the
evidence for each promotion belongs in the class documentation.

- A plain data record with no behaviour stays a `struct` with public members. `Zone`, `IopModule`,
  `MemTagTotal`, and `Timer` are records of this kind.
- A class with behaviour declares its data members private.
- A data member becomes protected when the code of a derived class reads or writes it.
- A data member becomes public only when code outside the class and its derived classes reads or
  writes it directly, and the image has no accessor for it. State that evidence, because a direct
  field access from foreign code is frequently an inlined accessor rather than a public field.
  Access from another instance of the same class proves nothing, since private access permits it.
- A method is public when a call site outside the class hierarchy exists, protected when only
  derived classes invoke it, and private when only the methods of its own class invoke it.
- A virtual override takes the access of the base declaration.
- Order the **method** sections public, then protected, then private.
- **Data members are declared in recovered offset order**, and access specifiers interleave between
  them as the inference requires. Layout fidelity outranks section tidiness, because reordering two
  members to group them by access silently changes every offset after the first of them. `FailSink`
  declares a public handler at `+0x00`, two private members at `+0x04` and `+0x08`, and a public
  dump level at `+0x0c`, in that order.

An inline member is reconstructible only when its body appears at the call sites. The `HxStr`
destructor qualifies, because `if (mStr != 0) MemFree(mStr)` appears inline at every site that
destroys a string. A trivial accessor does not qualify: a `Str()` that returns `mStr` and a public
`mStr` compile to the same single load, so the image cannot distinguish them. Where the two
readings are indistinguishable, prefer the public member, because it asserts a property while an
invented accessor adds a function that no address can be attached to.

The dividing line is necessity rather than taste. An inferred member is acceptable when the code
cannot be expressed without it. `HxStr::Mid()` returns a string by value, so something has to
construct that return value from the buffer it just allocated, and the private constructor that
adopts a buffer and a length is therefore structural. An accessor that only wraps a field access is
not structural, because the reader can name the field.

Whatever stays public is part of the class's documented surface, so every public member takes a
Doxygen comment. A private member uses a plain `//` comment, because it is internal commentary.
For a public data member, fold the recovered offset into the trailing member comment so that one
comment serves both purposes.

```cpp
class Transformable : public virtual Object {
public:
    /**
     * Report the transformable that positions this one.
     *
     * @return The parent transformable, or null when no referrer positions this one.
     * @ghidraAddress 0x004f0770
     */
    Transformable *Parent();

    float mWorldXfm[kXfmRowCount][kXfmRowFloatCount]; /*!< Composed world transform. +0x50 */

private:
    std::list<Transformable *> mTransList; // +0x04
    int mUnknown08;                        // +0x08
};
```

### Assert text is recovered data

`HX_ASSERT` stringifies its argument with `#expression`, and that text survives in `.rodata`. The
image stores `mStr != 0` at `0x008211d0`, alongside `HxStr.cpp` and the line number, so the
argument is written as `mStr != 0` rather than as `mStr != nullptr`. Rewriting it would emit a
different string constant and break the match against the image. Treat an assert argument as
recovered data, exactly like a class name.

The house preference for `nullptr` still governs ordinary pointer comparisons, including the
inline destructor in [include/os/hxstr.h](include/os/hxstr.h). A 2001 g++ 2.9x build had no
`nullptr` keyword in any case, so the original could only have written `0` or `NULL`, and the
string constant proves which.

These assert strings are also how the reconstruction recovers line numbers. A file name plus a
line plus the failed expression pins each check to its original source line.

### Naming policy

- A class name attested by RTTI is used verbatim, including its namespace.
- A member or function name attested by an assert or log string is used verbatim. Harmonix prefixed
  members with a bare `m`, recovered from the `HxStr.cpp` assert `mStr != 0`. Reconstructed members
  therefore retain that form rather than the house `m_` style, because fidelity to the original
  source wins for recovered identifiers.
- A class with no RTTI (a non-polymorphic class emits none) is titled after its embedded `__FILE__`
  basename, its allocation tag, or its methods, and its header marks the name as inferred.
- Every reconstructed routine includes `@ghidraAddress` for checking against the binary.
- A global that points at a NUL-terminated string takes the `g_sz` prefix rather than `g_psz`, in
  both the source and the Ghidra label, so that the two agree. The tooling enforces `sz` for a
  `char *` global and rejects `psz`, and `reconstruction.md` requires reconstructed globals to
  retain their Ghidra names, so the source follows. Parameters and data members are not subject to
  that check and keep the `psz` and `p` prefixes, which is why `g_szLastFailure` is assigned from a
  parameter titled `pszMessage`.

### Source layout evidence

Only a handful of original paths survive in the image. `C:/FREQ/src/rndartt/abitmap.h` fixes the
tree root at `src/`, and these basenames appear in assert and allocation strings: `ArkFile.cpp`,
`GrooveWorld.cpp`, `Heap.cpp`, `HxStr.cpp`, `MetFreqLoader.cpp`, `MetRemixManager.cpp`,
`async.cpp`, `cxx_extensions.cpp`, `loadfile.cpp`, `midi_main.cpp`, `rndtex.cpp`, `seccache.cpp`,
`zone.cpp`. Directory titles beyond `rndartt` are not attested. Headers are therefore grouped by
subsystem under `include/`, and implementations mirror that grouping under `src/`.

### Memory

The game allocates through an instrumented allocator that records the caller's file and line.

| Address      | Signature                                         |
| ------------ | ------------------------------------------------- |
| `0x004a8520` | `void *MemAllocTagged(size_t, const char *, int)` |
| `0x004a8380` | `void *MemAlloc(size_t)`                          |
| `0x004a94e8` | `void MemFreeTagged(void *, const char *, int)`   |
| `0x004a92c8` | `void MemFree(void *)`                            |

`MemAllocTagged` accumulates byte counts into a 24-entry table keyed by the tag string. Class names
such as `Rnd::Manager` and pool titles such as `stl_list` therefore appear as string literals
throughout `.rodata`. Blocks live in one of twelve named zones (`zone.cpp`).

## Embedded Python

The game embeds a trimmed fork of **CPython 2.0**, the BeOpen release, and it is reconstructed
under `src/python/`. Five independent markers fix the version.

- The copyright banner at `0x007b25e0` is `getcopyright.c` from 2.0 verbatim, with the BeOpen,
  CNRI, and Stichting parts and no Python Software Foundation line. 1.6 has only the CNRI part, and
  2.0.1 onward add the foundation line.
- `Objects/unicodeobject.c`, `ucnhash`, and `UnicodeError` are present. 1.5.2 has no Unicode at all.
- The `\N{...}` named escape, whose failure text is `Unicode name missing closing brace`, arrived in
  2.0.
- The image stores the bare version string `2.0`, the build date `Oct 12 2001`, and the compiler
  banner `[GCC 2.95.2 v2]`, which `Py_GetCompiler()` produces. That date matches the shipped ELF.
- The release archive for 2.0 is titled `BeOpen-Python-2.0.tar.gz`, matching the banner.

### Differential reconstruction

The core is upstream, so it is not re-derived from the disassembly. The method is to take the
upstream file, verify it against the image, and record only what the port changed. Matching every
string literal of each upstream `.c` file against the image identifies which translation units are
compiled in, and several match completely: `Modules/cPickle.c` at 72 literals of 72,
`Objects/abstract.c` at 60 of 60, `Modules/cStringIO.c` at 40 of 40, `Modules/arraymodule.c` at 34
of 34, `Modules/stropmodule.c` at 20 of 20, and `Modules/structmodule.c` at 19 of 19. A file whose
literals all appear is near-verbatim upstream and needs verification rather than recovery.

Around 55 upstream translation units are compiled in, and 73 are absent. The absent set is the
optional and platform modules, `_cursesmodule.c`, `bsddbmodule.c`, `dbmmodule.c`, `dlmodule.c`,
`audioop.c`, `binascii.c`, `cdmodule.c`, `clmodule.c`, `cmathmodule.c`, and the rest. Both regular
expression engines survive, the old `regexmodule.c` with `regexpr.c` and `pcremodule.c` with
`pypcre.c`, alongside the newer `_sre.c`. `Modules/posixmodule.c` is compiled in but heavily
trimmed, matching 3 literals of 143.

A low match ratio needs checking before it counts. `_tkinter.c`, `almodule.c`, and `mmapmodule.c`
each matched one literal, and in every case the text is shared with another file that is genuinely
present, so all three are absent.

### Three layers, three homes

The embedded interpreter is three separate bodies of code and they do not share a directory.

Upstream CPython 2.0, verbatim or nearly so, goes under `src/python/` mirroring the upstream
layout. A file whose literals all match upstream is verified against the image rather than
recovered from it, and any difference found is recorded in that file.

The C++ binding layer is **PyCXX**, a third-party wrapper, and it goes under `3rdparty/`. The RTTI
proves the whole library is linked in, 29 descriptors in namespace `Py`: `Object`, `Int`, `Float`,
`Char`, `String`, `Tuple`, `List`, `Dict`, `Callable`, `Type`, `Module`, `MethodTable`,
`PythonType`, `PythonExtensionBase`, `ExtensionModuleBase`, `FromAPI`, the `SeqBase` and `MapBase`
templates, and an exception hierarchy of `Exception`, `StandardError`, `AttributeError`,
`LookupError`, `KeyError`, `NameError`, `RuntimeError`, and `TypeError`. `Py::PythonExtensionBase`
derives from `_object`, which is how a C++ class becomes a Python object.

The game's own script host is Harmonix code and belongs with the rest of the game rather than under
`src/python/`. `RunMasterInitScript` at `0x00508e0c` runs `Global/GrvScript.py`, and the host
reports failures as `python error: `, `no python exception found`, and
`while initializing PyShell`, while fetching `traceback_str` from the interpreter for the detail.

### Port differences found so far

**The allocator is replaced through upstream's own hook, and no upstream header is edited.**
`Objects/obmalloc.c` does not exist in 2.0; it arrived in 2.1. In 2.0 the allocator is a set of
macros in `Include/pymem.h` and `Include/objimpl.h`, each guarded by `#ifndef`, and `pymem.h`
documents that guard as the supported way to plug in a different allocator. The port defines
`PyCore_MALLOC` and its siblings in its own configuration header, so both upstream headers are
unmodified.

The object variants have to be overridden as well as the raw ones. `objimpl.h` defaults
`PyCore_OBJECT_MALLOC_FUNC` to `PyCore_MALLOC_FUNC` rather than to the `PyCore_MALLOC` macro, so
overriding only the raw macro would leave every object allocation calling `malloc`. The tags prove
both are overridden, since `tupleobject.c` and `unicodeobject.c` appear among them and both
allocate through the object interface.

The evidence at the call sites is that of the 150 callers of `Heap::Alloc`, 106 are in the
interpreter's address range, and each passes the `g_pPythonHeap` at `0x00723998` as the receiver
**together with `__FILE__` and `__LINE__`**, which upstream's macros do not take.

**There is no import hook.** The port keeps upstream's stdio import machinery and redirects the
file primitive underneath it, which is why nothing in `import.c` needed replacing.
`RunMasterInitScript` at `0x00508de8` composes an empty prefix with `Global/GrvScript.py`, opens it
with `fopen` in mode `r`, and hands the `FILE *` to `PyRun_File` at `0x0054eef0` with a start symbol
of 257, which is `Py_file_input`. Underneath, `fopen` reaches the SDK's `open` through
`0x00551750`, `0x00551670`, and `0x005da840`. So a script is read as a loose file through the SDK,
neither out of an archive nor frozen.

Three findings agree. The frozen table is upstream's stock test-module table, `getpathp.c` computes
`sys.path` in the ordinary way, and `LoadWholeFile` has no callers anywhere in the image, which is
what one would expect when the interpreter uses stdio and nothing needs a whole-file loader.

The whole port is therefore two substitutions, the allocator macros and the file primitive, plus
four deletions. The interpreter itself is left alone.

`Py_Initialize` builds that heap by selecting the zone titled `python`, taking the whole zone in one
`ZoneAlloc`, and constructing a `Heap` over it with first-fit and fatal-when-full. The zone is
2400 KiB in the start-up table, above the 2 MiB fallback `Py_Initialize` passes, so the fallback
never applies on the shipped configuration. Exhaustion reports `Python heap is out of memory!` and
stops the machine.

**The port reused the Windows build, not the Unix one.** `Modules/getpath.c` is absent and
`getpathp.c` is present, and upstream keeps the latter at `PC/getpathp.c`. A console has no Unix
filesystem layout, and the Windows path module is the one already written not to assume one.
`PCacceler.c` corroborates the same choice, since upstream keeps that file at `Parser/acceler.c`
and only the `PC/` tree uses the prefixed spelling, so the build drew on `PC/` and flattened it.
`Modules/config.c` does not exist upstream at all, only a `config.c.in` that the Unix build
generates, while `PC/config.c` is a static built-in module table, so the table is very likely that
file with its module list cut to the set actually compiled in.

**There is no frozen standard library.** `Python/frozen.c` is unmodified: the image stores exactly
upstream's stock `__hello__`, `__phello__`, and `__phello__.spam` and nothing else. The scripts
therefore arrive through the game's own loader rather than frozen into the executable, which makes
the import hook in `Python/import.c` the open question in this area. That file matches 37 of its 47
literals, and the ten absent ones are the likely site of the edit.

`Modules/posixmodule.c` survives as almost nothing, 3 literals of 143, and the three name
`posix_confstr`, `posix_sysconf`, and `posix_abort`.

### The trim is configuration, not deleted code

An earlier version of this section recorded the trim as four deletions. That was wrong, and the
correction came from building the acceptance test rather than from more reading.

Almost every literal that upstream has and the image lacks sits inside an `#ifdef` the port does
not define, so the file is byte-identical upstream and there is nothing to patch. The undefined
macros are `WITH_THREAD`, which accounts for the import lock, `PyEval_AcquireThread`,
`PyEval_ReleaseThread`, and nine of `ceval.c`'s ten absent literals; `CHECK_IMPORT_CASE` for the
case-mismatch and find-file checks; `HAVE_DYNAMIC_LOADING` for `imp.load_dynamic`; `macintosh` for
`imp.load_resource`; `USE_STACKCHECK` for the stack overflow path; `CHECKEXC` for both undetected
error paths; `Py_TRACE_REFS` for `PYTHONDUMPREFS` and the reference dump; and `SIZEOF_TIME_T > 4`
for the timestamp overflow message. Upstream proves the mechanism directly: `import.c` line 144
opens `#ifdef WITH_THREAD` and lines 190 and 191 define `lock_import` and `unlock_import` away as
empty macros in the `#else`.

So the fork is far less invasive than deletion would imply. Its home is the undefined-macro list in
`src/python/PC/config.h`, with the evidence for each.

### The one patch

`src/python/patches/import.c.patch` is a single line: the `write_compiled_module` call in
`load_source_module`. That function is static and this was its only call site, so the compiler then
discards the function along with both of its verbose messages, which is what the image shows.
Read-only media cannot write a compiled module beside its source. No other file needs a patch.

### Acceptance test

`.wiswa-ci/freq/py_verify_patch.py` accounts for every absent literal as guarded, comment, dropped,
or patched, and fails on anything left over. It also checks the reverse direction, that every
literal present before patching survives, which catches a cut that removes a function holding a
live literal.

The suite currently exits non-zero with 34 unexplained literals, which is the honest state. One is
in `ceval.c`, three in `pythonrun.c`, and thirty in `posixmodule.c`, the latter being the `popen`,
`spawn`, `tmpnam`, and `strerror` argument messages together with the `MIPS_CS_*` entries. Each
group has a plausible explanation and none is demonstrated, so they stay itemised rather than
absorbed into a category to make the count reach zero.

The test has a limitation worth stating, because it bears on how much weight it can carry. It
cannot distinguish a real deletion from a macro that was never defined. A patch removing every
`WITH_THREAD` block from `ceval.c` would drive that file's count to zero and still be false, since
the port did not edit `ceval.c` at all. So a passing patch is necessary evidence and not sufficient
evidence, and a difference that lives in a macro belongs in the undefined-macro list rather than in
a diff. That is why only `import.c` has a patch.

### Module finding needs no hook

Upstream 2.0 does not use file metadata to find a module. `find_module` walks
`_PyImport_Filetab`, appends each suffix in turn, and calls `fopen(buf, fdp->mode)`, taking the
first that opens. `stat` appears in that function for one purpose only, the `S_ISDIR` test that
recognises a package directory. So a device with no file metadata is already served by the `fopen`
redirect, which is why the port needed no import hook.

No package directory appears in the shipped scripts. There is no `__init__.py` anywhere in the
extracted data, so `hx` resolves to the built-in module of that name in `_PyImport_Inittab`, and the
`gscripts/hx` directory is merely a location on `sys.path` holding the plain modules `hxcons` and
`hxutl`. The `S_ISDIR` path therefore has nothing in the shipped data to exercise it.

### Absent literals by file

Inverting the string comparison is what characterises the trim. A literal that upstream has and the
image lacks marks a deleted code path, and the deletions describe the port far better than the
additions do.

`Python/import.c` is missing ten literals of 47 and they group cleanly. The `.pyc` writing path, the
timestamp validation, `find_module`'s failure path, the case check, `load_dynamic` and
`load_resource`, the import lock, and the Windows registry import are all gone. `Python/ceval.c` is
missing ten of 60, four being the thread-state acquire and release interface, plus the stack
overflow and undetected error paths. `Python/pythonrun.c` is missing five of 36, including
`PYTHONDUMPREFS` and the reference-count debug.

So the fork is four deletions rather than a rewrite: no threading, no `.pyc` writing or timestamp
and case validation, no dynamic loading, and no reference-count debug. Each follows from the
platform, a read-only disc with no real filesystem and a single thread, and `thread.c`,
`threadmodule.c`, and every `dynload_*.c` being absent from the inventory agrees. `Python/pystate.c`
is present, so the single-threaded `PyThreadState` bookkeeping remains while the lock interface
around it is gone.

### Built-in modules and the ps2 module

`_PyImport_Inittab`'s name pool at `0x00741380` gives the table directly: `errno`, `_sre`,
`exceptions`, `sys`, `__builtin__`, `__main__`, `imp`, `marshal`, `_codecs`, `pcre`, `cPickle`,
`cStringIO`, `struct`, `strop`, `signal`, `regex`, `ps2`, `new`, and `array`, with `hx` and
`ucnhash` registered elsewhere.

`ps2` is the port's name for `Modules/posixmodule.c`. Neither `nt` nor `posix` appears anywhere in
the image, while `ps2` sits both in that name pool and in posixmodule's own string pool beside
`O_CREAT`, `O_EXCL`, and `O_TRUNC`. So the module is compiled in and registered under a new name
rather than reduced to data tables. The shipped `os.py` is modified to match: its platform chain
gains an `elif 'ps2' in _names:` branch at line 92, which selects `ntpath` for path handling and is
a third independent confirmation of the Windows lineage.

`hx` is the game's own extension module, exposed through PyCXX.

### The scripts are shipped as source

The game's Python is in the archives as `.py` text, not as bytecode, and no `.pyc` exists anywhere
in the extracted data. `ARK/ROOT/global/grvscript.py` is the file `RunMasterInitScript` runs; it
imports `os`, `os.path`, and `hx`, calls `hx.get_freq_root()`, and then executes
`global/defaults.py`. The host's `traceback_str` is a Python function, defined in
`ARK/ROOT/gscripts/hx/hxutl.py`, which closes the loop between the C++ host and the script layer.

The shipped standard library subset is `codeop`, `code`, `linecache`, `ntpath`, `os`, `stat`,
`string`, `traceback`, `types`, and `whrandom`. `posixpath.py` is not shipped, which is what forces
the `ntpath` branch. These files are data rather than reconstruction targets, but `os.py` carries a
port modification and so belongs in the difference record.

### Inventory by allocation tag

Matching string literals misses any file whose literals are all shorter than the threshold:
`Objects/sliceobject.c` and `Parser/node.c` are provably compiled in yet appear in neither the
present nor the absent list. For a file that allocates, the allocator tag is the better evidence,
because the edited macros pass `__FILE__`. The image stores 45 such tags, and two of them,
`cutscene.c` and `libscf.c`, are game files rather than interpreter files, so the tag list needs
that filter before it is used as an inventory.

## Start-up

`entry` (`0x00458688`) is Sony's stock `crt0.s` and is not reconstructed. It clears `.bss`, calls
`SetupThread` and `SetupHeap`, runs `__do_global_ctors`, enables interrupts, and invokes `main`.
The call to `__main` at the top of `main` is likewise compiler-generated. `main` itself is in
[src/main.cpp](src/main.cpp).
