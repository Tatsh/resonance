#include "rndartt/atgafile.h"

#include "rndartt/abitmap.h"

namespace {

struct TgaHeader {
    unsigned char mIdLength;
    unsigned char mColorMapType;
    unsigned char mImageType;
    unsigned char mColorMapSpec[5];
    unsigned short mXOrigin;
    unsigned short mYOrigin;
    unsigned short mWidth;
    unsigned short mHeight;
    unsigned char mPixelDepth;
    unsigned char mDescriptor;
};

constexpr int kHeaderSize = 18;
constexpr unsigned char kImageTypeTrueColor = 2;
constexpr unsigned char kImageTypeRleTrueColor = 10;
constexpr unsigned char kDescriptorOriginMask = 0x30;
constexpr int kPixelDepth32 = 32;
constexpr int kRGBByteCount = 3;
constexpr int kRGBAByteCount = 4;
constexpr unsigned char kAlphaOpaque = 0xff;
constexpr unsigned char kPacketRunFlag = 0x80;
constexpr unsigned char kPacketCountMask = 0x7f;

// 0x007b8100, the one pixel a run length encoded packet is read into.
unsigned char s_abPixel[kRGBAByteCount];

} // namespace

// 0x0061fff8
int ATgaFile::ReadHeader() {
    TgaHeader header;
    fread(&header, 1, kHeaderSize, mFile);
    if (header.mColorMapType != 0) {
        return kAGfxFileBadFormat;
    }
    if (header.mImageType != kImageTypeRleTrueColor && header.mImageType != kImageTypeTrueColor) {
        return kAGfxFileBadFormat;
    }
    if (header.mIdLength != 0) {
        fseek(mFile, header.mIdLength, SEEK_CUR);
    }
    mTopDown = (header.mDescriptor & kDescriptorOriginMask) != 0;
    mRle = header.mImageType == kImageTypeRleTrueColor;
    mPixelDepth = header.mPixelDepth;
    mWidth = header.mWidth;
    mHeight = header.mHeight;
    mBounds.mRight = static_cast<short>(header.mWidth);
    mBounds.mBottom = static_cast<short>(header.mHeight);
    mImageRead = 0;
    mBounds.mLeft = 0;
    mBounds.mTop = 0;
    return kAGfxFileOk;
}

// 0x00620508
int ATgaFile::ReadImage(ABitmap *pImage, int *pbEnd) {
    if (mImageRead != 0) {
        *pbEnd = 1;
        return kAGfxFileOk;
    }
    *pImage =
        ABitmap(nullptr, kABitmapFormatLinear32, false, mWidth, mHeight, mWidth * kRGBAByteCount);
    if (pImage->mPixels == nullptr) {
        return kAGfxFileNoMemory;
    }
    const int nResult = ReadPixels(pImage);
    if (nResult != kAGfxFileOk && pImage->mPixels != nullptr) {
        // Yes, the single-object release, although the tagged allocator made the block.
        delete static_cast<unsigned char *>(pImage->mPixels);
        pImage->mPixels = nullptr;
    }
    return nResult;
}

// 0x00620500
int ATgaFile::Write([[maybe_unused]] const ABitmap &bitmap) {
    return kAGfxFileUnsupported;
}

// 0x00620128
int ATgaFile::ReadPixels(ABitmap *pImage) {
    unsigned char *pPixels = static_cast<unsigned char *>(pImage->mPixels);
    for (int y = 0; y < pImage->mHeight; ++y) {
        const int nRow = mTopDown != 0 ? y : pImage->mHeight - (y + 1);
        unsigned char *pDest = pPixels + nRow * pImage->mBytesPerRow;
        const int nPixelBytes = mPixelDepth == kPixelDepth32 ? kRGBAByteCount : kRGBByteCount;
        if (mRle != 0) {
            for (int x = 0; x < pImage->mWidth;) {
                unsigned char nPacket;
                fread(&nPacket, 1, 1, mFile);
                unsigned short nCount = (nPacket & kPacketCountMask) + 1;
                x += nCount;
                if ((nPacket & kPacketRunFlag) != 0) {
                    fread(s_abPixel, 1, nPixelBytes, mFile);
                    const unsigned char nFirst = s_abPixel[0];
                    s_abPixel[0] = s_abPixel[2];
                    s_abPixel[2] = nFirst;
                    while (nCount-- != 0) {
                        pDest[0] = s_abPixel[0];
                        pDest[1] = s_abPixel[1];
                        pDest[2] = s_abPixel[2];
                        pDest[3] = s_abPixel[3];
                        pDest += kRGBAByteCount;
                    }
                } else {
                    while (nCount-- != 0) {
                        fread(s_abPixel, 1, nPixelBytes, mFile);
                        pDest[0] = s_abPixel[2];
                        pDest[1] = s_abPixel[1];
                        pDest[2] = s_abPixel[0];
                        pDest[3] = s_abPixel[3];
                        pDest += kRGBAByteCount;
                    }
                }
            }
        } else if (mPixelDepth == kPixelDepth32) {
            fread(pDest, 1, pImage->mWidth * kRGBAByteCount, mFile);
            for (int nRemaining = pImage->mWidth; nRemaining > 0; --nRemaining) {
                const unsigned char nFirst = pDest[0];
                pDest[0] = pDest[2];
                pDest[2] = nFirst;
                pDest += kRGBAByteCount;
            }
        } else {
            for (int nRemaining = pImage->mWidth; nRemaining > 0; --nRemaining) {
                fread(pDest, 1, kRGBByteCount, mFile);
                const unsigned char nFirst = pDest[0];
                pDest[0] = pDest[2];
                pDest[2] = nFirst;
                pDest[3] = kAlphaOpaque;
                pDest += kRGBAByteCount;
            }
        }
    }
    return kAGfxFileOk;
}
