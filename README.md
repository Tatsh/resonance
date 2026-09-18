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
`this + vptr[n].delta`. Slot 0 is the class's `GetTypeInfo`, slot 1 its destructor (with the
g++ 2.x `__in_chrg` argument), and user virtuals start at slot 2.

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

A list node is `{ next, prev, value }`, so the value of a node sits at `+0x08`. The pool rounds
the 12 bytes up to a 16-byte bucket.

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
- Order the sections public, then protected, then private.

An inline member is reconstructible only when its body appears at the call sites. The `HxStr`
destructor qualifies, because `if (mStr != 0) MemFree(mStr)` appears inline at every site that
destroys a string. A trivial accessor does not qualify: a `Str()` that returns `mStr` and a public
`mStr` compile to the same single load, so the image cannot distinguish them. Where the two
readings are indistinguishable, prefer the public member, because it asserts a property while an
invented accessor adds a function that no address can be attached to.

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

## Start-up

`entry` (`0x00458688`) is Sony's stock `crt0.s` and is not reconstructed. It clears `.bss`, calls
`SetupThread` and `SetupHeap`, runs `__do_global_ctors`, enables interrupts, and invokes `main`.
The call to `__main` at the top of `main` is likewise compiler-generated. `main` itself is in
[src/main.cpp](src/main.cpp).
