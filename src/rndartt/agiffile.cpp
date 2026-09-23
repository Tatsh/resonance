#include "rndartt/agiffile.h"

#include <string.h>

#include "rndartt/abitmap.h"
#include "rndartt/apalette.h"

namespace {

struct GifScreenDescriptor {
    char mSignature[6];
    unsigned short mWidth;
    unsigned short mHeight;
    unsigned char mFlags;
    unsigned char mBackground;
    unsigned char mAspect;
};

struct GifImageDescriptor {
    unsigned short mLeft;
    unsigned short mTop;
    unsigned short mWidth;
    unsigned short mHeight;
    unsigned char mFlags;
};

struct GifGraphicControl {
    unsigned char mBlockSize;
    unsigned char mFlags;
    unsigned short mDelay;
    unsigned char mTransparentIndex;
    unsigned char mTerminator;
};

constexpr int kScreenDescriptorSize = 13;
constexpr int kImageDescriptorSize = 9;
constexpr int kGraphicControlSize = 5;
constexpr int kSignatureCompareLength = 3;
constexpr int kRGBByteCount = 3;
constexpr int kMaxColorCount = 256;
constexpr int kMsPerDelayUnit = 10;

constexpr unsigned char kColorTableFlag = 0x80;
constexpr unsigned char kColorTableSizeMask = 0x07;
constexpr unsigned char kInterlaceFlag = 0x40;
constexpr unsigned char kTransparentFlag = 0x01;

// Block introducers and extension labels.
constexpr char kIntroducerImage = ',';
constexpr char kIntroducerExtension = '!';
constexpr char kIntroducerTrailer = ';';
constexpr char kIntroducerOddExtension = 1;
constexpr char kEndOfFile = -1;
constexpr unsigned char kLabelGraphicControl = 0xf9;
constexpr unsigned char kLabelPlainText = 0x01;
constexpr unsigned char kLabelComment = 0xfe;
constexpr unsigned char kLabelApplication = 0xff;
constexpr int kPlainTextHeaderSize = 13;
constexpr int kApplicationHeaderSize = 12;

constexpr int kMinCodeSize = 2;
constexpr int kMaxMinCodeSize = 8;
constexpr int kMaxCodeSize = 12;
constexpr int kMaxCodeCount = 1 << kMaxCodeSize;
constexpr short kNoCode = -1;
constexpr int kBitsPerByte = 8;
constexpr int kBitsPerHalfword = 16;
constexpr int kSubBlockSize = 256;

// The line buffer has no recorded size. The static at 0x007b8100 bounds it, and it may belong to
// another translation unit.
constexpr int kLineBufferSize = 0xac8;

const char *const kSignature = "GIF";

// The byte every block introducer and code size is read into.
// 0x008e8168
signed char s_cByte;

// 0x008e8170
APalette s_palette;

// 0x008e8578
GifImageDescriptor s_image;

// 0x008e8588
GifGraphicControl s_graphicControl;

// 0x008e8590
GifScreenDescriptor s_screen;

// 0x007b35e8
// the low n bits for each code size n.
const unsigned short kCodeMasks[] = {0x0000,
                                     0x0001,
                                     0x0003,
                                     0x0007,
                                     0x000f,
                                     0x001f,
                                     0x003f,
                                     0x007f,
                                     0x00ff,
                                     0x01ff,
                                     0x03ff,
                                     0x07ff,
                                     0x0fff,
                                     0x1fff,
                                     0x3fff,
                                     0x7fff};

// Each is bounded at six entries by its neighbour, and the trailing zeros are what a fifth pass
// reads.
// 0x007b3608
const int kInterlaceStep[] = {8, 8, 4, 2, 0, 0};
// 0x007b3620
const int kInterlaceStart[] = {0, 4, 2, 1, 0, 0};

// 0x007b3638
unsigned char s_abStack[kMaxCodeCount];

// 0x007b4638
unsigned char s_abSuffix[kMaxCodeCount];

// 0x007b5638
short s_anPrefix[kMaxCodeCount];

// 0x007b7638
unsigned char s_abLine[kLineBufferSize];

// Step to the next data byte, reading the next sub-block when the current one is used up. A zero
// length block or a short read fails.
inline bool
NextDataByte(FILE *pFile, unsigned char *pBlock, unsigned char **ppByte, unsigned char **ppEnd) {
    ++*ppByte;
    if (*ppByte < *ppEnd) {
        return true;
    }
    unsigned char nCount;
    fread(&nCount, 1, 1, pFile);
    if (nCount == 0) {
        return false;
    }
    if (fread(pBlock, 1, nCount, pFile) != nCount) {
        return false;
    }
    *ppEnd = pBlock + nCount;
    *ppByte = pBlock;
    return true;
}

} // namespace

