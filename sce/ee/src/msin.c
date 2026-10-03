#include <csl.h>
#include <stddef.h>

enum {
    // The input streams live in the second buffer group.
    kInputGroup = 1,
    // A stream header holds two counters, so a usable buffer is at least this long.
    kStreamHeaderSize = 8,
    // Only the high nibble of a channel message selects its length.
    kStatusMask = 0xf0,
};

// NTSC-U/C: 0x005e4600, PAL: 0x006267c0
static int put_message(sceCslCtx *pCtx, unsigned int nPort, unsigned char *pBytes, int nCount) {
    sceCslBuffGrp *pGroups;
    sceCslMidiStream *pStream;
    unsigned char *pDest;
    unsigned int nValid;
    int i;

    if (pCtx == NULL) {
        return -1;
    }
    pGroups = pCtx->buffGrp;
    if (nPort >= (unsigned int)pGroups[kInputGroup].buffNum) {
        return -1;
    }
    pStream = (sceCslMidiStream *)pGroups[kInputGroup].buffCtx[nPort].buff;
    nValid = pStream->validsize;
    if (pStream->buffsize < nValid + (unsigned int)nCount + kStreamHeaderSize) {
        return -1;
    }
    pStream->validsize = nValid + (unsigned int)nCount;
    if (nCount == 0) {
        return 0;
    }
    pDest = pStream->data + nValid;
    for (i = 0; i < nCount; ++i) {
        pDest[i] = pBytes[i];
    }
    return 0;
}

// NTSC-U/C: 0x005e4570, PAL: 0x00626730
int sceMSIn_Init(sceCslCtx *pCtx) {
    sceCslBuffGrp *pGroups;
    sceCslMidiStream *pStream;
    int nBuffNum;
    int i;

    if (pCtx == NULL) {
        return -1;
    }
    if (pCtx->buffGrpNum < 2) {
        return -1;
    }
    pGroups = pCtx->buffGrp;
    if (pGroups == NULL) {
        return -1;
    }
    nBuffNum = pGroups[kInputGroup].buffNum;
    if (nBuffNum == 0) {
        return -1;
    }
    if (pGroups[kInputGroup].buffCtx == NULL) {
        return -1;
    }
    if (nBuffNum <= 0) {
        return 0;
    }
    for (i = 0; i < nBuffNum; ++i) {
        pStream = (sceCslMidiStream *)pGroups[kInputGroup].buffCtx[i].buff;
        if (pStream == NULL) {
            return -1;
        }
        if (pStream->buffsize < kStreamHeaderSize) {
            return -1;
        }
    }
    return 0;
}

// NTSC-U/C: 0x005e4698, PAL: 0x00626858
int sceMSIn_PutMsg(sceCslCtx *pCtx, unsigned int nPort, unsigned int nMsg) {
    unsigned int nCopy = nMsg;
    int nCount;

    switch (nCopy & kStatusMask) {
    case 0x80:
    case 0xc0:
    case 0xd0:
        // These statuses queue two bytes.
        nCount = 2;
        break;
    case 0x90:
    case 0xa0:
    case 0xb0:
    case 0xe0:
        // These statuses queue three bytes.
        nCount = 3;
        break;
    default:
        // The module rejects any other status.
        return -1;
    }
    return put_message(pCtx, nPort, (unsigned char *)&nCopy, nCount);
}
