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

#include "os/heap.h"

/* The one interpreter heap, built by Py_Initialize over the whole of the zone titled python. */
extern Heap *g_pPythonHeap;

#define PyCore_MALLOC(n) g_pPythonHeap->Alloc((n), __FILE__, __LINE__)
#define PyCore_REALLOC(p, n) g_pPythonHeap->Realloc((p), (n), __FILE__, __LINE__)
#define PyCore_FREE(p) g_pPythonHeap->Free((p), __FILE__, __LINE__)

#define PyCore_OBJECT_MALLOC(n) PyCore_MALLOC(n)
#define PyCore_OBJECT_REALLOC(p, n) PyCore_REALLOC((p), (n))
#define PyCore_OBJECT_FREE(p) PyCore_FREE(p)

#endif /* !Py_CONFIG_H */
