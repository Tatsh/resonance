# Embedded Python manifest

The game embeds a trimmed fork of CPython 2.0, the BeOpen release. This tree records only what the
port **changed**. Every other translation unit is upstream 2.0 and is listed below with its
evidence rather than copied, so the interpreter is reproducible as upstream plus the differences
recorded here.

The reference tree is vendored at `3rdparty/Python-2.0/`, the BeOpen release archive with
its top directory name preserved, so the interpreter is reproducible as upstream plus the
differences recorded here. The build compiles the port's own `PC/config.c` and `PC/config.h`
from this directory against those upstream headers and sources.

## Method

Two independent tests decide whether an upstream translation unit is compiled in, and they have
different blind spots, so the manifest uses both.

A **literal match** counts how many of a file's string literals of fourteen characters or more
appear in the image. It sees any file with distinctive text and misses a file that has none.

An **allocator tag** is stronger. The port's `PyCore_*` macros pass `__FILE__` and `__LINE__`, so
every file that allocates stores its own basename in `.rodata`. A tag is direct proof the file is
compiled in. It misses a file that never allocates.

Neither test alone is sufficient. `Objects/sliceobject.c` and `Parser/node.c` have no testable
literal at all and are invisible to the first test, while the tag list proves both are present.

Two of the 45 harvested tag names are **not** Python. `cutscene.c` and `libscf.c` are game files
that allocate through the same tagged allocator, and they belong with the game rather than here.

## Port differences

### The allocator, which is the one invasive change

`Objects/obmalloc.c` does not exist in 2.0; it arrived in 2.1. The allocator change lives instead
in the `PyCore_*` override hooks that `Include/pymem.h` and `Include/objimpl.h` already provide,
which upstream documents as the supported way to plug in a custom allocator. Both headers guard
their defaults with `#ifndef`, so a definition made before either is reached replaces `malloc`
without editing an upstream file. **`pymem.h` and `objimpl.h` are therefore unmodified.**

The port supplies those definitions in its configuration header, which is where `PC/config.h`
would carry them and where upstream carries none. The recovered behaviour is below. The size
argument comes from the macro's own parameter and the other two from the call site, which is what
makes every allocating translation unit store its own basename.

```c
#define PyCore_MALLOC(n) PyHeap_Alloc((n), __FILE__, __LINE__)
#define PyCore_REALLOC(p, n) PyHeap_Realloc((p), (n), __FILE__, __LINE__)
#define PyCore_FREE(p) PyHeap_Free((p), __FILE__, __LINE__)

#define PyCore_OBJECT_MALLOC(n) PyCore_MALLOC(n)
#define PyCore_OBJECT_REALLOC(p, n) PyCore_REALLOC((p), (n))
#define PyCore_OBJECT_FREE(p) PyCore_FREE(p)
```

The forwards go through plain C functions rather than through methods on the heap object, because
the interpreter itself compiles as C and a method call does not parse there. The functions forward
to the one interpreter heap and their names are build scaffolding, while the tagging they carry is
the observed behaviour.

The object variants must be overridden as well as the raw ones, because upstream defaults
`PyCore_OBJECT_MALLOC_FUNC` to `PyCore_MALLOC_FUNC` rather than to the `PyCore_MALLOC` macro, so
overriding only the raw macro would leave every object allocation going to `malloc`. The tags prove
they do not: `tupleobject.c` and `unicodeobject.c` both appear, and both allocate through the
object interface.

They are reconstructed in [PC/config.h](PC/config.h). The `_PyImport_Inittab` recovery settled the
host file: the module table is `PC/config.c`, so the sibling header is where the port's compiler
settings go.

`Py_Initialize` builds that heap over the whole of the zone titled `python`, which the start-up
table sizes at 2400 KiB, above the 2 MiB fallback the call passes, so the fallback never applies on
the shipped configuration. Exhaustion is fatal and reports `Python heap is out of memory!`.

### Script loading, where the port does nothing at all

There is **no import hook**. The port keeps upstream's stdio-based import machinery and redirects
the file primitive underneath it instead, which is why no replacement was needed anywhere in
`import.c`.

