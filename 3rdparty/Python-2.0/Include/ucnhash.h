
#include "Python.h"
#include <stdlib.h>

/* --- C API ----------------------------------------------------*/
/* C API for usage by other Python modules */
typedef struct _Py_UCNHashAPI
{
    unsigned Py_LONG cKeys;
    unsigned Py_LONG cchMax;
    unsigned Py_LONG (*hash)(const char *key, unsigned int cch);
    const void *(*getValue)(unsigned Py_LONG iKey);
} _Py_UCNHashAPI;

typedef struct 
{
    const char *pszUCN;
    Py_UCS4 value;
} _Py_UnicodeCharacterName;

