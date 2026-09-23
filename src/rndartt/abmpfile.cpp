#include "rndartt/abmpfile.h"

#include <string.h>

#include "rndartt/abitmap.h"
#include "rndartt/apalette.h"

namespace {

// The file header as the file lays it out, with the size at offset 2.
struct __attribute__((packed)) BmpFileHeader {
    unsigned short mType;
    unsigned int mFileSize;
    unsigned short mReserved1;
    unsigned short mReserved2;
    unsigned int mPixelOffset;
};

struct BmpInfoHeader {
    unsigned int mSize;
    int mWidth;
    int mHeight;
    short mPlanes;
    unsigned short mBitCount;
    unsigned int mCompression;
    unsigned int mImageSize;
    int mXPixelsPerMeter;
    int mYPixelsPerMeter;
    unsigned int mColorsUsed;
    unsigned int mColorsImportant;
};

// Both headers with the file header unpacked, as ReadHeader() and Write() build them on the stack.
struct BmpHeader {
    unsigned short mType;
    unsigned int mFileSize;
    unsigned short mReserved1;
    unsigned short mReserved2;
    unsigned int mPixelOffset;
    BmpInfoHeader mInfo;
};

constexpr unsigned short kBmpType = 0x4d42; // "BM" read as a little endian halfword.
constexpr int kFileHeaderSize = 14;
constexpr unsigned int kInfoHeaderSize = 40;
constexpr short kPlaneCount = 1;
constexpr int kUnsupportedBitCount = 1;
constexpr int kBitCount4 = 4;
constexpr int kBitCount8 = 8;
constexpr int kBitCount16 = 16;
constexpr int kBitCount24 = 24;
constexpr int kMaxIndexedBitCount = 8;
constexpr int kTopDownRowStep = 1;
constexpr int kBottomUpRowStep = -1;
constexpr unsigned int kCompressionNone = 0;
constexpr int kPixelsPerMeter = 0xb89; // 75 dots per inch.
constexpr int kPaletteEntryCount = 256;
constexpr int kPaletteEntrySize = 4;
constexpr int kRowAlignment = 4;
constexpr int kPixelOffsetDirect = kFileHeaderSize + kInfoHeaderSize;
constexpr int kPixelOffsetIndexed = kPixelOffsetDirect + kPaletteEntryCount * kPaletteEntrySize;

constexpr int kRGBByteCount = 3;
constexpr int kRGBAByteCount = 4;
constexpr unsigned char kAlphaOpaque = 0xff;
constexpr unsigned int kAlphaOpaqueWord = 0xff000000;
constexpr int kGreenShift = 8;
constexpr int kBlueShift = 16;
constexpr unsigned int kLowNibbleMask = 0x0f;
constexpr unsigned int kHighNibbleMask = 0xf0;
constexpr int kNibbleShift = 4;

// Run length encoding escapes, the value byte of a pair whose count byte is zero.
constexpr unsigned char kRleEndOfLine = 0;
constexpr unsigned char kRleEndOfBitmap = 1;
constexpr unsigned char kRleDelta = 2;

// Stride masks the reader applies to rows rounded up to four bytes. The four bit mask is a
// halfword mask because the binary rounds at halfword width.
constexpr int kRow4Mask = 0xfff8;
constexpr int kRowWordMask = 0xfffc;

// 0x008e68e0
BmpFileHeader s_readFileHeader;

// 0x008e68f0
BmpInfoHeader s_readInfoHeader;

// 0x008e6918
BmpFileHeader s_writeFileHeader;

// 0x008e6928
BmpInfoHeader s_writeInfoHeader;

// 0x007a5768
// the zero bytes each written row is padded with.
const unsigned char kRowPadding[kRowAlignment] = {0, 0, 0, 0};

inline unsigned char SwapNibbles(unsigned char nByte) {
    return static_cast<unsigned char>((nByte >> kNibbleShift) | (nByte << kNibbleShift));
}

} // namespace

