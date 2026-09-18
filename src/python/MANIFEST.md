# Embedded Python manifest

The game embeds a trimmed fork of CPython 2.0, the BeOpen release. This tree records only what the
port **changed**. Every other translation unit is upstream 2.0 and is listed below with its
evidence rather than copied, so the interpreter is reproducible as upstream plus the differences
recorded here.

The reference tree is at `.wiswa-ci/freq/Python-2.0/`.

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
would carry them and where upstream carries none. The recovered form is below. The size argument
comes from the macro's own parameter and the other two from the call site, which is what makes
every allocating translation unit store its own basename.

```c
#define PyCore_MALLOC(n)                g_pPythonHeap->Alloc((n), __FILE__, __LINE__)
#define PyCore_REALLOC(p, n)            g_pPythonHeap->Realloc((p), (n), __FILE__, __LINE__)
#define PyCore_FREE(p)                  g_pPythonHeap->Free((p), __FILE__, __LINE__)
#define PyCore_OBJECT_MALLOC(n)         PyCore_MALLOC(n)
#define PyCore_OBJECT_REALLOC(p, n)     PyCore_REALLOC((p), (n))
#define PyCore_OBJECT_FREE(p)           PyCore_FREE(p)
```

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
test-module table, `getpathp.c` computes `sys.path` normally, and the game's `LoadWholeFile` has no
callers anywhere in the image.

The scripts ship as **`.py` source text** in the archives, with no `.pyc` anywhere, so the
interpreter compiles every script at run time. `ARK/ROOT/global/grvscript.py` is the file
`RunMasterInitScript` runs; it imports `os`, `os.path`, and `hx`, calls `hx.get_freq_root()`, and
executes `global/defaults.py`. The host loop closes back into Python, because `traceback_str` is a
function in `ARK/ROOT/gscripts/hx/hxutl.py` rather than a C symbol.

The shipped library subset is `codeop`, `code`, `linecache`, `ntpath`, `os`, `stat`, `string`,
`traceback`, `types`, and `whrandom`. `posixpath.py` is absent, which forces the `ntpath` branch.

`find_module` needs no modification either, and the reason is structural. Upstream 2.0 does not
stat to find a module. It walks `_PyImport_Filetab` and calls `fopen(buf, fdp->mode)` once per
suffix, taking the first that opens, so a device with no file metadata is already served by the
`fopen` redirect and nothing else is required. That is also why the absent
`Can't find file for module` proves nothing about the search: it belongs to the case-check path,
not to the loop that does the finding.

`stat` appears in `find_module` for one purpose only, the `S_ISDIR` test that recognises a package
directory, and nothing in the shipped data exercises it. There is no `__init__.py` anywhere, and
`hx` is in `_PyImport_Inittab`, so `import hx` resolves to the built-in C module rather than to a
directory. `gscripts/hx` is only a location on `sys.path` holding two plain modules, `hxcons` and
`hxutl`. So the port may not need a working `stat` at all, which tightens the account to exactly
two substitutions, the allocator and the file primitive.

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

### The trim is configuration, not code

An earlier reading of this called the trim four deletions of upstream code. That was wrong, and the
correction matters because it changes how invasive the fork is. Almost every absence is an upstream
`#ifdef` the port simply does not define, so the file is byte-identical upstream and no patch
exists to write. The undefined macros are listed with their evidence in
[PC/config.h](PC/config.h).

`.wiswa-ci/freq/py_verify_patch.py` is the acceptance test and it accounts for every absent literal
under exactly one of four headings, because a test that only drives a missing count to zero cannot
work when the source is unmodified:

- **GUARDED**, behind a macro the port does not define
- **COMMENT**, quoted text the literal extractor matched inside a comment, so never a literal
- **DROPPED**, inside a static function that nothing references once a patch applies, so the
  compiler discards the function and its literals with it
- **PATCHED**, removed outright by a patch

It also rejects overcutting, by requiring that every literal the unpatched file had present in the
image is still present after patching. That is the half a naive check misses, since deleting a
function that holds a present literal does not raise the missing count, it merely stops the literal
being checked.

A fifth heading, **ARTEFACT**, covers a fragment the extractor split across a quote boundary. Such
a fragment opens with a close paren or a comma, so it was never one literal.

**The suite exits non-zero, and it should.** Three literals remain unexplained out of the 165
absent, and the test reports that rather than absorbing them, which is the point of having it.

| File | Absent | Account | Verdict |
| ---- | ------ | ------- | ------- |
| `Python/import.c` | 10 of 47 | 6 guarded, 2 comment, 2 dropped by the patch | accounted for |
| `Python/ceval.c` | 10 of 60 | 9 guarded, 1 unexplained | 1 open |
| `Python/pythonrun.c` | 5 of 36 | 2 guarded, 2 artefact, 1 unexplained | 1 open |
| `Modules/posixmodule.c` | 140 of 143 | 139 guarded, 1 unexplained | 1 open |

Every guard was read off the literal's own enclosing block rather than assumed. The confstr,
sysconf, and pathconf table entries are guarded by an underscore plus the entry name, which is
tested exactly. The rest sit under autoconf feature macros the console does not satisfy,
`HAVE_EXECV`, `HAVE_POPEN`, `HAVE_TMPNAM`, `USE_TMPNAM_R`, `HAVE_FPATHCONF`, `HAVE_PATHCONF`,
`HAVE_STRERROR`, and `PYOS_OS2`.

### The one patch

[patches/import.c.patch](patches/import.c.patch) removes a single line, the
`write_compiled_module` call in `load_source_module`. The function is static and that was its only
call site, so the compiler then discards it along with its two verbose messages, which is exactly
what the image shows. The port cannot write a compiled module next to a source file on read-only
media, and this is the smallest edit that produces that.

