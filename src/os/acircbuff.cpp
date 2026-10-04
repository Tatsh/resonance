#include "os/acircbuff.h"

#include <string.h>

#include "os/async.h"
#include "os/log.h"

ACircBuff::ACircBuff(char *pBuff, int nSize)
    : mBuff(pBuff), mBuffSize(nSize), mWrite(pBuff), mRead(pBuff), mWrap(pBuff + nSize) {
}

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

void ACircBuff::WrapWriteIfOk() {
    if (mWrite >= mWrap && mBuff < mRead) {
        mWrite = mBuff;
    }
}

char *ACircBuff::AdvanceRead(int nBytes) {
    mRead += nBytes;
    if (mRead >= mWrap) {
        mRead = mBuff + (mRead - mWrap);
    }
    return mRead;
}

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

int ACircBuff::WillWriteWrap(int nBytes) const {
    if (mWrite + nBytes > mWrap) {
        return mWrap - mWrite;
    }
    return nBytes;
}

char *ACircBuff::AdvanceWrite(int nBytes) {
    mWrite += nBytes;
    if (mWrite >= mWrap && mRead != mBuff) {
        mWrite = mBuff;
    }
    return mWrite;
}

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

int ACircBuff::Write(int nFile, int nBytes) {
    WrapWriteIfOk();
    if (mWrite < mRead) {
        if (nBytes >= mRead - mWrite) {
            return 0;
        }
        read(nFile, mWrite, nBytes); // Yes, the count read is discarded.
        mWrite += nBytes;
        return nBytes;
    }
    if (mWrap - mWrite < nBytes) {
        if (nBytes >= mRead - mBuff) {
            return 0;
        }
        // Yes, the binary reads to the old write position and only then wraps it to mBuff.
        read(nFile, mWrite, nBytes);
        mWrite = mBuff + nBytes;
        return nBytes;
    }
    read(nFile, mWrite, nBytes);
    mWrite += nBytes;
    return nBytes;
}

void ACircBuff::Dump(const char *pszLabel) const {
    printf("%s: circbuff: pRead: %p, pWrite: %p, pWrap: %p, pBuff: %p, buffsz: %d\n",
           pszLabel,
           mRead,
           mWrite,
           mWrap,
           mBuff,
           mBuffSize);
}
