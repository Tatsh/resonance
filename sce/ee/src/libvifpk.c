#include <stddef.h>
#include <stdint.h>

#include <libvifpk.h>

// 0x0061e7c8
void sceVif1PkInit(sceVif1Packet *pPacket, void *pBase) {
    pPacket->pCurrent = (unsigned int *)pBase;
    pPacket->mOtherWords[0] = 0u;
    pPacket->pBase = pBase;
}

// 0x0062dd48
void sceVif1PkReset(sceVif1Packet *pPacket) {
    unsigned int *pBase = (unsigned int *)pPacket->pBase;

    pPacket->mOtherWords[0] = 0u;
    pPacket->pCurrent = pBase;
}

// 0x0061e7d8
void sceVif1PkCnt(sceVif1Packet *pPacket, unsigned int nOption) {
    unsigned int *pCurrent = sceVif1PkTerminate(pPacket);

    pPacket->mOtherWords[0] = (uintptr_t)pCurrent;
    pCurrent[0] = nOption | 0x10000000u;
    pPacket->mOtherWords[1] = 0u;
    pPacket->pCurrent = pCurrent + 2;
    pCurrent[1] = 0u;
}

// 0x0061fd08
void sceVif1PkOpenDirectCode(sceVif1Packet *pPacket, int bStall) {
    unsigned int *pCode;
    unsigned int nCode = 0x50000000u;

    sceVif1PkAlign(pPacket, 2, 3);
    if (bStall != 0) {
        nCode = 0xd0000000u;
    }
    pCode = pPacket->pCurrent;
    pPacket->pCurrent = pCode + 1;
    pPacket->mOtherWords[1] = (unsigned int)(uintptr_t)pCode;
    *pCode = nCode;
}

// 0x0062a940
void sceVif1PkOpenGifTag(sceVif1Packet *pPacket, sceGifTag gifTag) {
    sceGifTag *pTag = (sceGifTag *)pPacket->pCurrent;

    *pTag = gifTag;
    pPacket->pCurrent = (unsigned int *)(pTag + 1);
    pPacket->mOtherWords[3] = (unsigned int)(uintptr_t)pTag;
}

// 0x0062f3f8
unsigned int *sceVif1PkReserve(sceVif1Packet *pPacket, unsigned int nWords) {
    unsigned int *pResult = pPacket->pCurrent;

    pPacket->pCurrent = pResult + nWords;
    return pResult;
}

// 0x00613d50
void sceVif1PkCloseGifTag(sceVif1Packet *pPacket) {
    unsigned int *pCurrent = pPacket->pCurrent;
    unsigned long long *pTag = (unsigned long long *)(uintptr_t)pPacket->mOtherWords[3];
    unsigned long long nTag = pTag[0];
    unsigned int nUnits = (unsigned int)((unsigned char *)pCurrent - (unsigned char *)pTag) / 8u;
    unsigned int nCount = nUnits - 2u;
    unsigned int nFlag = (unsigned int)((nTag >> 58) & 3ull);
    unsigned int nReg;

    if (nFlag != 1u) {
        nCount >>= 1;
    }
    if (nFlag != 2u) {
        nReg = (unsigned int)(nTag >> 60);
        if (nReg == 0u) {
            nReg = 16u;
        }
        nCount = (nCount + nReg - 1u) / nReg;
    }
    pPacket->mOtherWords[3] = 0u;
    pTag[0] = nTag + nCount;
    while (((uintptr_t)pCurrent & 0xcu) != 0u) {
        *pCurrent++ = 0u;
    }
    pPacket->pCurrent = pCurrent;
}

// 0x0062dd78
void sceVif1PkCloseDirectCode(sceVif1Packet *pPacket) {
    unsigned int *pCode = (unsigned int *)(uintptr_t)pPacket->mOtherWords[1];
    unsigned int nQuads = (unsigned int)(pPacket->pCurrent - pCode - 1) / 4u;

    pPacket->mOtherWords[1] = 0u;
    *pCode += nQuads;
}

// 0x0061d720
void sceVif1PkEnd(sceVif1Packet *pPacket, unsigned int nOption) {
    unsigned int *pCurrent = sceVif1PkTerminate(pPacket);

    pPacket->mOtherWords[0] = (uintptr_t)pCurrent;
    pCurrent[0] = nOption | 0x70000000u;
    pPacket->mOtherWords[1] = 0u;
    pPacket->pCurrent = pCurrent + 2;
    pCurrent[1] = 0u;
}

// 0x006206d8
unsigned int *sceVif1PkTerminate(sceVif1Packet *pPacket) {
    unsigned int *pCurrent = pPacket->pCurrent;
    unsigned int *pTag = (unsigned int *)(uintptr_t)pPacket->mOtherWords[0];

    while (((uintptr_t)pCurrent & 0xcu) != 0u) {
        *pCurrent++ = 0u;
    }
    if (pTag != NULL) {
        *pTag += (unsigned int)(pCurrent - pTag) / 4u - 1u;
    }
    pPacket->mOtherWords[0] = 0u;
    pPacket->pCurrent = pCurrent;
    return pCurrent;
}

// 0x00651fe8
void sceVif1PkAlign(sceVif1Packet *pPacket, int nKind, int nSize) {
    unsigned int nKeep = ((unsigned int)nKind + 2u) & 31u;
    unsigned int nLow = 0xffffffffu >> ((32u - nKeep) & 31u);
    unsigned int nEnd =
        (unsigned int)(((uintptr_t)pPacket->pCurrent & ~(uintptr_t)nLow) + (uintptr_t)nSize * 4u);
    unsigned int *pCurrent = pPacket->pCurrent;

    if (nEnd < (uintptr_t)pCurrent) {
        nEnd += nLow + 1u;
    }
    while ((uintptr_t)pCurrent < nEnd) {
        *pCurrent++ = 0u;
        pPacket->pCurrent = pCurrent;
    }
}