// 0x0062a9c0
int AGifFile::ReadHeader() {
    GifScreenDescriptor screen;
    fread(&screen, 1, kScreenDescriptorSize, mFile);
    s_screen = screen;
    if (memcmp(s_screen.mSignature, kSignature, kSignatureCompareLength) != 0) {
        return kAGfxFileBadFormat;
    }
    memset(&s_graphicControl, 0, sizeof(s_graphicControl));
    if ((s_screen.mFlags & kColorTableFlag) != 0) {
        unsigned char abColors[kMaxColorCount * kRGBByteCount];
        memset(abColors, 0, sizeof(abColors));
        const int nCount = 1 << ((s_screen.mFlags & kColorTableSizeMask) + 1);
        if (nCount > kMaxColorCount) {
            return kAGfxFileBadFormat;
        }
        fread(abColors, 1, nCount * kRGBByteCount, mFile);
        s_palette.SetEntriesRGB(abColors, 0, nCount);
    }
    return kAGfxFileOk;
}

// 0x0062aaf8
int AGifFile::ReadImage(ABitmap *pImage, int *pbEnd) {
    *pbEnd = 0;
    for (;;) {
        fread(&s_cByte, 1, 1, mFile);
        if (s_cByte == 0) {
            fread(&s_cByte, 1, 1, mFile);
        }
        const signed char cIntroducer = s_cByte;
        if (cIntroducer < 0 || cIntroducer == kIntroducerTrailer) {
            *pbEnd = 1;
            return kAGfxFileOk;
        }
        if (cIntroducer == kIntroducerImage) {
            GifImageDescriptor image;
            fread(&image, 1, kImageDescriptorSize, mFile);
            s_image = image;
            *pImage = ABitmap(nullptr,
                              kABitmapFormatLinear8,
                              false,
                              s_image.mWidth,
                              s_image.mHeight,
                              s_image.mWidth);
            mBounds.mLeft = static_cast<short>(s_image.mLeft);
            mBounds.mTop = static_cast<short>(s_image.mTop);
            mBounds.mRight = static_cast<short>(s_image.mLeft + s_image.mWidth);
            mBounds.mBottom = static_cast<short>(s_image.mTop + s_image.mHeight);
            if ((s_image.mFlags & kColorTableFlag) != 0) {
                const int nCount = 1 << ((s_image.mFlags & kColorTableSizeMask) + 1);
                if (nCount > kMaxColorCount) {
                    return kAGfxFileBadFormat;
                }
                unsigned char abColors[kMaxColorCount * kRGBByteCount];
                fread(abColors, 1, nCount * kRGBByteCount, mFile);
                s_palette.SetEntriesRGB(abColors, 0, nCount);
            }
            pImage->mPalette = new APalette(s_palette.mEntries, s_palette.mEnd);
            fread(&s_cByte, 1, 1, mFile);
            if (s_cByte == kEndOfFile) {
                return kAGfxFileBadFormat;
            }
            if (DecodeLzwImage(mFile, s_cByte, static_cast<unsigned char *>(pImage->mPixels)) ==
                0) {
                return kAGfxFileBadFormat;
            }
            if ((s_graphicControl.mFlags & kTransparentFlag) != 0) {
                pImage->mHasTransparentColor = 1;
                pImage->mTransparentColor = s_graphicControl.mTransparentIndex;
            }
            return kAGfxFileOk;
        }
        if (cIntroducer != kIntroducerExtension) {
            if (cIntroducer != kIntroducerOddExtension) {
                return kAGfxFileBadFormat;
            }
            fread(&s_cByte, 1, 1, mFile);
            if (s_cByte != kIntroducerOddExtension) {
                return kAGfxFileBadFormat;
            }
            fread(&s_cByte, 1, 1, mFile);
            if (s_cByte != 0) {
                return kAGfxFileBadFormat;
            }
            fread(&s_cByte, 1, 1, mFile);
            if (s_cByte != kIntroducerExtension) {
                return kAGfxFileBadFormat;
            }
        }
        ReadExtensionBlock();
    }
}