`RunMasterInitScript` composes an empty prefix with `Global/GrvScript.py`, opens it with `fopen`
in mode `r`, and hands the `FILE *` straight to `PyRun_File` with a start symbol of 257, which is
`Py_file_input`. Globals and locals are the same dictionary. The file is closed and a null result
is reported through the host's error path.

That `fopen` resolves through the game's own FILE layer down to the PlayStation 2 open primitive,
so a Python source file is read through the SDK rather than through any interpreter-side hook. It
is not frozen, which three separate findings agree on: the frozen table is upstream's stock
test-module table, `getpathp.c` computes `sys.path` from its defaults, and the game's
`LoadWholeFile` has no callers anywhere in the image.

The scripts ship as **`.py` source text** in the archives, with no `.pyc` anywhere, so the
interpreter compiles every script at run time. `ARK/ROOT/global/grvscript.py` is the file
`RunMasterInitScript` runs; it imports `os`, `os.path`, and `hx`, calls `hx.get_freq_root()`, and
executes `global/defaults.py`. The host loop closes back into Python, because `traceback_str` is a
function in `ARK/ROOT/gscripts/hx/hxutl.py` rather than a C symbol.

The shipped library subset is `codeop`, `code`, `linecache`, `ntpath`, `os`, `stat`, `string`,
`traceback`, `types`, and `whrandom`. `posixpath.py` is absent, which forces the `ntpath` branch.

`find_module` does not need modification for the device, and the reason is structural. Its one
port edit, a skipped prefix list described under the edits below, does not concern the device.
Upstream 2.0 does not stat to find a module. It walks `_PyImport_Filetab` and calls
`fopen(buf, fdp->mode)` once per suffix, taking the first that opens. The `fopen` redirect
therefore already serves a device with no file metadata, and the device does not require another
change. The same structure is also why the absent `Can't find file for module` does not prove
anything about the search. The literal belongs to the case-check path, not to the loop that does
the finding.

`stat` appears in `find_module` for one purpose only, the `S_ISDIR` test that recognises a package
directory, and nothing in the shipped data exercises it. There is no `__init__.py` anywhere, and
`hx` is in `_PyImport_Inittab`, so `import hx` resolves to the built-in C module rather than to a
directory. `gscripts/hx` is only a location on `sys.path` holding two plain modules, `hxcons` and
`hxutl`. The port has no working `stat` at all. It always fails, as the system call section
below records.

### Path and configuration, taken from the Windows build

`Modules/getpath.c` is absent and `getpathp.c` is present. That is `PC/getpathp.c`, the **Windows**
path module, which is the one already written not to assume a Unix filesystem layout. A console has
no such layout, so the port took it rather than writing a replacement.

The tag `PCacceler.c` corroborates the same conclusion from the other direction, because only the
`PC` tree uses that spelling while upstream has `Parser/acceler.c`. The build therefore drew on
`PC/` and flattened it.

The built-in module table is `PC/config.c` with its list replaced, which the `_PyImport_Inittab`
name pool at `0x00741380` confirms rather than infers. Upstream has no `Modules/config.c` at all,
only a `config.c.in` the Unix build generates, so the PC file is the only candidate. The nineteen
entries are reconstructed in [PC/config.c](PC/config.c).

**`ps2` is the port's name for `posixmodule.c`.** That settles whether its three surviving literals
were live code or orphaned tables: they are code. Neither `nt` nor `posix` appears anywhere in the
image, while `ps2` appears five times, so the module is compiled in and renamed rather than reduced
to data. The shipped `os.py` agrees, because its platform chain gains an `elif 'ps2' in _names:`
branch, which is a third independent confirmation of the Windows lineage after `getpathp.c` and
`PCacceler.c`.

`hx` and `ucnhash` are registered from outside the table. `hx` is the game's own extension module.

### Build configuration, taken from the toolchain

The build defines `_GNU_SOURCE` with the hosted feature macros (`HAVE_UNISTD_H`,
`HAVE_FCNTL_H`, `HAVE_SIGNAL_H`, `HAVE_UTIME_H`, `HAVE_SYS_WAIT_H`, `HAVE_DIRENT_H`,
`HAVE_NETDB_H`, `HAVE_SYS_SOCKET_H`, `HAVE_PROTOTYPES`, and `HAVE_STDARG_PROTOTYPES`) and the
console widths, because the toolchain headers provide the matching declarations. A forced include
of `PC/pycompat.h` supplies the C library headers, the `PYTHONPATH` default, and the socket
constants and name service declarations upstream expects from its configuration. The forced include
does not change which upstream blocks compile in.

