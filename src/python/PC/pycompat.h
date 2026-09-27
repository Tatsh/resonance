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

/* Module search default the port never recorded. The leading dot matches every upstream
   variant and keeps the buffer sizing honest. */
#define PYTHONPATH "."

/* The Windows headers supply this for the path module upstream. */
#define min(a, b) ((a) < (b) ? (a) : (b))

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