// 0x0062b6f0
int AGifFile::Write([[maybe_unused]] const ABitmap &bitmap) {
    return kAGfxFileUnsupported;
}

// 0x0062ae10
void AGifFile::ReadExtensionBlock() {
    unsigned char nLabel;
    fread(&nLabel, 1, 1, mFile);
    int nSkip;
    int bSubBlocks;
    unsigned char nDiscard;
    switch (nLabel) {
    case kLabelGraphicControl: {
        GifGraphicControl control;
        fread(&control, 1, kGraphicControlSize, mFile);
        s_graphicControl.mBlockSize = control.mBlockSize;
        s_graphicControl.mFlags = control.mFlags;
        s_graphicControl.mDelay = control.mDelay;
        s_graphicControl.mTransparentIndex = control.mTransparentIndex;
        fread(&nDiscard, 1, 1, mFile);
        mDuration += s_graphicControl.mDelay * kMsPerDelayUnit;
        return;
    }
    case kLabelPlainText:
        nSkip = kPlainTextHeaderSize;
        bSubBlocks = 1;
        break;
    case kLabelComment:
        nSkip = 0;
        bSubBlocks = 1;
        break;
    case kLabelApplication:
        nSkip = kApplicationHeaderSize;
        bSubBlocks = 1;
        break;
    default: {
        signed char cLength;
        fread(&cLength, 1, 1, mFile);
        nSkip = 0;
        bSubBlocks = 0;
        for (int i = cLength; i > 0; --i) {
            fread(&nDiscard, 1, 1, mFile);
        }
        break;
    }
    }
    for (int i = nSkip; i != 0; --i) {
        fread(&nDiscard, 1, 1, mFile);
    }
    if (bSubBlocks == 1) {
        signed char cLength;
        do {
            fread(&cLength, 1, 1, mFile);
            for (int i = cLength; i > 0; --i) {
                fread(&nDiscard, 1, 1, mFile);
            }
        } while (cLength > 0);
    }
}