### The trim is configuration, not code

An earlier reading of this called the trim four deletions of upstream code. That was wrong, and the
correction matters because it changes how invasive the fork is. Almost every absence is an upstream
`#ifdef` the port does not define. The file is therefore byte-identical upstream, and no edit
exists to write. The undefined macros are listed with their evidence in
[PC/config.h](PC/config.h).

`.wiswa-ci/freq/py_verify_patch.py` is the acceptance test and it accounts for every absent literal
under exactly one of four headings, because a test that only drives a missing count to zero cannot
work when the source is unmodified:

- **GUARDED**, behind a macro the port does not define
- **COMMENT**, quoted text the literal extractor matched inside a comment, so never a literal
- **DROPPED**, inside a static function that no caller references once the port's edit applies
  (the compiler discards the function and its literals)
- **PATCHED**, removed outright by the port's edit

It also rejects overcutting, by requiring that every literal the unedited file had present in the
image is still present after the edit. A naive check misses the overcutting half. Deleting a
function that includes a present literal does not raise the missing count, and only stops the
literal being checked.

A fifth heading, **ARTEFACT**, covers a fragment the extractor split across a quote boundary. Such
a fragment opens with a close paren or a comma, so it was never one literal.

**The suite exits non-zero, and it should.** Three literals remain unexplained out of the 165
absent, and the test reports that rather than absorbing them, which is the point of having it.

| File                    | Absent     | Account                                     | Verdict       |
| ----------------------- | ---------- | ------------------------------------------- | ------------- |
| `Python/import.c`       | 10 of 47   | 6 guarded, 2 comment, 2 dropped by the edit | accounted for |
| `Python/ceval.c`        | 10 of 60   | 9 guarded, 1 unexplained                    | 1 open        |
| `Python/pythonrun.c`    | 5 of 36    | 2 guarded, 2 artefact, 1 unexplained        | 1 open        |
| `Modules/posixmodule.c` | 140 of 143 | 139 guarded, 1 unexplained                  | 1 open        |

Every guard was read off the literal's own enclosing block rather than assumed. The confstr,
sysconf, and pathconf table entries are guarded by an underscore plus the entry name, which is
tested exactly. The rest sit under autoconf feature macros the console does not satisfy,
`HAVE_EXECV`, `HAVE_POPEN`, `HAVE_TMPNAM`, `USE_TMPNAM_R`, `HAVE_FPATHCONF`, `HAVE_PATHCONF`,
`HAVE_STRERROR`, and `PYOS_OS2`.

### Edits to the vendored source

The port's changes are made in place in `3rdparty/Python-2.0`, each marked with a short comment at
the edit. The build compiles the edited files directly.

`Python/import.c` has `load_source_module` compile every source module without consulting or
writing a compiled module. Retail `load_source_module` at `0x0057c470` calls
`PyOS_GetLastModificationTime`, forms the compiled path (unused), parses, compiles, frees the
tree, and executes the code, and it makes no call to `check_compiled_module`,
`read_compiled_module`, or `write_compiled_module`. The static helpers then have no caller, and
their literals are absent from the image.

`Python/pythonrun.c` has `PyRun_SimpleFileEx` refuse a `.pyc` or `.pyo` file. Retail at
`0x0054ebc8` matches either extension, closes the file when `closeit` is set, and returns -1 without
reopening the file, printing, or setting an error. `run_pyc_file` is then never called.

`Python/pythonrun.c` also has `PyRun_SimpleString` write its result to `sys.stdout` before releasing
the result. Retail at `0x0054eac0` fetches `stdout` (`0x0082a190`) with `PySys_GetObject`, calls
`PyFile_WriteObject` with `Py_PRINT_RAW` and then `PyFile_WriteString` with `\n` (`0x0082a198`),
without testing the file or either result.

`Python/import.c` also has `find_module` skip the `fopen` for a candidate path that begins with
`.\DLLs;` or `cdrom0:\.\DLLs;`. Retail `find_module` at `0x0057c860` walks the null-terminated
table at `0x00766920` before each `fopen`, compares each prefix against the candidate with `memcmp`
over the prefix's `strlen`, and records a null file on a match.

