#include "rndartt/arlereader.h"

namespace {

constexpr unsigned char kControlTerminator = 0;
constexpr unsigned char kControlLengthMask = 0x7f;
constexpr unsigned char kControlLiteralFlag = 0x80;

} // namespace

// 0x0060da78
// The keyed and the opaque walks are separate code in the binary rather than one walk
// with the comparison inside it, and the shape here reproduces that.
unsigned char *ARleReader::DecodeRow(unsigned char *pDest) {
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
    const unsigned char nKey = static_cast<unsigned char>(mTransparentValue);
    while (nColumns > 0) {
        const unsigned char nControl = *mSource;
        ++mSource;
        const int nLength = nControl & kControlLengthMask;
        nColumns -= nLength;
        if ((nControl & kControlLiteralFlag) != 0) {
            for (int nRemaining = nLength; nRemaining > 0; --nRemaining) {
                const unsigned char nValue = *mSource;
                if (nValue != nKey) {
                    *pDest = nValue;
                }
                ++pDest;
                ++mSource;
            }
        } else {
            const unsigned char nValue = *mSource;
            if (nValue == nKey) {
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

// 0x0060dc98
void ARleReader::DecodeRows(unsigned char *pDest) {
    while (*mSource != kControlTerminator) {
        pDest = DecodeRow(pDest);
    }
}

// 0x0060dc10
// mWidth is read once before the first row and re-derived from a register afterwards,
// so a width written between rows would not be seen.
void ARleReader::SkipRows(int nRows) {
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