// 0x0061c688
int ABmpFile::ReadHeader() {
    fread(&s_readFileHeader, 1, kFileHeaderSize, mFile);
    fread(&s_readInfoHeader, 1, kInfoHeaderSize, mFile);
    BmpHeader header;
    header.mType = s_readFileHeader.mType;
    header.mFileSize = s_readFileHeader.mFileSize;
    header.mPixelOffset = s_readFileHeader.mPixelOffset;
    header.mInfo = s_readInfoHeader;
    if (header.mType != kBmpType || header.mInfo.mSize != kInfoHeaderSize ||
        header.mInfo.mPlanes != kPlaneCount || header.mInfo.mBitCount == kUnsupportedBitCount) {
        return kAGfxFileBadFormat;
    }

    mHeight = header.mInfo.mHeight;
    mRowStep = kBottomUpRowStep;
    if (mHeight < 0) {
        mRowStep = kTopDownRowStep;
        mHeight = -mHeight;
    }
    mWidth = header.mInfo.mWidth;
    mPixelOffset = header.mPixelOffset;
    mPaletteOffset = header.mInfo.mSize + kFileHeaderSize;
    mCompression = header.mInfo.mCompression;
    mBitCount = header.mInfo.mBitCount;
    mImageRead = 0;
    if (mBitCount == kBitCount4 || mBitCount == kBitCount8) {
        mColorCount = header.mInfo.mColorsUsed;
        if (mColorCount == 0) {
            mColorCount = 1 << mBitCount;
        }
    } else {
        mColorCount = 0;
    }
    // Yes, the bounds take the height as the file records it, sign included.
    mBounds.mRight = static_cast<short>(header.mInfo.mWidth);
    mBounds.mBottom = static_cast<short>(header.mInfo.mHeight);
    mBounds.mLeft = 0;
    mBounds.mTop = 0;
    return kAGfxFileOk;
}

// 0x0061c860
int ABmpFile::ReadImage(ABitmap *pImage, int *pbEnd) {
    if (mImageRead != 0) {
        *pbEnd = 1;
        return kAGfxFileOk;
    }
    APalette *pPalette = nullptr;
    if (mBitCount <= kMaxIndexedBitCount) {
        pPalette = ReadPalette();
    }
    switch (mBitCount) {
    case kBitCount4:
        *pImage = ABitmap(nullptr,
                          kABitmapFormatLinear4,
                          false,
                          mWidth,
                          mHeight,
                          ((mWidth + 7) & kRow4Mask) >> 1);
        break;
    case kBitCount8:
        *pImage = ABitmap(
            nullptr, kABitmapFormatLinear8, false, mWidth, mHeight, (mWidth + 3) & kRowWordMask);
        break;
    case kBitCount16:
        *pImage = ABitmap(nullptr,
                          kABitmapFormatLinear15,
                          false,
                          mWidth,
                          mHeight,
                          (mWidth * 2 + 3) & kRowWordMask);
        break;
    case kBitCount24:
        *pImage = ABitmap(nullptr,
                          kABitmapFormatLinear32,
                          false,
                          mWidth,
                          mHeight,
                          (mWidth * kRGBAByteCount) & kRowWordMask);
        break;
    default:
        return kAGfxFileBadFormat; // Yes, the palette read above is not released.
    }
    if (pImage->mPixels == nullptr) {
        if (pPalette != nullptr) {
            delete pPalette;
        }
        return kAGfxFileNoMemory;
    }
    const int nResult = ReadPixels(pImage);
    if (nResult != kAGfxFileOk) {
        if (pImage->mPixels != nullptr) {
            // Yes, the single-object release, although the tagged allocator made the block.
            delete static_cast<unsigned char *>(pImage->mPixels);
            pImage->mPixels = nullptr;
        }
        if (pPalette != nullptr) {
            delete pPalette;
        }
        return nResult;
    }
    pImage->mPalette = pPalette;
    mImageRead = 1;
    return kAGfxFileOk;
}

