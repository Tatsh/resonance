#ifndef Py_CONFIG_H
#define Py_CONFIG_H

/* Port note. Only the part of PC/config.h that the port changed is recorded here, which is the
   allocator redirect. The rest of the upstream header is unrecovered, because a compiler setting
   leaves no trace in the image unless it changes emitted code.

   Objects/obmalloc.c does not exist in 2.0; it arrived in 2.1. So the allocator change has no file
   of its own and instead uses the override hooks Include/pymem.h and Include/objimpl.h already
   provide, both of which guard their defaults with #ifndef and are documented for exactly this.
   Defining the macros here, ahead of either header, replaces malloc without editing upstream. Both
   of those headers are therefore unmodified.

   The size argument comes from the macro's own parameter while the file and line come from the
   call site, which upstream's macros do not pass. That is what makes every allocating translation
   unit store its own basename in .rodata, and those basenames are the manifest's strongest
   evidence for which files are compiled in.

   The object variants have to be overridden as well as the raw ones. Upstream defaults
   PyCore_OBJECT_MALLOC_FUNC to PyCore_MALLOC_FUNC rather than to the PyCore_MALLOC macro, so
   overriding only the raw macro would leave every object allocation going to malloc. The tags prove
   it does not: tupleobject.c and unicodeobject.c both appear and both allocate through the object
   interface. */

/* Macros this port does NOT define, which is where most of the trim actually lives. Each one
   compiles out an upstream block, so the affected files are byte-identical upstream and need no
   patch. The evidence for each is the absence of its text from the image, cross-checked by
   .wiswa-ci/freq/py_verify_patch.py.

     WITH_THREAD             the import lock, and ceval's PyEval_AcquireThread and
                             PyEval_ReleaseThread interface. threadmodule.c and thread.c are both
                             absent, so there is no global interpreter lock in this build. The
                             single-threaded PyThreadState bookkeeping in pystate.c is untouched.
     HAVE_DYNAMIC_LOADING    imp.load_dynamic. Every dynload_*.c is absent.
     CHECK_IMPORT_CASE       the case-mismatch and find-file checks in import.c.
     USE_STACKCHECK          ceval's stack-overflow check.
     CHECKEXC               ceval's undetected-error assertions.
     Py_TRACE_REFS           PYTHONDUMPREFS and the reference-count dump.
     macintosh, MS_WIN32     imp.load_resource and the Windows and Macintosh path branches.
     SIZEOF_TIME_T > 4       the timestamp-overflow check, because time_t is four bytes here.

   posixmodule.c is trimmed the same way rather than by editing. Its confstr, sysconf and pathconf
   tables guard every entry with that entry's own constant, and the console defines almost none of
   them, which is why 140 of its 143 literals are absent while the module itself is compiled in. */

/* The complex type is compiled out. The builtin method table at 0x0076c390 runs from compile
   straight to delattr, and none of complexobject.c's literals are in the image. The shipped
   types.py therefore takes its NameError branch. */
#define WITHOUT_COMPLEX

/* fileobject.c sizes reads without fstat. file_read at 0x005f5138 inlines new_buffersize as the
   chunk arithmetic alone, and the file's code does not call fstat, lseek, or ftell. */
#define DONT_HAVE_FSTAT

/* The port's build stamp. Py_GetBuildInfo at 0x006371f0 formats build 0 with "Oct 12 2001"
   (0x00841af0) and "12:05:12" (0x00841b00) for sys.version. */
#define DATE "Oct 12 2001"
#define TIME "12:05:12"

/* The port's compiler stamp. Py_GetCompiler at 0x0063f1d0 returns "\n[GCC 2.95.2 v2]"
   (0x00842480). Py_GetVersion places the string in sys.version. */
#define COMPILER "\n[GCC 2.95.2 v2]"

/* The port's compiler gave the C long 64 bits. The image shows the width in PyInt_AsLong at
   0x00581c00 (an ld of ob_ival) and int_add at 0x00581d98 (a daddu with a 64-bit overflow test).
   The n32 toolchain retains a 32-bit long and rejects -mlong64. The interpreter therefore writes
   its long as Py_LONG, and the width that upstream reads from the host follows Py_LONG. int_lshift
   at 0x00581f38 compares the shift count against 64. The limits are 32-bit nonetheless.
   PyInt_GetMax at 0x00581cc8 returns 0x7fffffff, and sysmodule.c stores the result as maxint. The
   port's C library header retained the 32-bit values. */
#define Py_LONG long long
#define SIZEOF_LONG 8
#define PY_LONG_BIT 64
#define PY_LONG_MAX 0x7fffffffLL
#define PY_LONG_MIN (-PY_LONG_MAX - 1)
#define PY_ULONG_MAX 0xffffffffULL

/* Upstream's PC/config.h defines the 64-bit integer type and its width, and the port's
   configuration descends from it. PyLong_AsVoidPtr at 0x00577718 inlines PyLong_AsLong, sign test
   included. PyLong_AsLongLong forwards to PyLong_AsLong only when the two widths match. */
#define HAVE_LONG_LONG 1
#define LONG_LONG long long
#define SIZEOF_LONG_LONG 8

#ifdef __cplusplus
#include "os/heap.h"

/* The one interpreter heap, built by Py_Initialize over the whole of the zone titled python. */
extern Heap *g_pPythonHeap;

/* The C translation units of the interpreter cannot call methods, so the redirect goes through
   these functions, which forward to the heap. Their names are build scaffolding rather than
   recovered titles, but the file and line tagging they carry is the observed behaviour. The C++
   declarations beside them keep the one definition in heap.cpp visible to every includer. */
extern "C" {
#endif
void PyHeap_Init(void);
void *PyHeap_Alloc(unsigned nSize, const char *pszFile, int nLine);
void *PyHeap_Realloc(void *pBlock, unsigned nSize, const char *pszFile, int nLine);
void PyHeap_Free(void *pBlock, const char *pszFile, int nLine);
#ifdef __cplusplus
}
#endif

#define PyCore_MALLOC(n) PyHeap_Alloc((n), __FILE__, __LINE__)
#define PyCore_REALLOC(p, n) PyHeap_Realloc((p), (n), __FILE__, __LINE__)
#define PyCore_FREE(p) PyHeap_Free((p), __FILE__, __LINE__)

#define PyCore_OBJECT_MALLOC(n) PyCore_MALLOC(n)
#define PyCore_OBJECT_REALLOC(p, n) PyCore_REALLOC((p), (n))
#define PyCore_OBJECT_FREE(p) PyCore_FREE(p)

#endif /* !Py_CONFIG_H */
