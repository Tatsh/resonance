#include <stddef.h>
#include <string.h>

#include <sifdev.h>
#include <sifrpc.h>

// The client of the IOP module loader server. Every call binds the server on first use and checks
// the version it reported.

enum {
    kLoadFileServerId = 0x80000006,
};

enum {
    kLoadFileFunctionModule = 0,
    kLoadFileFunctionElf = 1,
    kLoadFileFunctionSetValue = 2,
    kLoadFileFunctionGetValue = 3,
    kLoadFileFunctionBuffer = 6,
    kLoadFileFunctionStop = 7,
    kLoadFileFunctionUnload = 8,
    kLoadFileFunctionSearchName = 9,
    kLoadFileFunctionSearchAddress = 10,
    kLoadFileFunctionVersion = 255,

    kLoadPathSize = 252,
    kLoadArgSize = 252,
    kVersionSize = 4,
    kWordSize = 4,
    kResultSize = 8,
    kElfResultSize = 16,
    kValueRequestSize = 32,
    kSetValueResultSize = 16,
    kBindRetryDelay = 0x100000,

    kLoadFileErrorBind = -0x10000,
    kLoadFileErrorRpc = -0x10001,
    kLoadFileErrorType = -0x10002,
    kLoadFileErrorNoExec = -0x10003,
    kLoadFileErrorVersion = -0x10004,
};

// The argument and reply block every call shares. The server writes its reply over the start of
// the block.
typedef struct {
    union {
        int nArgLength;
        const void *pImage;
        int nModuleId;
        const void *pIopAddr;
        unsigned int nIopAddr;
        int nResult;
        unsigned int nEpc;
        unsigned char nResultByte;
        unsigned short nResultHalf;
        unsigned int nResultWord;
    };
    union {
        int nBufferArgLength;
        int nValueType;
        int nModuleResult;
        unsigned int nGp;
    };
    union {
        char szPath[kLoadPathSize];
        unsigned char nValueByte;
        unsigned short nValueHalf;
        unsigned int nValueWord;
    };
    union {
        char abArgs[kLoadArgSize];
        char szSection[kLoadArgSize];
    };
} LoadFileArgs;

// NTSC-U/C: 0x0071ea7c, PAL: 0x007624fc, the server version the library was built for.
static const char g_abLoadFileLibraryVersion[kVersionSize] = {'2', '3', '0', '0'};

// NTSC-U/C: 0x00780dc8, PAL: 0x007c4ae0, negative until the client is bound.
static int g_nLoadFileBound = -1;

// NTSC-U/C: 0x00780dcc, PAL: 0x007c4ae4, a second server version the library accepts.
static const char *g_pszLoadFileAltVersion = "....";

// NTSC-U/C: 0x008e5c80, PAL: 0x0092ac80. SIF DMA needs the 64-byte alignment retail gives it.
static LoadFileArgs g_loadFileArgs __attribute__((aligned(64)));

// NTSC-U/C: 0x008e5e80, PAL: 0x0092ae80
static sceSifClientData g_loadFileClient __attribute__((aligned(64)));

// NTSC-U/C: 0x008e5ea8, PAL: 0x0092aea8, the version the server reported when the client bound it.
static char g_abLoadFileServerVersion[kVersionSize];

// Copy the module arguments into the block, at most kLoadArgSize bytes of them, and return the
// count copied.
static inline int copyModuleArgs(char *pDest, int nArgs, const char *pArgs) {
    if (nArgs <= kLoadArgSize) {
        memcpy(pDest, pArgs, (size_t)nArgs);
        return nArgs;
    }
    memcpy(pDest, pArgs, kLoadArgSize);
    return kLoadArgSize;
}

// NTSC-U/C: 0x005fb238, PAL: 0x0063bf48
static int _lf_bind(void) {
    int nDelay;

    if (g_nLoadFileBound >= 0) {
        return 0;
    }
    for (;;) {
        if (sceSifBindRpc(&g_loadFileClient, kLoadFileServerId, 0) < 0) {
            return -1;
        }
        if (g_loadFileClient.serve != NULL) {
            break;
        }
        for (nDelay = kBindRetryDelay; nDelay != -1; --nDelay) {
            __asm__ volatile("nop");
        }
    }
    g_nLoadFileBound = 0;
    if (sceSifCallRpc(&g_loadFileClient,
                      kLoadFileFunctionVersion,
                      0,
                      NULL,
                      0,
                      &g_loadFileArgs,
                      kWordSize,
                      NULL,
                      NULL) < 0) {
        return kLoadFileErrorRpc;
    }
    memcpy(g_abLoadFileServerVersion, &g_loadFileArgs, kVersionSize);
    return 0;
}