`Python/pythonrun.c` calls `PyHeap_Init()` at the top of `Py_Initialize()`, before the first
allocation. The allocator section above describes the heap.

`Modules/posixmodule.c` registers under the console name. `INITFUNC` is `initps2` and `MODNAME` is
`"ps2"`, and `"posix"` is absent from the image. The method table is reduced as described under
the `ps2` method table below. Its `listdir` returns a new empty list without reading its arguments.
Retail `posix_listdir` at `0x00651a98` is only a call to `PyList_New(0)`.

`Modules/posixmodule.c` also retains upstream's Windows path handling in `posix_do_stat` although
`MS_WIN32` is not defined. Retail `posix_do_stat` at `0x006513a0` measures the path with `strlen`,
fails a path longer than 250 bytes with `errno` 91 through `PyErr_SetFromErrno`, and copies a path
ending in `\` or `/` into a stack buffer without the separator, sparing `/`, `\`, and a drive
root such as `c:/`. The edit removes the two `MS_WIN32` guards there and gives `MAX_PATH` its value
of 250.

`Modules/posixmodule.c` also omits `NGROUPS_MAX`, `WNOHANG`, `O_DSYNC`, and `O_RSYNC` from the
module dictionary. Retail `all_ins` at `0x006514d0` inserts fifteen names, `F_OK` through `TMP_MAX`
and `O_RDONLY` through `O_TRUNC`, with values that match this build's headers. It does not insert
the four that newlib defines. The edit undefines the four before `all_ins`.

The bare `malloc`, `realloc`, and `free` calls of every interpreter file arrive at the interpreter
heap with the call site's file and line. `PC/pycompat.h` therefore routes all three to the
`PyCore_*` macros for C translation units, after the C library prototypes. No vendored file is
edited for the routing. Retail
evidence covers each file with such calls: `_sre.c`'s mark stack at `0x00640954`, `0x0064098c`, and
`0x006409ac`; `cPickle.c` at `0x0064a798` (`Pdata_grow` inlined into `load_binintx`);
`cStringIO.c` at `0x00656b70`, `0x00656ce8`, `0x006578dc`, and `0x00657aac`; `regexmodule.c`'s
`reg_dealloc` at `0x00652090`; `regexpr.c`'s `re_compile_pattern` at `0x00662034`; and
`getpathp.c` at `0x0056a278`. The `posixmodule.c` calls are in functions this build does not
compile.

`Modules/pypcre.c` initialises `pcre_malloc` and `pcre_free` with two small functions over the
interpreter heap, because a function-like macro does not redirect a function name used as a value.
Retail has the two as separate functions at `0x00661040` and `0x00661070`. They call `Heap::Alloc`
and `Heap::Free` with the `pypcre.c` tag.

`Modules/cStringIO.c` raises its two `MemoryError`s with `python out of memory` (`0x008452e0`)
rather than upstream's `out of memory`. Retail passes the string from `O_cwrite` at `0x00657834`,
from `newOobject` at `0x00657ad8`, and from the inlined copies at `0x00656d08` and `0x00656ff8`.

The interpreter's `printf` is the game's `LogPrintf`. `PC/pycompat.h` sets the redirect for C
translation units. Retail `fixstate` passes `XXX too many states!` and
`XXX too high nonterminal number!` to `LogPrintf` at `0x005d9fb0` and `0x005da014`, and the
`Parser/assert.h` check in `PyGrammar_FindDFA` calls `LogPrintf` at `0x00629b64` before `abort`.
`fprintf` is unchanged, as at `0x0061c1fc`.

`PC/config.h` sets `DATE` and `TIME` to the port's build stamp. Retail `Py_GetBuildInfo` at
`0x006371f0` formats build 0 with `Oct 12 2001` and `12:05:12` for `sys.version`. `PC/config.h`
also sets `COMPILER` to `\n[GCC 2.95.2 v2]`, the string `Py_GetCompiler` at `0x0063f1d0` returns
for `sys.version`.

`Include/stringobject.h` and `Include/unicodeobject.h` drop the `register` storage class from nine
parameter declarations. The C++ standard no longer allows the storage class there. The keyword was
only a hint, and no behaviour changes.

`PC/config.h` defines `WITHOUT_COMPLEX`. The builtin method table at `0x0076c390` runs from
`compile` straight to `delattr`, and none of `complexobject.c`'s literals are in the image. The
shipped `types.py` therefore takes its `NameError` branch. `complexobject.c` is not compiled.

`PC/pycompat.h` sets the `PYTHONPATH` default to the empty string at `0x0082c9f0`. A `.` entry
would arrive at the archive lookup as a `./` path. `ArkFile::MapPathToArkIndex()` treats a `./`
path as fatal in retail and in the reconstruction.

Every compiled file and every header under `Include/` writes the C `long` as `Py_LONG`.
`PC/config.h` defines `Py_LONG` as `long long` together with `SIZEOF_LONG` 8, a `PY_LONG_BIT` of
64, and the matching limits. The port's compiler gave `long` 64 bits. `PyInt_AsLong` at
`0x00581c00` loads `ob_ival` with `ld`, `int_add` at `0x00581d98` adds with `daddu` and tests for
64-bit overflow, and `int_lshift` at `0x00581f38` compares the shift count against 64. The
toolchain this tree builds with retains a 32-bit `long` and rejects `-mlong64`. The rewrite
therefore covers the type, the `LONG_MAX`, `LONG_MIN`, `ULONG_MAX`, and `LONG_BIT` limits, `L`
literal suffixes, `%ld` formats (including the `%%%s.%dl%c` template that `formatint` in
`stringobject.c` and `unicodeobject.c` expands at run time, retail `0x0072e8e0` and `0x00733280`),
and the `atol`, `strtol`, `strtoul`, and `labs` calls. Comments are unchanged. `PC/config.h` also
defines `HAVE_LONG_LONG`, as upstream's `PC/config.h` does.

`Objects/longobject.c` has `PyLong_FromVoidPtr` and `PyLong_AsVoidPtr` take upstream's
`SIZEOF_VOID_P == SIZEOF_LONG` branches although the port's pointers are four bytes and
`SIZEOF_VOID_P` stays 4. Retail `PyLong_FromVoidPtr` at `0x0057ace8` is a call to `PyInt_FromLong`
with the pointer register passed through unchanged. The pointer widens sign-extended, and every
pointer becomes an int. Retail `PyLong_AsVoidPtr` at `0x00577718` reads an int's `ob_ival` or calls
`PyLong_AsLong`, narrows the result to the pointer, and consults `PyErr_Occurred` when the narrowed
value is -1.

The port's soft-float library compared doubles through one three-way compare with the operands in
source order, and the compare reports an unordered pair as greater. The toolchain moves a constant
to the right of a comparison before any later pass runs. A NaN against a constant written first
would otherwise give the opposite result. The four such comparisons call `freq_compare_double`
from the runtime instead: `CHECK` in `Objects/floatobject.c` (`float_pow` at `0x0047b1f8`), and the
`0.5 <= f` frexp range tests in `Modules/cPickle.c` and twice in `Modules/structmodule.c`. A NaN is
therefore out of range there, as in retail.

`ceval.c` needs no edit. `pythonrun.c` has one unexplained literal, and inventing an edit around it
would pass the acceptance test without being evidence. Any cut including the unexplained literal
would pass equally.

### C library system calls

The game supplies the C library's system calls, and two of them differ from a hosted library in a
way the interpreter depends on. `stat()` at `0x005966b8` sets `errno` to `EIO` and returns -1 for
every path. `getpathp.c` therefore never finds its landmark, and `find_module` never sees a
package directory. `fstat()` at `0x00596670` reports a character device for every descriptor and
succeeds. `PyOS_GetLastModificationTime` therefore succeeds for an archive stream. Both are in
`src/os/filelog.cpp` beside the other system calls.

### Still unexplained

Four literals resist all four headings, and they are recorded rather than papered over.

`ceval.c` lacks `standard sequence type does not support step size other than one`, which is
unguarded and in no static function.

`pythonrun.c` lacks `python: Can't reopen .pyc file` because the port cut the compiled-module
branch of `PyRun_SimpleFileEx`. Retail at `0x0054ebc8` closes the file and returns -1 for a `.pyc`
or `.pyo` name, and the edit is recorded under the edits to the vendored source. The literal is
therefore explained and no longer belongs under this heading. Its other two absences,
`) == 0 || strcmp(ext,` and `, v = PyString_FromString(`, are not literals at all but code fragments
the extractor mis-split across a quote boundary.

