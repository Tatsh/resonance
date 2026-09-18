/* Module configuration */

/* This file contains the table of built-in modules.
   See init_builtin() in import.c. */

/* Port note. This is PC/config.c with the module list replaced. The upstream PC list registers
   the Windows set, "nt" among it, and neither "nt" nor "posix" appears anywhere in the image while
   "ps2" appears five times, so posixmodule.c is compiled in under a console name of its own. The
   nineteen entries and their order come from the _PyImport_Inittab name pool at 0x00741380.

   "hx" and "ucnhash" are absent here on purpose. Both are registered from outside this table,
   "hx" being the game's own extension module. */

#include "Python.h"

extern void initerrno(void);
extern void init_sre(void);
extern void initimp(void);
extern void init_codecs(void);
extern void initpcre(void);
extern void initcPickle(void);
extern void initcStringIO(void);
extern void initstruct(void);
extern void initstrop(void);
extern void initsignal(void);
extern void initregex(void);
extern void initps2(void);
extern void initnew(void);
extern void initarray(void);

struct _inittab _PyImport_Inittab[] = {

        {"errno", initerrno},
        {"_sre", init_sre},

        /* These entries are here for sys.builtin_module_names */
        {"exceptions", NULL},
        {"sys", NULL},
        {"__builtin__", NULL},
        {"__main__", NULL},

        /* This lives it with import.c */
        {"imp", initimp},

        /* This module "lives in" with marshal.c */
        {"marshal", PyMarshal_Init},

        {"_codecs", init_codecs},
        {"pcre", initpcre},
        {"cPickle", initcPickle},
        {"cStringIO", initcStringIO},
        {"struct", initstruct},
        {"strop", initstrop},
        {"signal", initsignal},
        {"regex", initregex},

        /* posixmodule.c, renamed for the console. The shipped os.py tests for this name. */
        {"ps2", initps2},

        {"new", initnew},
        {"array", initarray},

        /* Sentinel */
        {0, 0}
};