// 0x0061ca78
int ABmpFile::Write(const ABitmap &bitmap) {
    BmpHeader header;
    header.mType = kBmpType;
    header.mPixelOffset = kPixelOffsetDirect;
    if (bitmap.mFormat == kABitmapFormatLinear8) {
        header.mPixelOffset = kPixelOffsetIndexed;
    }
    header.mReserved1 = 0;
    header.mReserved2 = 0;
    header.mFileSize = header.mPixelOffset + bitmap.mHeight * bitmap.mBytesPerRow;
    s_writeFileHeader.mType = header.mType;
    s_writeFileHeader.mFileSize = header.mFileSize;
    s_writeFileHeader.mPixelOffset = header.mPixelOffset;

    header.mInfo.mSize = kInfoHeaderSize;
    header.mInfo.mWidth = bitmap.mWidth;
    header.mInfo.mHeight = bitmap.mHeight;
    header.mInfo.mPlanes = kPlaneCount;
    switch (bitmap.mFormat) {
    case kABitmapFormatLinear8:
        header.mInfo.mBitCount = kBitCount8;
        break;
    case kABitmapFormatLinear15:
        header.mInfo.mBitCount = kBitCount16;
        break;
    case kABitmapFormatLinear24:
        header.mInfo.mBitCount = kBitCount24;
        break;
    default:
        return kAGfxFileBadFormat;
    }
    header.mInfo.mCompression = kCompressionNone;
    header.mInfo.mImageSize = bitmap.mHeight * bitmap.mBytesPerRow;
    header.mInfo.mXPixelsPerMeter = kPixelsPerMeter;
    header.mInfo.mYPixelsPerMeter = kPixelsPerMeter;
    header.mInfo.mColorsUsed = bitmap.mFormat == kABitmapFormatLinear8 ? kPaletteEntryCount : 0;
    header.mInfo.mColorsImportant = header.mInfo.mColorsUsed;
    s_writeInfoHeader = header.mInfo;
    fwrite(&s_writeFileHeader, 1, kFileHeaderSize, mFile);
    fwrite(&s_writeInfoHeader, 1, kInfoHeaderSize, mFile);

    if (bitmap.mFormat == kABitmapFormatLinear8) {
        unsigned int aEntries[kPaletteEntryCount];
        if (bitmap.mPalette != nullptr) {
            memcpy(aEntries, bitmap.mPalette->mEntries, sizeof(aEntries));
        } else {
            memset(aEntries, 0, sizeof(aEntries));
        }
        unsigned char abQuad[kPaletteEntrySize];
        abQuad[3] = 0;
        for (int i = 0; i < kPaletteEntryCount; ++i) {
            abQuad[2] = static_cast<unsigned char>(aEntries[i]);
            abQuad[1] = static_cast<unsigned char>(aEntries[i] >> kGreenShift);
            abQuad[0] = static_cast<unsigned char>(aEntries[i] >> kBlueShift);
            fwrite(abQuad, 1, kPaletteEntrySize, mFile);
        }
    }

    const int nPadding = -bitmap.mBytesPerRow & (kRowAlignment - 1);
    const unsigned char *pRow = static_cast<const unsigned char *>(bitmap.mPixels) +
                                (bitmap.mHeight - 1) * bitmap.mBytesPerRow;
    for (int y = bitmap.mHeight - 1; y >= 0; --y) {
        fwrite(pRow, 1, bitmap.mBytesPerRow, mFile);
        if (nPadding != 0) {
            fwrite(kRowPadding, 1, nPadding, mFile);
        }
        pRow -= bitmap.mBytesPerRow;
    }
    return kAGfxFileOk;
}

// 0x0061cde0
APalette *ABmpFile::ReadPalette() {
    unsigned int aEntries[kPaletteEntryCount];
    memset(aEntries, 0, sizeof(aEntries));
    fseek(mFile, mPaletteOffset, SEEK_SET);
    for (int i = 0; i < mColorCount; ++i) {
        unsigned char abQuad[kPaletteEntrySize];
        fread(abQuad, 1, kPaletteEntrySize, mFile);
        aEntries[i] =
            abQuad[2] | (abQuad[1] << kGreenShift) | (abQuad[0] << kBlueShift) | kAlphaOpaqueWord;
    }
    return new APalette(aEntries, mColorCount);
}

// 0x0061d5c0
int ABmpFile::ReadPixels(ABitmap *pImage) {
    fseek(mFile, mPixelOffset, SEEK_SET);
    if (mCompression == kCompressionNone) {
        return ReadUncompressedPixels(pImage);
    }
    return ReadRlePixels(pImage);
}

// 0x0061cef8
int ABmpFile::ReadUncompressedPixels(ABitmap *pImage) {
    unsigned char *pRow = static_cast<unsigned char *>(pImage->mPixels);
    int nStride = pImage->mBytesPerRow;
    int nFirstRow = 0;
    int nEndRow = mHeight;
    if (mRowStep < 0) {
        pRow += (mHeight - 1) * pImage->mBytesPerRow;
        nStride = -nStride;
        nFirstRow = mHeight;
        nEndRow = 0;
    }
    for (int y = nFirstRow; y != nEndRow; y += mRowStep) {
        if (mBitCount == kBitCount4) {
            unsigned char *pByte = pRow;
            for (int i = 0; i < pImage->mBytesPerRow * 2; i += 2) {
                unsigned char nByte;
                fread(&nByte, 1, 1, mFile);
                *pByte++ = SwapNibbles(nByte);
            }
        } else if (pImage->mFormat == kABitmapFormatLinear32) {
            fread(pRow, 1, pImage->mWidth * kRGBByteCount, mFile);
            ABitmap::SwapRedBlue24(pRow, pImage->mWidth);
            ExpandRow24To32(pRow, pImage->mWidth);
        } else {
            fread(pRow, 1, pImage->mBytesPerRow, mFile);
            if (pImage->mFormat == kABitmapFormatLinear15) {
                ABitmap::SwapRedBlue15(static_cast<unsigned short *>(static_cast<void *>(pRow)),
                                       pImage->mWidth);
            } else if (pImage->mFormat == kABitmapFormatLinear24) {
                ABitmap::SwapRedBlue24(pRow, pImage->mWidth);
            }
        }
        pRow += nStride;
    }
    return kAGfxFileOk;
}

