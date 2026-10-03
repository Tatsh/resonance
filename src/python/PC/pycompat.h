#ifndef PYCOMPAT_H
#define PYCOMPAT_H

/* Build glue for the vendored interpreter, which stays unmodified. Upstream expects the C
   library headers, the module search default, and the full socket vocabulary from its own
   configuration, none of which the port carries, so this header supplies them. It enters every
   interpreter translation unit through the forced include in src/python/CMakeLists.txt and
   nowhere else. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "os/log.h"

/* The port's failed assertion prints a fixed line and aborts, without the expression, file, or
   line. getarrayitem's check, inlined into array_tolist at 0x006564a0, calls printf (0x0053dde0)
   with "Assertion failed\n" and then abort (0x005e64c8). The C library's assert macro is
   redefined at every inclusion of its header, but it identifies its failure routine at each use.
   The routine is therefore redirected instead of the macro. */
#ifndef __cplusplus
#define __assert_func py_assert_failed
static void py_assert_failed(const char *file, int line, const char *function,
                             const char *expression) __attribute__((noreturn, unused));
static void py_assert_failed(const char *file, int line, const char *function,
                             const char *expression) {
    (void)file;
    (void)line;
    (void)function;
    (void)expression;
    printf("Assertion failed\n");
    abort();
}
#endif

/* The port's module search default is empty (0x0082c9f0). A dot entry would arrive at the archive
   lookup as a `./` path, and the lookup treats a `./` path as fatal. */
#define PYTHONPATH ""

/* The port routed the C library allocator to the interpreter heap in every interpreter file, with
   the call site's file and line. The bare calls in cStringIO.c (Heap::Realloc at 0x00656ce8),
   regexmodule.c (Heap::Free at 0x00652090), regexpr.c (Heap::Alloc in re_compile_pattern at
   0x00662034), getpathp.c (Heap::Alloc at 0x0056a278), and _sre.c (the mark stack at 0x00640954)
   all include their file's tag. The library prototypes above are already declared, and C++
   includers retain the plain functions. */
#ifndef __cplusplus
#define malloc(n) PyCore_MALLOC(n)
#define realloc(p, n) PyCore_REALLOC((p), (n))
#define free(p) PyCore_FREE(p)
#endif

/* The port's three-way double compare with its operands in source order, from the runtime. The
   interpreter's comparisons of a constant against a double call it where the original result for an
   unordered operand would otherwise be lost (see the runtime's soft-float file). */
int freq_compare_double(double a, double b);

/* The Windows headers supply this for the path module upstream. */
#define min(a, b) ((a) < (b) ? (a) : (b))

/* The toolchain declares its own close-with-flag as posix_close, which collides with the
   module method of the same name. The method is static and only its address in the method
   table matters, so it builds under a distinct name. */
#define posix_close py_posix_close_method

/* Socket types newlib does not name. The values follow the Linux ABI and are unverified against
   the image, whose socket tables the reconstruction has not recovered. */
#ifndef SOCK_SEQPACKET
#define SOCK_SEQPACKET 5
#endif
#ifndef SOCK_RDM
#define SOCK_RDM 4
#endif

/* Name service declarations newlib leaves out. Only the unlinked socket module needs them, so
   their shapes follow the BSD standard and their bodies never have to exist. */
struct servent {
    char *s_name;
    char **s_aliases;
    int s_port;
    char *s_proto;
};
struct protoent {
    char *p_name;
    char **p_aliases;
    int p_proto;
};
struct servent *getservbyname(const char *name, const char *proto);
struct protoent *getprotobyname(const char *name);
int gethostname(char *name, size_t len);
FILE *fdopen(int fd, const char *mode);

#endif
