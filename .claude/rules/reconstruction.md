# Reconstruction methodology

This tree reconstructs a decompiled binary. These rules govern how to translate Ghidra output into
faithful C and C++. The coding style of the resulting source lives in
[c-cpp-objc.md](c-cpp-objc.md); this file is about getting from the binary to that source correctly.

- Tie a reconstructed routine to its binary function with the `@ghidraAddress 0x...` Doxygen tag (a
  custom tag in our Doxygen configuration) on its header declaration; the address is relative to the
  program's image base. In an implementation file this tag appears only inside a block body (see the
  Objective-C block rule in `c-cpp-objc.md`).
- Keep the binary's names. Reconstructed globals keep their Ghidra names (for example the
  `g_`-prefixed globals). Ghidra placeholder names (`FUN_*`, `DAT_*`, `PTR_*`) are never used as
  identifiers in reconstructed code; rename them descriptively and record the address with
  `@ghidraAddress`.
- Assume every Ghidra "free function" is really a class method until proven otherwise. First test
  whether it is an instance method: a pointer argument (in any position, not only the first) that the
  function treats as its object — reads/writes that object's fields, or is the receiver the name
  implies — makes it an instance method of that object's class (`obj->Method(otherArgs)`). If no
  argument is an object receiver, test whether it is a static method: does it construct, vend, or
  operate on one specific class (a `Class::shared()` singleton getter, a factory, a table/among a
  family keyed to one class)? Place it as a `static` member of that class. Only after both searches
  are exhausted — no receiver argument and no owning class — may it be reconstructed as a genuine
  free function. Singleton getters are always static methods named `shared()` on the vended class.
- Take a C++ class's name from its RTTI when RTTI is present: the Itanium `type_info` name string
  (and the demangled vtable/`type_info` symbol) is authoritative — use it verbatim as the class
  name, exactly as an Objective-C class name comes from the runtime metadata. When there is no RTTI
  (a non-polymorphic class emits none), name the class from its embedded `__FILE__` basename and its
  method/`__func__` names instead, and note that the name is inferred rather than RTTI-confirmed.
- Model real types, not decompiler artifacts. Use real struct fields (never `field_0xNN`) and real
  pointer, enum, and `bool` types (never `void *` for a typed pointer, an `int` that holds a
  pointer, or `undefined`/`undefined4`).
- Trailing `// +0xNN` offset comments record a struct layout that is still being recovered. A
  structure whose every member is settled does not need them and may be written without them; keep
  them only where the layout still has gaps — a reserved or padding placeholder, or a member whose
  purpose is undetermined — because there the offset is the only record of the gap. Where they do
  appear they are documentation only: do not `#pragma pack` or `static_assert` the layout, and
  never read or write a struct by a hardcoded offset. The recorded offsets do not hold on the
  64-bit target, so always go through named fields.
- `reinterpret_cast` is a smell: it usually hides a type or signature bug, especially a
  function-pointer callback ABI. Prefer real types and typed access, and replace such casts at crash
  sites. `void *` is likewise a major smell: use it only for a genuinely opaque raw byte buffer (for
  example the `const void *` data argument of an MD5 helper), never for a typed engine object — those
  get their real class type.
- `ptr + someOffset` is banned in general. It should appear nowhere in the reconstructed code.
- `dynamic_cast` is also a smell.
- Recover the true function signature. A Ghidra decompile that uses `in_*` pseudo-variables (for
  example `in_w1`, `in_x2`, `in_stack_*`), or lists them under its Parameters, is missing formal
  parameters: the function takes arguments the decompiler did not bind into the prototype. Never
  model such a function as taking fewer arguments than the `in_*` usage and the disassembly's
  register/stack reads prove (in particular, never as no-arg when it clearly is not) — fix the Ghidra
  prototype, then reconstruct the real signature. Scan for `in_*` whenever a signature looks empty.
- When reconstructing a C or C++ function you MUST get its signature correct first (per the `in_*`
  rule above), and then update ALL of its callers to match that corrected signature — both the
  Ghidra program (fix the prototype so every call site re-decompiles cleanly) and any already-written
  reconstructed source that calls it. A signature fix is not complete until every caller agrees with
  it; a corrected callee with stale callers is a defect, not a finished routine.
- Fix the Ghidra program itself, not only the reconstructed source. As you work a function, in
  Ghidra: give every parameter, local, and return a real type (never a bare `long`/`int`/`undefined*`
  standing in for an object or struct pointer); rename every auto-named variable (`pnVar1`, `lVar2`,
  `uVar3`, `iVar4`, `pcVar5`, …) to a meaningful name; rename and type every `DAT_*`/`FUN_*`/`PTR_*`
  global as it is encountered; and create the real `struct`/`class` types so that offset-and-cast
  access (`*(int *)(in_x0 + i * 4 + 0x28)`) becomes a named field access (`p->nSpriteCount`). A
  function whose first argument is a pointer to a structure is almost always an instance method of
  that structure's class — model it as one. This applies even when the cast is taken _adjacent_ to
  an already-named field: `*(undefined1 *)((long)&x.field + 1) = 1` means a distinct field exists at
  that offset (or the neighbouring field is modelled wrong — for example a `ushort` that is really
  two bytes). Create or correct the struct field so the access is a clean named field; never leave
  such a cast behind.
- Flag surprising-but-faithful behaviour with a short comment so a reader does not mistake it for a
  reconstruction bug: a discarded return value, a call kept only for effect, a deliberate off-by-one,
  a value that looks wrong but matches the binary. Keep it terse — a trailing same-line comment where
  it fits, otherwise the line above (for example `(void)GetIsTallScreenFlag(); // Yes, the binary
discards this call's result.`). Do not write an extensive explanation.
- Scrutinise return values as hard as arguments: confirm the real return type and whether the value
  is actually returned/used (a discarded return, a returned `this`, or a bool-in-a-wider-register are
  all common), and fix the Ghidra prototype accordingly.
- The decompile is a usually incorrect guide, not the source of truth — work from the disassembly only.
- For a very large function body, save the decompiler output and the disassembler output to files,
  then break the function into parts by de-inlining the repeated or logically-distinct blocks into
  helper functions, and reconstruct using those helpers. Mark such helpers `inline` (not
  `__attribute__((always_inline))`) unless the block is genuinely performance-critical and you are
  certain the `always_inline` form will compile.
- Do not implement anything that is going to come from the PlayStation 2 SDK.
- When you write a source file, if you encounter a function/data structure/identifier not yet
  defined in this source, do NOT write forward declarations (seams). #include a header even if that
  header does not exist. If it does not exist yet, now is the time to start it.
