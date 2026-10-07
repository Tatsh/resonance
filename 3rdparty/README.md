# Vendored sources

Each directory here is an upstream release, imported with its archive's top directory name. The
port's changes are made in place and are listed below. The formatting and checking tools skip this
directory, and upstream code retains its original style.

| Directory     | Upstream archive                                                 | SHA-256 of the archive                                             | Files imported           |
| ------------- | ---------------------------------------------------------------- | ------------------------------------------------------------------ | ------------------------ |
| `Python-2.0/` | <https://www.python.org/ftp/python/2.0/BeOpen-Python-2.0.tar.gz> | `63bbcb1a69cd1673b92068247ba0fba19a2b3ddcb7b73bc9391fa148b3d4e652` | All 2043 files           |
| `gzip-1.2.4/` | <https://ftp.gnu.org/gnu/gzip/gzip-1.2.4.tar.gz>                 | `1ca41818a23c9c59ef1d5e1d00c0d5eaa2285d931c0fb059637d7c0cc02ad967` | 5 of the release's files |

## Python 2.0

The tree is the BeOpen release of CPython 2.0. The repository stores every file with LF line
endings. The 42 files the release ships with CRLF line endings (under `PC/`, `PCbuild/`, and
`Tools/scripts/`) differ from the archive only in their line endings.

The port changes 55 files. [src/python/MANIFEST.md](../src/python/MANIFEST.md) records each change
and the evidence for it in the image. These are the changes:

- **`Py_LONG`.** Every header and compiled file writes the C `long` as `Py_LONG`, a 64-bit type
  with the port's 32-bit limits. The `%ld` formats and the `long` conversions follow it.
- **Imports.** `Python/import.c` compiles every source module without reading or writing a compiled
  module, and `find_module` skips the `DLLs` path prefixes.
- **Running code.** `Python/pythonrun.c` refuses compiled files in `PyRun_SimpleFileEx`, and
  `PyRun_SimpleString` writes its result to `sys.stdout`.
- **The `ps2` module.** `Modules/posixmodule.c` registers as `ps2`, retains the 12 methods the
  image has, returns an empty list from `listdir`, and runs the Windows path checks in `posix_do_stat`.
- **`sys.version`.** `Python/sysmodule.c` reports the image's build date, time, and compiler.
- **Complex values.** `Python/bltinmodule.c`, `Python/compile.c`, `Python/getargs.c`, and
  `Python/marshal.c` build each `Py_complex` through field stores.
- **Warnings.** `Include/stringobject.h` and `Include/unicodeobject.h` drop the `register` storage
  class from nine parameters. `Modules/cPickle.c`, `Modules/pypcre.c`, `Modules/regexpr.c`,
  `Objects/unicodeobject.c`, and `Python/ceval.c` gain fall-through comments. These two changes do
  not change the generated code.

## gzip 1.2.4

Only `COPYING`, `README`, `gzip.h`, `inflate.c`, and `tailor.h` are imported. The port changes one
file. `inflate.c` calls `HuftReset()` before each block to rewind the table pool.
[src/os/INFLATE.md](../src/os/INFLATE.md) records the change and the version evidence.

## Verification

Download an archive, check its SHA-256, extract it, and compare each imported file with line endings
removed. For example, `cmp <(tr -d '\r' < Python-2.0/Python/ceval.c) <(tr -d '\r' < 3rdparty/Python-2.0/Python/ceval.c)`
reports the first difference in that file.