`posixmodule.c` lacks `Second argument must be a 2-tuple of numbers.` from `posix_utime`, which
sits under no guard at all.

## Compiled-in translation units

64 upstream units, by evidence. A tag is proof. A literal ratio is corroboration, and a low ratio
needs the checks noted underneath.

| Unit                      | Literals | Tag                                                    |
| ------------------------- | -------- | ------------------------------------------------------ |
| `Modules/cPickle.c`       | 72/72    | yes                                                    |
| `Objects/unicodeobject.c` | 66/70    | yes                                                    |
| `Objects/abstract.c`      | 60/60    |                                                        |
| `Python/ceval.c`          | 50/60    | yes                                                    |
| `Python/compile.c`        | 47/54    | yes                                                    |
| `Python/exceptions.c`     | 47/49    | yes                                                    |
| `Modules/_codecsmodule.c` | 40/42    |                                                        |
| `Modules/cStringIO.c`     | 40/40    | yes                                                    |
| `Modules/errnomodule.c`   | 40/181   |                                                        |
| `Python/getargs.c`        | 39/42    | yes                                                    |
| `Python/import.c`         | 37/47    | yes                                                    |
| `Objects/stringobject.c`  | 35/36    | yes                                                    |
| `Python/bltinmodule.c`    | 35/42    | yes                                                    |
| `Modules/arraymodule.c`   | 34/34    | yes                                                    |
| `Objects/classobject.c`   | 32/34    | yes                                                    |
| `Python/pythonrun.c`      | 31/36    | yes                                                    |
| `Objects/listobject.c`    | 23/24    | yes                                                    |
| `Modules/stropmodule.c`   | 20/20    | yes                                                    |
| `Modules/structmodule.c`  | 19/19    |                                                        |
| `Python/sysmodule.c`      | 19/21    |                                                        |
| `Objects/intobject.c`     | 18/20    | yes                                                    |
| `Modules/pcremodule.c`    | 14/14    |                                                        |
| `Modules/regexpr.c`       | 14/15    | yes                                                    |
| `Modules/socketmodule.c`  | 14/107   |                                                        |
| `Objects/fileobject.c`    | 14/17    | yes                                                    |
| `Modules/newmodule.c`     | 13/13    |                                                        |
| `Objects/floatobject.c`   | 13/13    | yes                                                    |
| `Objects/longobject.c`    | 12/12    | yes                                                    |
| `Objects/bufferobject.c`  | 11/11    | yes                                                    |
| `Objects/object.c`        | 11/20    | yes                                                    |
| `Modules/pypcre.c`        | 10/29    | yes                                                    |
| `Python/codecs.c`         | 10/10    |                                                        |
| `Python/pystate.c`        | 9/9      | yes                                                    |
| `Python/marshal.c`        | 8/9      | yes                                                    |
| `Modules/_sre.c`          | 7/10     | yes                                                    |
| `Modules/regexmodule.c`   | 7/7      | yes                                                    |
| `Parser/tokenizer.c`      | 7/7      | yes                                                    |
| `Objects/funcobject.c`    | 6/7      | yes                                                    |
| `Python/modsupport.c`     | 6/6      |                                                        |
| `Objects/cobject.c`       | 5/5      | yes                                                    |
| `Objects/rangeobject.c`   | 5/6      | yes                                                    |
| `Objects/methodobject.c`  | 4/4      | yes                                                    |
| `Objects/moduleobject.c`  | 4/4      | yes                                                    |
| `Parser/acceler.c`        | 4/5      |                                                        |
| `Python/errors.c`         | 4/5      |                                                        |
| `Modules/posixmodule.c`   | 3/143    |                                                        |
| `Modules/signalmodule.c`  | 3/5      |                                                        |
| `Objects/frameobject.c`   | 3/4      | yes                                                    |
| `Objects/tupleobject.c`   | 3/3      | yes                                                    |
| `Python/structmember.c`   | 3/4      |                                                        |
| `Parser/parsetok.c`       | 2/2      | yes                                                    |
| `Python/graminit.c`       | 2/2      |                                                        |
| `Python/traceback.c`      | 2/3      | yes                                                    |
| `Objects/dictobject.c`    | 1/1      | yes                                                    |
| `Objects/typeobject.c`    | 1/1      |                                                        |
| `Parser/myreadline.c`     | 1/1      |                                                        |
| `Parser/parser.c`         | 1/7      | yes                                                    |
| `Python/frozen.c`         | 1/1      |                                                        |
| `Objects/sliceobject.c`   | none     | yes                                                    |
| `Parser/node.c`           | none     | yes                                                    |
| `PC/getpathp.c`           |          | yes                                                    |
| `PC/config.c`             |          | inferred, see above                                    |
| `PC/config.c`             |          | modified, reconstructed here                           |
| `PC/config.h`             |          | modified, allocator hooks reconstructed here           |
| `Lib/os.py`               |          | modified script, `ps2` branch at line 92               |
| `Modules/posixmodule.c`   |          | trimmed to twelve methods and renamed `ps2`, see above |