// NTSC-U/C: 0x005fb338, PAL: 0x0063c048
// Reports a mismatch only when the server matches neither accepted version and the two accepted
// versions also differ from each other.
static int loadFileVersionMismatch(void) {
    if (memcmp(g_abLoadFileServerVersion, g_abLoadFileLibraryVersion, kVersionSize) == 0) {
        return 0;
    }
    if (memcmp(g_abLoadFileServerVersion, g_pszLoadFileAltVersion, kVersionSize) == 0) {
        return 0;
    }
    return memcmp(g_abLoadFileLibraryVersion, g_pszLoadFileAltVersion, kVersionSize) != 0;
}

// NTSC-U/C: 0x005fb3c8, PAL: 0x0063c0d8
int sceSifLoadFileReset(void) {
    g_nLoadFileBound = -1;
    memset(g_abLoadFileServerVersion, 0, kVersionSize);
    return 0;
}

// NTSC-U/C: 0x005fb400, PAL: 0x0063c110
static int loadModuleBuffer(const void *pImage, int nArgs, const char *pArgs, int *pResult) {
    if (_lf_bind() < 0) {
        return kLoadFileErrorBind;
    }
    if (loadFileVersionMismatch() != 0) {
        return kLoadFileErrorVersion;
    }
    g_loadFileArgs.pImage = pImage;
    if (pArgs != NULL) {
        g_loadFileArgs.nBufferArgLength = copyModuleArgs(g_loadFileArgs.abArgs, nArgs, pArgs);
    } else {
        g_loadFileArgs.nBufferArgLength = 0;
    }
    if (sceSifCallRpc(&g_loadFileClient,
                      kLoadFileFunctionBuffer,
                      0,
                      &g_loadFileArgs,
                      sizeof(g_loadFileArgs),
                      &g_loadFileArgs,
                      kResultSize,
                      NULL,
                      NULL) < 0) {
        return kLoadFileErrorRpc;
    }
    *pResult = g_loadFileArgs.nModuleResult;
    return g_loadFileArgs.nResult;
}

// NTSC-U/C: 0x005fb608, PAL: 0x0063c318
int sceSifStopModule(int modid, int args, const char *argp, int *result) {
    if (_lf_bind() < 0) {
        return kLoadFileErrorBind;
    }
    if (loadFileVersionMismatch() != 0) {
        return kLoadFileErrorVersion;
    }
    g_loadFileArgs.nModuleId = modid;
    if (argp != NULL) {
        g_loadFileArgs.nBufferArgLength = copyModuleArgs(g_loadFileArgs.abArgs, args, argp);
    } else {
        g_loadFileArgs.nBufferArgLength = 0;
    }
    if (sceSifCallRpc(&g_loadFileClient,
                      kLoadFileFunctionStop,
                      0,
                      &g_loadFileArgs,
                      sizeof(g_loadFileArgs),
                      &g_loadFileArgs,
                      kResultSize,
                      NULL,
                      NULL) < 0) {
        return kLoadFileErrorRpc;
    }
    *result = g_loadFileArgs.nModuleResult;
    return g_loadFileArgs.nResult;
}

// NTSC-U/C: 0x005fb810, PAL: 0x0063c520
int sceSifUnloadModule(int modid) {
    if (_lf_bind() < 0) {
        return kLoadFileErrorBind;
    }
    if (loadFileVersionMismatch() != 0) {
        return kLoadFileErrorVersion;
    }
    g_loadFileArgs.nModuleId = modid;
    if (sceSifCallRpc(&g_loadFileClient,
                      kLoadFileFunctionUnload,
                      0,
                      &g_loadFileArgs,
                      kWordSize,
                      &g_loadFileArgs,
                      kWordSize,
                      NULL,
                      NULL) < 0) {
        return kLoadFileErrorRpc;
    }
    return g_loadFileArgs.nResult;
}

