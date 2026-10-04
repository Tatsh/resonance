#include <stddef.h>
#include <stdint.h>

#include <libgifpk.h>

// DMA tag identifiers stored by the count, end, and reference paths, with the shifts and masks
// used to read the transfer mode and the register count back from an open header. Every value
// below comes straight from the disassembly of the routines in this file.
enum {
    kCntTagId = 0x10000000,
    kEndTagId = 0x70000000,
    kRefTagId = 0x30000000,
    kDefaultRegisterCount = 16,
    kGifFlagShift = 58,
    kGifFlagMask = 3,
    kGifRegisterShift = 60
};

// The mask applied to the reference data pointer. It is kept apart because it does not fit in an
// enumeration constant.
static const unsigned int kRefAddressMask = 0x9FFFFFFFU;

void sceGifPkInit(sceGifPkData *pPacket, void *pBuffer) {
    // Attach the packet to the buffer, and clear the pending tag.
    pPacket->mCurrent = pBuffer;
    pPacket->mBase = pBuffer;
    pPacket->mDmaTag = NULL;
}

void sceGifPkReset(sceGifPkData *pPacket) {
    // Rewind the packet to its base, and clear the pending tag.
    pPacket->mCurrent = pPacket->mBase;
    pPacket->mDmaTag = NULL;
}

void sceGifPkCnt(sceGifPkData *pPacket, int nLoop, int nEop, int nPre) {
    unsigned int *tag;

    // Finalise any pending tag, then open a continue tag for the new run.
    sceGifPkTerminate(pPacket);
    tag = (unsigned int *)pPacket->mCurrent;
    pPacket->mDmaTag = tag;
    tag[0] = (unsigned int)nPre | kCntTagId;
    tag[1] = 0U;
    tag[2] = (unsigned int)nLoop;
    tag[3] = (unsigned int)nEop;
    pPacket->mCurrent = tag + 4;
}

void sceGifPkEnd(sceGifPkData *pPacket, int nLoop, int nEop, int nPre) {
    unsigned int *tag;

    // Finalise any pending tag, then open an end tag for the new run.
    sceGifPkTerminate(pPacket);
    tag = (unsigned int *)pPacket->mCurrent;
    pPacket->mDmaTag = tag;
    tag[0] = (unsigned int)nPre | kEndTagId;
    tag[1] = 0U;
    tag[2] = (unsigned int)nLoop;
    tag[3] = (unsigned int)nEop;
    pPacket->mCurrent = tag + 4;
}

void sceGifPkOpenGsAD(sceGifPkData *pPacket, const void *pData) {
    unsigned long long *current;
    const unsigned long long *header;

    // Copy the sixteen byte header to the current position.
    current = (unsigned long long *)pPacket->mCurrent;
    header = (const unsigned long long *)pData;
    current[0] = header[0];
    current[1] = header[1];
    // Remember the header address, then advance past it.
    pPacket->mPrevious = current;
    pPacket->mCurrent = current + 2;
}

void sceGifPkAddGsAD(sceGifPkData *pPacket, int nAddress, unsigned long long nData) {
    unsigned long long *current;

    // Store the data word followed by the address word, then advance past both.
    current = (unsigned long long *)pPacket->mCurrent;
    current[0] = nData;
    current[1] = (unsigned long long)(unsigned int)nAddress;
    pPacket->mCurrent = current + 2;
}

void sceGifPkCloseGifTag(sceGifPkData *pPacket) {
    unsigned char *previous;
    unsigned char *current;
    unsigned long long tag;
    unsigned int mode;
    int loopCount;

    previous = (unsigned char *)pPacket->mPrevious;
    current = (unsigned char *)pPacket->mCurrent;
    tag = *(unsigned long long *)previous;
    loopCount = (int)((current - previous) >> 3) - 2;
    mode = (unsigned int)((tag >> kGifFlagShift) & kGifFlagMask);
    // Fold the word count to a quadword count outside register list mode.
    if (mode != 1U) {
        loopCount = (int)((unsigned int)loopCount >> 1);
    }
    // Scale the count by the register count outside image mode. A zero field selects sixteen,
    // so the divisor below is never zero.
    if (mode != 2U) {
        unsigned int divisor;
        unsigned int rate;

        divisor = (unsigned int)(tag >> kGifRegisterShift);
        if (divisor == 0U) {
            rate = kDefaultRegisterCount;
        } else {
            rate = divisor;
        }
        loopCount = (int)(((unsigned int)loopCount + rate - 1U) / rate);
    }
    {
        unsigned long long patched;

        // Add the count to the preserved header word, forget the open tag, and pad the
        // position to sixteen bytes with zeroes.
        patched = tag + (unsigned long long)(unsigned int)loopCount;
        pPacket->mPrevious = NULL;
        *(unsigned long long *)previous = patched;
        while ((((uintptr_t)current) & 0xCU) != 0U) {
            *(unsigned int *)current = 0U;
            current += 4;
        }
        pPacket->mCurrent = current;
    }
}

void sceGifPkTerminate(sceGifPkData *pPacket) {
    unsigned char *current;
    unsigned int *dmaTag;

    current = (unsigned char *)pPacket->mCurrent;
    dmaTag = (unsigned int *)pPacket->mDmaTag;
    // Pad the position to sixteen bytes with zeroes.
    while ((((uintptr_t)current) & 0xCU) != 0U) {
        *(unsigned int *)current = 0U;
        current += 4;
    }
    // Fold the quadwords written since the pending tag was opened back into its length word.
    if (dmaTag != NULL) {
        unsigned int quadwords;

        quadwords = (unsigned int)(current - (unsigned char *)dmaTag) >> 4;
        *dmaTag += quadwords - 1U;
    }
    pPacket->mDmaTag = NULL;
    pPacket->mCurrent = current;
}

unsigned long long *sceGifPkReserve(sceGifPkData *pPacket, int nWords) {
    unsigned long long *reserved;

    // Hand out the current position, then skip past the requested words.
    reserved = (unsigned long long *)pPacket->mCurrent;
    pPacket->mCurrent = (unsigned char *)reserved + (nWords * 4);
    return reserved;
}

void sceGifPkRef(
    sceGifPkData *pPacket, void *pData, int nQuadwords, int nOption1, int nOption2, int nFlag) {
    unsigned int *tag;

    // Finalise any pending tag, then write a reference tag for the data and its length.
    sceGifPkTerminate(pPacket);
    tag = (unsigned int *)pPacket->mCurrent;
    tag[0] = (unsigned int)nFlag | (unsigned int)nQuadwords | kRefTagId;
    tag[1] = (unsigned int)(uintptr_t)pData & kRefAddressMask;
    tag[2] = (unsigned int)nOption1;
    tag[3] = (unsigned int)nOption2;
    pPacket->mCurrent = tag + 4;
}