### Ratios that need a check before they count

`_tkinter.c` at 1/28, `almodule.c` at 1/129, and `mmapmodule.c` at 1/20 each matched a single
literal that another present file also contains, so all three are **absent**. `parsermodule.c` at
1/66 is the same shape and is very likely absent too, but `parser.c` carries a tag, so the
distinction between the parser module and the parser itself needs resolving.

`errnomodule.c` at 40/181 and `socketmodule.c` at 14/107 are present with most of their literals
missing, which is what a table-driven module looks like when the platform supports only part of it
rather than evidence of absence.

`Python/frozen.c` matches exactly one literal, `__phello__.spam`, which is upstream's own test
package. The frozen table is unmodified and no library is frozen into the image.

### The `ps2` method table

Twelve of upstream's ninety-four `posix_methods[]` entries survive. The literals could not answer
this, because a surviving error message does not imply a surviving method, so the evidence is the
name pool instead: the twelve names sit in one run of 0x58 bytes at file offset `0x745738`, while
every other occurrence of a candidate name in the image is scattered elsewhere and belongs to libc
or another module.

`listdir`, `lstat`, `stat`, `getpid`, `open`, `close`, `lseek`, `read`, `write`, `fstat`, `isatty`,
and `abort`.

That is a read-only file interface plus `getpid` and `abort`, which is what the rest of the port
predicts: the directory and metadata calls needed to import a module, the descriptor calls under
`fopen`, and nothing that mutates a filesystem. No `chdir`, no `mkdir`, no `unlink`, no `rename`,
no process control beyond `abort`. It also explains the third surviving literal, because `abort` is
a real method here.

