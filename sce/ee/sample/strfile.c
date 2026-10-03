#include "ezmpeg/strfile.h"

#include <ctype.h>
#include <ezmpeg.h>
#include <libcdvd.h>
#include <sifdev.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "os/log.h"

// Room for a rebuilt path and for the device prefix before the colon.
enum {
    kPathSize = 256,
    kDeviceSize = 16,
    // The flag sceOpen() receives, which opens for reading only.
    kOpenReadOnly = 1,
    // The streaming parameters the compact-disc path passes to sceCdStInit().
    kStreamSectors = 0x50,
    kStreamBanks = 5,
    // Eighty sectors of 2048 bytes with sixteen bytes of slack for alignment.
    kIopHeapSize = 0x28010,
    // Offset within StrFile::cdFile of the stored IOP heap pointer (+0x2c).
    kHeapSlot = 0x24,
    // A sector holds 2048 bytes, so sector counts shift by eleven bits.
    kSectorShift = 11,
};

// Records the first compact-disc open, as the shipped program does. Nothing else reads it.
static int g_bCdInitialized;

// NTSC-U/C: 0x0058de70, PAL: 0x005d11c8
int strFileOpen(StrFile *pFile, const char *pszName) {
    char szPath[kPathSize];
    char szDev[kDeviceSize];

    const char *pColon = strchr(pszName, ':');
    if (pColon != NULL) {
        size_t nPrefix = (size_t)(pColon - pszName);
        strncpy(szDev, pszName, nPrefix);
        szDev[nPrefix] = '\0';
        const char *pRest = pColon + 1;
        if (strcmp(szDev, "cdrom0") != 0) {
            pFile->isOnCD = 0;
            sprintf(szPath, "%s%s", szDev, pRest);
        } else {
            pFile->isOnCD = 1;
            // Normalise the caller buffer in place, exactly as the shipped program does.
            // Forward slashes become backslashes and every character is uppercased.
            char *pWritable = (char *)pRest;
            int nLength = (int)strlen(pWritable);
            for (int i = 0; i < nLength; ++i) {
                if (pWritable[i] == '/') {
                    pWritable[i] = '\\';
                }
                pWritable[i] = (char)toupper((unsigned char)pWritable[i]);
            }
            const char *pSuffix = (strchr(pszName, ';') != NULL) ? "" : ";1";
            sprintf(szPath, "%s%s", pWritable, pSuffix);
        }
    } else {
        pFile->isOnCD = 0;
        strcpy(szDev, "host0");
        sprintf(szPath, "%s:%s", szDev, pszName);
    }

    printf("file: %s\n", szPath);

    if (pFile->isOnCD != 0) {
        if (g_bCdInitialized == 0) {
            g_bCdInitialized = 1;
        }
        void *pHeap = sceSifAllocIopHeap(kIopHeapSize);
        uintptr_t nAligned = ((uintptr_t)pHeap + 15) >> 4 << 4;
        sceCdStInit(kStreamSectors, kStreamBanks, (unsigned int)nAligned);
        *(void **)&pFile->cdFile[kHeapSlot] = pHeap;

        sceCdlFILE *pEntry = (sceCdlFILE *)&pFile->cdFile[0];
        if (sceCdSearchFile(pEntry, szPath) == 0) {
            printf("Cannot open '%s'(sceCdSearchFile)\n", szPath);
            return 0;
        }
        pFile->size = (int)pEntry->size;
        sceCdRMode mode = {0, 0, 0, 0};
        sceCdStStart(pEntry->lsn, &mode);
        return 1;
    }

    // The descriptor and the size are stored before they are tested, as the binary does.
    int nDescriptor = sceOpen(szPath, kOpenReadOnly);
    pFile->fd = nDescriptor;
    if (nDescriptor < 0) {
        printf("Cannot open '%s'(sceOpen)\n", szPath);
        return 0;
    }

    int nSize = sceLseek(nDescriptor, 0, SCE_SEEK_END);
    pFile->size = nSize;
    if (nSize < 0) {
        printf("sceLseek() fails (%s): %d\n", szPath, nSize);
        sceClose(nDescriptor);
        return 0;
    }

    if (sceLseek(nDescriptor, 0, SCE_SEEK_SET) < 0) {
        printf("sceLseek() fails (%s)\n", szPath);
        sceClose(nDescriptor);
        return 0;
    }
    return 1;
}

// NTSC-U/C: 0x0058e130, PAL: 0x005d1488
int strFileClose(StrFile* pFile) {
    if (pFile->isOnCD != 0) {
        sceCdStStop();
        void *pHeap = *(void **)&pFile->cdFile[kHeapSlot];
        sceSifFreeIopHeap(pHeap);
    } else {
        sceClose(pFile->fd);
    }
    return 1;
}

// NTSC-U/C: 0x0058e180, PAL: 0x005d14d8
int strFileRead(StrFile *pFile, void *pBuffer, int nSize) {
    if (pFile->isOnCD != 0) {
        unsigned int nError = 0;
        int nSectors = sceCdStRead((unsigned int)nSize >> kSectorShift,
                                   (unsigned int *)pBuffer,
                                   1,
                                   &nError);
        return nSectors << kSectorShift;
    }
    return sceRead(pFile->fd, pBuffer, nSize);
}