No other file gets a patch. `ceval.c` needs none. `posixmodule.c` needs none, because its trim is
configuration. `pythonrun.c` has one unexplained literal and inventing a patch shape around it
would pass the acceptance test without being evidence, since any cut containing that literal would
pass equally.

### Still unexplained

Four literals resist all four headings, and they are recorded rather than papered over.

`ceval.c` lacks `standard sequence type does not support step size other than one`, which is
unguarded and in no static function.

`pythonrun.c` lacks `python: Can't reopen .pyc file` from the compiled-module branch of
`PyRun_SimpleFileEx`, which is consistent with that branch being cut but not proof of it. Its other
two absences, `) == 0 || strcmp(ext,` and `, v = PyString_FromString(`, are not literals at all but
code fragments the extractor mis-split across a quote boundary.

`posixmodule.c` lacks `Second argument must be a 2-tuple of numbers.` from `posix_utime`, which
sits under no guard at all.

## Compiled-in translation units

64 upstream units, by evidence. A tag is proof. A literal ratio is corroboration, and a low ratio
needs the checks noted underneath.

| Unit | Literals | Tag |
| ---- | -------- | --- |
| `Modules/cPickle.c` | 72/72 | yes |
| `Objects/unicodeobject.c` | 66/70 | yes |
| `Objects/abstract.c` | 60/60 | |
| `Python/ceval.c` | 50/60 | yes |
| `Python/compile.c` | 47/54 | yes |
| `Python/exceptions.c` | 47/49 | yes |
| `Modules/_codecsmodule.c` | 40/42 | |
| `Modules/cStringIO.c` | 40/40 | yes |
| `Modules/errnomodule.c` | 40/181 | |
| `Python/getargs.c` | 39/42 | yes |
| `Python/import.c` | 37/47 | yes |
| `Objects/stringobject.c` | 35/36 | yes |
| `Python/bltinmodule.c` | 35/42 | yes |
| `Modules/arraymodule.c` | 34/34 | yes |
| `Objects/classobject.c` | 32/34 | yes |
| `Python/pythonrun.c` | 31/36 | yes |
| `Objects/listobject.c` | 23/24 | yes |
| `Modules/stropmodule.c` | 20/20 | yes |
| `Modules/structmodule.c` | 19/19 | |
| `Python/sysmodule.c` | 19/21 | |
| `Objects/intobject.c` | 18/20 | yes |
| `Modules/pcremodule.c` | 14/14 | |
| `Modules/regexpr.c` | 14/15 | yes |
| `Modules/socketmodule.c` | 14/107 | |
| `Objects/fileobject.c` | 14/17 | yes |
| `Modules/newmodule.c` | 13/13 | |
| `Objects/floatobject.c` | 13/13 | yes |
| `Objects/longobject.c` | 12/12 | yes |
| `Objects/bufferobject.c` | 11/11 | yes |
| `Objects/object.c` | 11/20 | yes |
| `Modules/pypcre.c` | 10/29 | yes |
| `Python/codecs.c` | 10/10 | |
| `Python/pystate.c` | 9/9 | yes |
| `Python/marshal.c` | 8/9 | yes |
| `Modules/_sre.c` | 7/10 | yes |
| `Modules/regexmodule.c` | 7/7 | yes |
| `Parser/tokenizer.c` | 7/7 | yes |
| `Objects/funcobject.c` | 6/7 | yes |
| `Python/modsupport.c` | 6/6 | |
| `Objects/cobject.c` | 5/5 | yes |
| `Objects/rangeobject.c` | 5/6 | yes |
| `Objects/methodobject.c` | 4/4 | yes |
| `Objects/moduleobject.c` | 4/4 | yes |
| `Parser/acceler.c` | 4/5 | |
| `Python/errors.c` | 4/5 | |
| `Modules/posixmodule.c` | 3/143 | |
| `Modules/signalmodule.c` | 3/5 | |
| `Objects/frameobject.c` | 3/4 | yes |
| `Objects/tupleobject.c` | 3/3 | yes |
| `Python/structmember.c` | 3/4 | |
| `Parser/parsetok.c` | 2/2 | yes |
| `Python/graminit.c` | 2/2 | |
| `Python/traceback.c` | 2/3 | yes |
| `Objects/dictobject.c` | 1/1 | yes |
| `Objects/typeobject.c` | 1/1 | |
| `Parser/myreadline.c` | 1/1 | |
| `Parser/parser.c` | 1/7 | yes |
| `Python/frozen.c` | 1/1 | |
| `Objects/sliceobject.c` | none | yes |
| `Parser/node.c` | none | yes |
| `PC/getpathp.c` | | yes |
| `PC/config.c` | | inferred, see above |
| `PC/config.c` | | modified, reconstructed here |
| `PC/config.h` | | modified, allocator hooks reconstructed here |
| `Lib/os.py` | | modified script, `ps2` branch at line 92 |
| `Modules/posixmodule.c` | | trimmed to twelve methods and renamed `ps2`, see above |

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

The `abort` method accounts for one of the three surviving literals. The other two, `unrecognized
configuration name` and `configuration names must be strings or integers`, remain unexplained:
`confstr`, `sysconf`, `fpathconf`, and `pathconf` are all absent from the method table, so the
messages survive without a caller that can produce them.

## Outstanding

The PyCXX release is deliberately deferred until the core is verified, because its version marker
is likely a header comment that does not survive compilation.

`ceval.c` and `pythonrun.c` get no patch, and the reason is not size. Nine of `ceval.c`'s ten
absences are macros the port does not define, so a patch would assert an edit that never happened.
The acceptance test cannot tell the difference, which means it would pass and still be false, so a
passing patch is necessary evidence rather than sufficient evidence.
