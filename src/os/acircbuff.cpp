#include "os/acircbuff.h"

#include <string.h>

#include "os/async.h"
#include "os/log.h"

// NTSC-U/C: 0x0060e8b0, PAL: 0x0064f520
ACircBuff::ACircBuff(char *pBuff, int nSize)
    : mBuff(pBuff), mBuffSize(nSize), mWrite(pBuff), mRead(pBuff), mWrap(pBuff + nSize) {
}

// NTSC-U/C: 0x0060e930, PAL: 0x0064f5a0
int ACircBuff::Avail() {
    if (mWrite == mRead) {
        return 0;
    }
    WrapWriteIfOk();
    if (mWrite < mRead) {
        return mRead - mWrite - 1;
    }
    return (mWrap - mWrite) + (mRead - mBuff) - 1;
}

// NTSC-U/C: 0x0060e998, PAL: 0x0064f608
int ACircBuff::IsRoom(int nBytes) {
    if (mWrite == mRead) {
        return 0;
    }
    WrapWriteIfOk();
    if (mWrite < mRead) {
        return nBytes < mRead - mWrite;
    }
    // This test measures the tail against mBuffSize rather than mWrap, which is the same point.
    if ((mBuff + mBuffSize) - mWrite < nBytes) {
        return nBytes < mRead - mBuff;
    }
    return 1;
}

// NTSC-U/C: 0x0060ea20, PAL: 0x0064f690
void ACircBuff::WrapWriteIfOk() {
    if (mWrite >= mWrap && mBuff < mRead) {
        mWrite = mBuff;
    }
}

// NTSC-U/C: 0x0060ea50, PAL: 0x0064f6c0
char *ACircBuff::AdvanceRead(int nBytes) {
    mRead += nBytes;
    if (mRead >= mWrap) {
        mRead = mBuff + (mRead - mWrap);
    }
    return mRead;
}

// NTSC-U/C: 0x0060ea80, PAL: 0x0064f6f0
int ACircBuff::FullyRead(const char *pStart, int nBytes) const {
    const char *pEnd = pStart + nBytes;
    if (mWrap < pEnd) {
        pEnd = mBuff + (pEnd - mWrap);
    }
    if (pStart < pEnd) {
        return mWrite < pStart || mWrite >= pEnd;
    }
    if (mWrite < pStart) {
        return mWrite >= pEnd;
    }
    return 0;
}

// NTSC-U/C: 0x0060eaf0, PAL: 0x0064f760
int ACircBuff::WillWriteWrap(int nBytes) const {
    if (mWrite + nBytes > mWrap) {
        return mWrap - mWrite;
    }
    return nBytes;
}

// NTSC-U/C: 0x0060eb18, PAL: 0x0064f788
char *ACircBuff::AdvanceWrite(int nBytes) {
    mWrite += nBytes;
    if (mWrite >= mWrap && mRead != mBuff) {
        mWrite = mBuff;
    }
    return mWrite;
}

// NTSC-U/C: 0x0060eb48, PAL: 0x0064f7b8
int ACircBuff::Write(const void *pSrc, int nBytes) {
    WrapWriteIfOk();
    if (mWrite < mRead) {
        if (nBytes >= mRead - mWrite) {
            return 0;
        }
        memcpy(mWrite, pSrc, nBytes);
        mWrite += nBytes;
        return nBytes;
    }
    if (mWrap - mWrite < nBytes) {
        if (nBytes >= mRead - mBuff) {
            return 0;
        }
        memcpy(mBuff, pSrc, nBytes);
        mWrite = mBuff + nBytes;
        return nBytes;
    }
    memcpy(mWrite, pSrc, nBytes);
    mWrite += nBytes;
    return nBytes;
}

// NTSC-U/C: 0x0060ec08, PAL: 0x0064f878
int ACircBuff::Write(int nFile, int nBytes) {
    WrapWriteIfOk();
    if (mWrite < mRead) {
        if (nBytes >= mRead - mWrite) {
            return 0;
        }
        FileRead(nFile, mWrite, nBytes); // Yes, the count read is discarded.
        mWrite += nBytes;
        return nBytes;
    }
    if (mWrap - mWrite < nBytes) {
        if (nBytes >= mRead - mBuff) {
            return 0;
        }
        // Yes, the binary reads to the old write position and only then wraps it to mBuff.
        FileRead(nFile, mWrite, nBytes);
        mWrite = mBuff + nBytes;
        return nBytes;
    }
    FileRead(nFile, mWrite, nBytes);
    mWrite += nBytes;
    return nBytes;
}

// NTSC-U/C: 0x0060ecd0, PAL: 0x0064f940
void ACircBuff::Dump(const char *pszLabel) const {
    LogPrintf("%s: circbuff: pRead: %p, pWrite: %p, pWrap: %p, pBuff: %p, buffsz: %d\n",
              pszLabel,
              mRead,
              mWrite,
              mWrap,
              mBuff,
              mBuffSize);
}
