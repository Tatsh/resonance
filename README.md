# FreQuency reconstruction

Reconstructed C++ source for Harmonix's _FreQuency_ (PlayStation 2, `SCUS-97125`, 2001), recovered
from the shipped ELF by reading its MIPS R5900 disassembly.

The target is the PlayStation 2. Where a subsystem is inherently PS2 hardware (IOP module loading,
the GS/GIF display path, VU1 microcode), the reconstruction treats the PS2 code as the primary path
and includes a clearly marked stub for a future PC port behind `FREQ_PLATFORM_PS2`.

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
