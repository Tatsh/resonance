#include "rndartt/arle8reader.h"

namespace {

constexpr unsigned char kControlTerminator = 0;
constexpr unsigned char kControlLengthMask = 0x7f;
constexpr unsigned char kControlLiteralFlag = 0x80;

} // namespace

unsigned char *ARle8Reader::UnpackRow(unsigned char *pDest) {
    // The keyed and the opaque walks are separate code in the binary rather than one walk with the
    // comparison inside it, and the shape here reproduces that.
    if (*mSource == kControlTerminator) {
        return pDest;
    }
    int nColumns = mWidth;
    if (mTransparentValue < 0) {
        while (nColumns > 0) {
            const unsigned char nControl = *mSource;
            ++mSource;
            const int nLength = nControl & kControlLengthMask;
            nColumns -= nLength;
            if ((nControl & kControlLiteralFlag) != 0) {
                for (int nRemaining = nLength; nRemaining > 0; --nRemaining) {
                    *pDest = *mSource;
                    ++pDest;
                    ++mSource;
                }
            } else {
                const unsigned char nValue = *mSource;
                for (int nRemaining = nLength; nRemaining > 0; --nRemaining) {
                    *pDest = nValue;
                    ++pDest;
                }
                ++mSource;
            }
        }
        return pDest;
    }
    while (nColumns > 0) {
        const unsigned char nControl = *mSource;
        ++mSource;
        const int nLength = nControl & kControlLengthMask;
        nColumns -= nLength;
        if ((nControl & kControlLiteralFlag) != 0) {
            for (int nRemaining = nLength; nRemaining > 0; --nRemaining) {
                const unsigned char nValue = *mSource;
                if (nValue != mTransparentValue) {
                    *pDest = nValue;
                }
                ++pDest;
                ++mSource;
            }
        } else {
            const unsigned char nValue = *mSource;
            if (nValue == mTransparentValue) {
                pDest += nLength;
            } else {
                for (int nRemaining = nLength; nRemaining > 0; --nRemaining) {
                    *pDest = nValue;
                    ++pDest;
                }
            }
            ++mSource;
        }
    }
    return pDest;
}

void ARle8Reader::UnpackAll(unsigned char *pDest) {
    while (*mSource != kControlTerminator) {
        pDest = UnpackRow(pDest);
    }
}

void ARle8Reader::SkipRow(int nRows) {
    if (*mSource == kControlTerminator) {
        return;
    }
    for (int nRemainingRows = nRows; nRemainingRows > 0; --nRemainingRows) {
        int nColumns = mWidth;
        while (nColumns > 0) {
            const unsigned char nControl = *mSource;
            const int nLength = nControl & kControlLengthMask;
            ++mSource;
            if ((nControl & kControlLiteralFlag) != 0) {
                mSource += nLength;
            } else {
                ++mSource;
            }
            nColumns -= nLength;
        }
        if (*mSource == kControlTerminator) {
            return;
        }
    }
}