// 0x0062b008
int AGifFile::DecodeLzwImage(FILE *pFile, int nCodeSize, unsigned char *pDest) {
    unsigned char abBlock[kSubBlockSize];
    unsigned char *pByte = abBlock;
    unsigned char *pEnd = abBlock;
    short nRow = 0;
    short nColumn = 0;
    int nPass = 0;
    int nBitsUsed = kBitsPerByte;
    if (nCodeSize < kMinCodeSize || nCodeSize > kMaxMinCodeSize) {
        return 0;
    }
    const int nInitialSize = nCodeSize + 1;
    const short nClear = static_cast<short>(1 << nCodeSize);
    short nCurrentSize = static_cast<short>(nInitialSize);
    short nMaxCode = static_cast<short>(1 << nCurrentSize);
    int nRowsLeft = s_image.mHeight;
    short nNextCode = static_cast<short>(nClear + 2);
    short nFirstChar = kNoCode;
    short nOldCode = kNoCode;

    for (;;) {
        if (nBitsUsed == kBitsPerByte) {
            if (!NextDataByte(pFile, abBlock, &pByte, &pEnd)) {
                return 0;
            }
            nBitsUsed = 0;
        }
        const short nBits = static_cast<short>(nCurrentSize + nBitsUsed);
        short nCode = *pByte;
        if (nBits <= kBitsPerByte) {
            *pByte = static_cast<unsigned char>(*pByte >> nCurrentSize);
            nBitsUsed = nBits;
        } else {
            if (!NextDataByte(pFile, abBlock, &pByte, &pEnd)) {
                return 0;
            }
            nCode = static_cast<short>(nCode | (*pByte << (kBitsPerByte - nBitsUsed)));
            if (nBits <= kBitsPerHalfword) {
                nBitsUsed = nBits - kBitsPerByte;
                *pByte = static_cast<unsigned char>(*pByte >> nBitsUsed);
            } else {
                if (!NextDataByte(pFile, abBlock, &pByte, &pEnd)) {
                    return 0;
                }
                nCode = static_cast<short>(nCode | (*pByte << (kBitsPerHalfword - nBitsUsed)));
                nBitsUsed = nBits - kBitsPerHalfword;
                *pByte = static_cast<unsigned char>(*pByte >> nBitsUsed);
            }
        }
        nCode = static_cast<short>(nCode & kCodeMasks[nCurrentSize]);

        if (nCode == nClear + 1) {
            return 1;
        }
        if (nCode > nNextCode) {
            return 0;
        }
        if (nCode == nClear) {
            nCurrentSize = static_cast<short>(nInitialSize);
            nNextCode = static_cast<short>(nCode + 2);
            nMaxCode = static_cast<short>(1 << nCurrentSize);
            nOldCode = kNoCode;
            nFirstChar = kNoCode;
            continue;
        }

        unsigned char *pStack = s_abStack;
        short nChar = nCode;
        if (nCode == nNextCode) {
            if (nOldCode == kNoCode) {
                return 0;
            }
            *pStack++ = static_cast<unsigned char>(nFirstChar);
            nChar = nOldCode;
        }
        while (nChar >= nClear) {
            *pStack++ = s_abSuffix[nChar];
            nChar = s_anPrefix[nChar];
        }
        nFirstChar = nChar;

        for (;;) {
            s_abLine[nColumn] = static_cast<unsigned char>(nChar);
            nColumn = static_cast<short>(nColumn + 1);
            if (nColumn >= s_image.mWidth) {
                if (nRow < s_image.mHeight) {
                    memcpy(
                        pDest + static_cast<long>(nRow) * s_image.mWidth, s_abLine, s_image.mWidth);
                }
                nColumn = 0;
                if ((s_image.mFlags & kInterlaceFlag) != 0) {
                    nRow = static_cast<short>(nRow + kInterlaceStep[nPass]);
                    if (nRow >= s_image.mHeight) {
                        ++nPass;
                        nRow = static_cast<short>(kInterlaceStart[nPass]);
                    }
                } else {
                    nRow = static_cast<short>(nRow + 1);
                }
                if (--nRowsLeft <= 0) {
                    return 1;
                }
            }
            if (pStack <= s_abStack) {
                break;
            }
            nChar = *--pStack;
        }

        if (nNextCode < kMaxCodeCount && nOldCode != kNoCode) {
            s_anPrefix[nNextCode] = nOldCode;
            s_abSuffix[nNextCode] = static_cast<unsigned char>(nFirstChar);
            nNextCode = static_cast<short>(nNextCode + 1);
            if (nNextCode >= nMaxCode && nCurrentSize < kMaxCodeSize) {
                nCurrentSize = static_cast<short>(nCurrentSize + 1);
                nMaxCode = static_cast<short>(1 << nCurrentSize);
            }
        }
        nOldCode = nCode;
    }
}
