#include "os/circbuff.h"

#include <string.h>

#include "os/async.h"
#include "os/log.h"

// 0x0060e8b0
CircBuff::CircBuff(char *pBuff, int nSize)
    : mBuff(pBuff), mBuffSize(nSize), mWrite(pBuff), mRead(pBuff), mWrap(pBuff + nSize) {
}

// 0x0060e930
int CircBuff::FreeSpace() {
    if (mWrite == mRead) {
        return 0;
    }
    WrapWrite();
    if (mWrite < mRead) {
        return mRead - mWrite - 1;
    }
    return (mWrap - mWrite) + (mRead - mBuff) - 1;
}

// 0x0060e998
int CircBuff::HasSpace(int nBytes) {
    if (mWrite == mRead) {
        return 0;
    }
    WrapWrite();
    if (mWrite < mRead) {
        return nBytes < mRead - mWrite;
    }
    // This test measures the tail against mBuffSize rather than mWrap, which is the same point.
    if ((mBuff + mBuffSize) - mWrite < nBytes) {
        return nBytes < mRead - mBuff;
    }
    return 1;
}

// 0x0060ea20
void CircBuff::WrapWrite() {
    if (mWrite >= mWrap && mBuff < mRead) {
        mWrite = mBuff;
    }
}

// 0x0060ea50
char *CircBuff::AdvanceRead(int nBytes) {
    mRead += nBytes;
    if (mRead >= mWrap) {
        mRead = mBuff + (mRead - mWrap);
    }
    return mRead;
}

// 0x0060ea80
int CircBuff::IsClearOfWrite(const char *pStart, int nBytes) const {
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

// 0x0060eaf0
int CircBuff::ContiguousWriteSize(int nBytes) const {
    if (mWrite + nBytes > mWrap) {
        return mWrap - mWrite;
    }
    return nBytes;
}

// 0x0060eb18
char *CircBuff::AdvanceWrite(int nBytes) {
    mWrite += nBytes;
    if (mWrite >= mWrap && mRead != mBuff) {
        mWrite = mBuff;
    }
    return mWrite;
}

// 0x0060eb48
int CircBuff::Write(const void *pSrc, int nBytes) {
    WrapWrite();
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

// 0x0060ec08
int CircBuff::ReadFromFile(int nFile, int nBytes) {
    WrapWrite();
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
        FileRead(nFile, mBuff, nBytes);
        mWrite = mBuff + nBytes;
        return nBytes;
    }
    FileRead(nFile, mWrite, nBytes);
    mWrite += nBytes;
    return nBytes;
}

// 0x0060ecd0
void CircBuff::Dump(const char *pszLabel) const {
    LogPrintf("%s: circbuff: pRead: %p, pWrite: %p, pWrap: %p, pBuff: %p, buffsz: %d\n",
              pszLabel,
              mRead,
              mWrite,
              mWrap,
              mBuff,
              mBuffSize);
}