The trim is a table, not deletions. The edit reduces `posix_methods[]` to the twelve entries above,
in the order of the run, each with its upstream doc string. The table at `0x007c7010` points every
entry at its doc (`listdir` at `0x007c6a88` through `abort` at `0x007c6f68`), and `initps2` at
`0x00651f20` passes `posix__doc__` (`0x007c6980`) to `Py_InitModule4`. No entry references the
other eighty-two bodies, and the compiler discards them with their doc strings. Discarding the
bodies removes their literals. The two configuration messages survive
because their shared helper is retained with `used` even though the trimmed table references
nothing that calls it; the image keeps those literals with no registered caller, so the helper
stays.
`confstr`, `sysconf`, `fpathconf`, and `pathconf` are all absent from the method table, and their
bodies go with the other discards; only the shared helper's two messages remain.

## PyCXX

The binding layer under `src/script/cxx/` is the port's modified PyCXX. The PyCXX 5.2 series
is contemporary with development and still supports Python 2.0. Later series dropped Python 2.0
support. The exact patch the port used is unrecoverable, because its version marker is likely a
header comment that compilation discards. The headers use the game's `HxStr` in place of
`std::string`, and `Py::Object` has no `owned` flag.

## Outstanding

`ceval.c` gets no edit, and the reason is not size. Nine of its ten absences are macros the port
does not define. An edit would therefore assert a change that never happened. The acceptance test
cannot tell the difference, and a passing edit is necessary evidence rather than sufficient
evidence.