// NTSC-U/C: 0x005fb8a0, PAL: 0x0063c5b0
int sceSifSearchModuleByName(const char *modulename) {
    if (_lf_bind() < 0) {
        return kLoadFileErrorBind;
    }
    if (loadFileVersionMismatch() != 0) {
        return kLoadFileErrorVersion;
    }
    strncpy(g_loadFileArgs.szPath, modulename, kLoadPathSize);
    g_loadFileArgs.szPath[kLoadPathSize - 1] = '\0';
    if (sceSifCallRpc(&g_loadFileClient,
                      kLoadFileFunctionSearchName,
                      0,
                      &g_loadFileArgs,
                      sizeof(g_loadFileArgs),
                      &g_loadFileArgs,
                      kWordSize,
                      NULL,
                      NULL) < 0) {
        return kLoadFileErrorRpc;
    }
    return g_loadFileArgs.nResult;
}

// NTSC-U/C: 0x005fb940, PAL: 0x0063c650
int sceSifSearchModuleByAddress(const void *addr) {
    if (_lf_bind() < 0) {
        return kLoadFileErrorBind;
    }
    if (loadFileVersionMismatch() != 0) {
        return kLoadFileErrorVersion;
    }
    g_loadFileArgs.pIopAddr = addr;
    if (sceSifCallRpc(&g_loadFileClient,
                      kLoadFileFunctionSearchAddress,
                      0,
                      &g_loadFileArgs,
                      kWordSize,
                      &g_loadFileArgs,
                      kWordSize,
                      NULL,
                      NULL) < 0) {
        return kLoadFileErrorRpc;
    }
    return g_loadFileArgs.nResult;
}

// NTSC-U/C: 0x005fb9d0, PAL: 0x0063c6e0
int sceSifLoadModuleBuffer(const void *addr, int args, const char *argp) {
    int nResult;

    return loadModuleBuffer(addr, args, argp, &nResult);
}

// NTSC-U/C: 0x005fb9f0, PAL: 0x0063c700
int sceSifLoadStartModuleBuffer(const void *addr, int args, const char *argp, int *result) {
    return loadModuleBuffer(addr, args, argp, result);
}

// NTSC-U/C: 0x005fba10, PAL: 0x0063c720
static int
loadModule(const char *pPath, int nArgs, const char *pArgs, int *pResult, int nFunction) {
    if (_lf_bind() < 0) {
        return kLoadFileErrorBind;
    }
    if (loadFileVersionMismatch() != 0) {
        return kLoadFileErrorVersion;
    }
    strncpy(g_loadFileArgs.szPath, pPath, kLoadPathSize);
    g_loadFileArgs.szPath[kLoadPathSize - 1] = '\0';
    if (pArgs != NULL) {
        g_loadFileArgs.nArgLength = copyModuleArgs(g_loadFileArgs.abArgs, nArgs, pArgs);
    } else {
        g_loadFileArgs.abArgs[0] = '\0';
        g_loadFileArgs.nArgLength = 0;
    }
    if (sceSifCallRpc(&g_loadFileClient,
                      (unsigned int)nFunction,
                      0,
                      &g_loadFileArgs,
                      sizeof(g_loadFileArgs),
                      &g_loadFileArgs,
                      kResultSize,
                      NULL,
                      NULL) < 0) {
        return kLoadFileErrorRpc;
    }
    *pResult = g_loadFileArgs.nModuleResult;
    return g_loadFileArgs.nResult;
}

// NTSC-U/C: 0x005fbc38, PAL: 0x0063c948
int sceSifLoadModule(const char *filename, int args, const char *argp) {
    int nResult;

    return loadModule(filename, args, argp, &nResult, kLoadFileFunctionModule);
}

// NTSC-U/C: 0x005fbc58, PAL: 0x0063c968
int sceSifLoadStartModule(const char *filename, int args, const char *argp, int *result) {
    return loadModule(filename, args, argp, result, kLoadFileFunctionModule);
}