// 0x0061d0e8
int ABmpFile::ReadRlePixels(ABitmap *pImage) {
    unsigned char *pRow = static_cast<unsigned char *>(pImage->mPixels);
    int nStride = pImage->mBytesPerRow;
    memset(pRow, 0, pImage->mByteCount);
    int y = 0;
    int nEndRow = mHeight;
    if (mRowStep < 0) {
        nStride = -nStride;
        y = mHeight;
        pRow += (mHeight - 1) * pImage->mBytesPerRow;
        nEndRow = 0;
    }
    while (y != nEndRow) {
        unsigned char *pDest = pRow;
        int nColumn = 0;
        for (;;) {
            unsigned char abPair[2];
            fread(&abPair[0], 1, 1, mFile);
            fread(&abPair[1], 1, 1, mFile);
            if (abPair[0] != 0) {
                if (mBitCount == kBitCount8) {
                    memset(pDest, abPair[1], abPair[0]);
                    nColumn += abPair[0];
                    pDest += abPair[0];
                } else {
                    for (int i = 0; i < abPair[0]; ++i) {
                        if ((nColumn & 1) != 0) {
                            *pDest = static_cast<unsigned char>((*pDest & kLowNibbleMask) |
                                                                (abPair[1] << kNibbleShift));
                            ++pDest;
                        } else {
                            *pDest = static_cast<unsigned char>((*pDest & kHighNibbleMask) |
                                                                (abPair[1] >> kNibbleShift));
                        }
                        ++nColumn;
                    }
                }
            } else if (abPair[1] == kRleEndOfBitmap) {
                return kAGfxFileOk;
            } else if (abPair[1] == kRleEndOfLine) {
                break;
            } else if (abPair[1] == kRleDelta) {
                unsigned char abDelta[2];
                fread(&abDelta[0], 1, 1, mFile);
                fread(&abDelta[1], 1, 1, mFile);
                pDest += abDelta[0];
                nColumn += abDelta[0];
                y += abDelta[1] * mRowStep;
                pRow += static_cast<long long>(abDelta[1]) * nStride;
                pDest += static_cast<long long>(abDelta[1]) * nStride;
            } else if (mBitCount == kBitCount8) {
                fread(pDest, 1, abPair[1], mFile);
                pDest += abPair[1];
                nColumn += abPair[1];
                if ((abPair[1] & 1) != 0) {
                    unsigned char nPad;
                    fread(&nPad, 1, 1, mFile);
                }
            } else {
                int nPairs = abPair[1] / 2;
                while (nPairs-- != 0) {
                    unsigned char nByte;
                    fread(&nByte, 1, 1, mFile);
                    *pDest++ = SwapNibbles(nByte);
                }
                // Yes, the column advances by the exhausted counter, which is -1 here, so it moves
                // back by two. See ACanvasLin4::WriteIndexedRow() for the same pattern.
                nColumn += nPairs * 2;
                if ((abPair[1] & 1) != 0) {
                    unsigned char nByte;
                    fread(&nByte, 1, 1, mFile);
                    if ((nColumn & 1) != 0) {
                        *pDest = static_cast<unsigned char>((*pDest & kLowNibbleMask) |
                                                            (nByte << kNibbleShift));
                        ++pDest;
                    } else {
                        *pDest = static_cast<unsigned char>((*pDest & kHighNibbleMask) |
                                                            (nByte >> kNibbleShift));
                    }
                    ++nColumn;
                }
                // The run pads to a whole halfword.
                if ((abPair[1] & 3) == 1 || (abPair[1] & 3) == 2) {
                    unsigned char nPad;
                    fread(&nPad, 1, 1, mFile);
                }
            }
            if (pImage->mBytesPerRow < pDest - pRow) {
                return kAGfxFileBadFormat;
            }
        }
        pRow += nStride;
        y += mRowStep;
    }
    return kAGfxFileOk;
}

// 0x0061d620
void ABmpFile::ExpandRow24To32(unsigned char *pPixels, int nCount) {
    const unsigned char *pSource = pPixels + (nCount - 1) * kRGBByteCount;
    unsigned char *pDest = pPixels + (nCount - 1) * kRGBAByteCount;
    while (pSource >= pPixels) {
        pDest[0] = pSource[0];
        pDest[1] = pSource[1];
        pDest[3] = kAlphaOpaque;
        pDest[2] = pSource[2];
        pSource -= kRGBByteCount;
        pDest -= kRGBAByteCount;
    }
}
