#include "os/arkfile.h"

#include <string.h>
#include <vector>

#include "os/loadfile.h"
#include "os/mem.h"
#include "os/seccache.h"

ArkFile::~ArkFile() {
    if (mTables != nullptr) {
        MemFreeTagged(mTables, __FILE__, __LINE__);
    }
    if (mOptimizedTable != nullptr) {
        MemFreeTagged(mOptimizedTable, __FILE__, __LINE__);
    }
}

int ArkFile::Close(const char *pszPath) {
    unsigned nArk = 0;
    while (nArk < g_apMountedArks.size()) {
        const char *pszMounted = g_apMountedArks[nArk]->mPath.mStr;
        if (pszMounted == nullptr) {
            pszMounted = g_pszEmpty;
        }
        if (strcmp(pszMounted, pszPath) == 0) {
            break;
        }
        ++nArk;
    }
    if (nArk == g_apMountedArks.size()) {
        return 0;
    }

    // A stream still open on the archive's file blocks the unmount. The offending
    // record is erased before the refusal, which is what allows a later attempt
    // to succeed.
    for (unsigned i = 0; i < g_aArkStreams.size(); ++i) {
        if (g_aArkStreams[i].mFile == g_apMountedArks[nArk]->mFile) {
            EraseArkStream(g_aArkStreams[i].mHandle);
            return 0;
        }
    }

    CloseLoadFile(g_apMountedArks[nArk]->mFile);
    InvalidateCachedSectors(g_apMountedArks[nArk]->mFile);
    delete g_apMountedArks[nArk];
    g_apMountedArks.erase(g_apMountedArks.begin() + nArk);
    return 1;
}

int EraseArkStream(int nHandle) {
    for (unsigned i = 0; i < g_aArkStreams.size(); ++i) {
        if (g_aArkStreams[i].mHandle == nHandle) {
            g_aArkStreams.erase(g_aArkStreams.begin() + i);
            return 0;
        }
    }
    return -1;
}