// NTSC-U/C: 0x005fbc78, PAL: 0x0063c988
static int loadElf(const char *pPath, const char *pSection, sceExecData *pData, int nFunction) {
    if (_lf_bind() < 0) {
        return kLoadFileErrorBind;
    }
    if (loadFileVersionMismatch() != 0) {
        return kLoadFileErrorVersion;
    }
    strncpy(g_loadFileArgs.szPath, pPath, kLoadPathSize);
    g_loadFileArgs.szPath[kLoadPathSize - 1] = '\0';
    strncpy(g_loadFileArgs.szSection, pSection, kLoadArgSize);
    g_loadFileArgs.szSection[kLoadArgSize - 1] = '\0';
    if (sceSifCallRpc(&g_loadFileClient,
                      (unsigned int)nFunction,
                      0,
                      &g_loadFileArgs,
                      sizeof(g_loadFileArgs),
                      &g_loadFileArgs,
                      kElfResultSize,
                      NULL,
                      NULL) < 0) {
        return kLoadFileErrorRpc;
    }
    if (g_loadFileArgs.nEpc == 0) {
        return kLoadFileErrorNoExec;
    }
    pData->epc = g_loadFileArgs.nEpc;
    pData->gp = g_loadFileArgs.nGp;
    return 0;
}

// NTSC-U/C: 0x005fbd80, PAL: 0x0063ca90
int sceSifLoadElfPart(const char *name, const char *secname, sceExecData *data) {
    return loadElf(name, secname, data, kLoadFileFunctionElf);
}

// NTSC-U/C: 0x005fbda0, PAL: 0x0063cab0
int sceSifLoadElf(const char *name, sceExecData *data) {
    return loadElf(name, "all", data, kLoadFileFunctionElf);
}

// NTSC-U/C: 0x005fbdc8, PAL: 0x0063cad8
// Unlike the other calls, the value calls do not check the server version.
int sceSifGetIopAddr(unsigned int addr, void *value, int type) {
    if (_lf_bind() < 0) {
        return kLoadFileErrorBind;
    }
    if ((unsigned int)type > SIF_IOP_VALUE_WORD) {
        return kLoadFileErrorType;
    }
    g_loadFileArgs.nIopAddr = addr;
    g_loadFileArgs.nValueType = type;
    if (sceSifCallRpc(&g_loadFileClient,
                      kLoadFileFunctionGetValue,
                      0,
                      &g_loadFileArgs,
                      kValueRequestSize,
                      &g_loadFileArgs,
                      kValueRequestSize,
                      NULL,
                      NULL) < 0) {
        return kLoadFileErrorRpc;
    }
    switch (type) {
    case SIF_IOP_VALUE_BYTE:
        *(unsigned char *)value = g_loadFileArgs.nResultByte;
        break;
    case SIF_IOP_VALUE_HALF:
        *(unsigned short *)value = g_loadFileArgs.nResultHalf;
        break;
    case SIF_IOP_VALUE_WORD:
        *(unsigned int *)value = g_loadFileArgs.nResultWord;
        break;
    default:
        return kLoadFileErrorType;
    }
    return 0;
}

// NTSC-U/C: 0x005fbeb8, PAL: 0x0063cbc8
int sceSifSetIopAddr(unsigned int addr, const void *value, int type) {
    if (_lf_bind() < 0) {
        return kLoadFileErrorBind;
    }
    g_loadFileArgs.nIopAddr = addr;
    g_loadFileArgs.nValueType = type;
    switch (type) {
    case SIF_IOP_VALUE_BYTE:
        g_loadFileArgs.nValueByte = *(const unsigned char *)value;
        break;
    case SIF_IOP_VALUE_HALF:
        g_loadFileArgs.nValueHalf = *(const unsigned short *)value;
        break;
    case SIF_IOP_VALUE_WORD:
        g_loadFileArgs.nValueWord = *(const unsigned int *)value;
        break;
    default:
        return kLoadFileErrorType;
    }
    if (sceSifCallRpc(&g_loadFileClient,
                      kLoadFileFunctionSetValue,
                      0,
                      &g_loadFileArgs,
                      kValueRequestSize,
                      &g_loadFileArgs,
                      kSetValueResultSize,
                      NULL,
                      NULL) < 0) {
        return kLoadFileErrorRpc;
    }
    return 0;
}
