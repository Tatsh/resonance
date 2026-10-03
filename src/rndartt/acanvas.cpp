#include "rndartt/acanvas.h"

#include <stdlib.h>
#include <string.h>

#include "math/color.h"
#include "os/mem.h"
#include "rndartt/acanvaslin15.h"
#include "rndartt/acanvaslin24.h"
#include "rndartt/acanvaslin32.h"
#include "rndartt/acanvaslin4.h"
#include "rndartt/acanvaslin8.h"
#include "rndartt/aclipspan.h"
#include "rndartt/afix.h"
#include "rndartt/afont.h"
#include "rndartt/apalette.h"
#include "rndartt/apoint.h"
#include "rndartt/apolygon.h"
#include "rndartt/apolygonedge.h"
#include "rndartt/arowspan.h"
#include "rndartt/astretchblit.h"
#include "rndartt/normalkey.h"

namespace {

constexpr unsigned int kChannelMask = 0xff;
constexpr int kGreenShift = 8;
constexpr int kBlueShift = 16;
constexpr int kNibbleBits = 4;
constexpr unsigned int kNibbleMask = 0x0f;
constexpr int kRGBByteCount = 3;
constexpr char kNewline = '\n';
constexpr int kFixedOne = 1 << kACanvasFractionBits;

// The two directions a polygon edge walks through the vertex list.
constexpr int kEdgeForward = 1;
constexpr int kEdgeBackward = -1;

// One pointer to member per ABitmapFormat code. DrawCharU() and DrawChar() both index the
// first table by the glyph format code, DrawBitmapU() and DrawBitmap() the first two by the source
// format code, GetBitmapU() and GetBitmap() the next two by the destination format code, and
// DrawScaledBitmap() the last by the source format code. The last three address protected members.
// Each is therefore a static inside the member that indexes it.
typedef void (ACanvas::*ABitmapCopyMember)(const ABitmap &, int, int);
typedef void (ACanvas::*ABitmapStretchMember)(const ABitmap &, const ARect &);

// NTSC-U/C: 0x0077dc98, PAL: 0x007c1a88
const ABitmapCopyMember pfDrawBitmapU[kABitmapFormatCount] = {&ACanvas::DrawBitmapLin4U,
                                                              &ACanvas::DrawBitmapLin8U,
                                                              &ACanvas::DrawBitmapLin15U,
                                                              &ACanvas::DrawBitmapLin24U,
                                                              &ACanvas::DrawBitmapLin32U,
                                                              &ACanvas::DrawBitmapRle8U};

// NTSC-U/C: 0x0077dcc8, PAL: 0x007c1ab8
const ABitmapCopyMember kCopyForFormat[kABitmapFormatCount] = {&ACanvas::DrawBitmapLin4,
                                                               &ACanvas::DrawBitmapLin8,
                                                               &ACanvas::DrawBitmapLin15,
                                                               &ACanvas::DrawBitmapLin24,
                                                               &ACanvas::DrawBitmapLin32,
                                                               &ACanvas::DrawBitmapRle8};

inline const unsigned char *SourceRow(const ABitmap &bitmap) {
    return static_cast<const unsigned char *>(bitmap.mPixels);
}

inline unsigned char *DestRow(const ABitmap &bitmap) {
    return static_cast<unsigned char *>(bitmap.mPixels);
}

inline const ABitmap *GlyphForCode(const AFont *pFont, int nCharCode) {
    const int nIndex = nCharCode - pFont->mFirstCharCode;
    // Yes, the binary tests only the upper bound, so a code below mFirstCharCode indexes before
    // the array, and a code at or beyond the count yields a null the caller then reads through.
    if (nIndex < pFont->mGlyphCount) {
        return pFont->mGlyphs[nIndex];
    }
    return nullptr;
}

// QuantizeToRamps() reserves room for this many keys. Each key owns a ramp of kRampLength palette
// entries, a shade of zero maps to the shared black entry, and a pixel with zero alpha to entry 0.
constexpr int kQuantizeReservedKeys = 16;
constexpr int kRampLength = 16;
constexpr int kRampLastShade = kRampLength - 1;
constexpr unsigned char kRampBlackIndex = 16;
constexpr unsigned char kTransparentIndex = 0;
constexpr unsigned int kAlphaMask = 0xff000000;
constexpr float kShadeScale = 1.0f / 17.0f;
constexpr float kRoundHalf = 0.5f;

// Advance a stretched copy by one destination row and return the source row it then samples.
inline int AdvanceStretchRow(ACanvas::AScaledRowInfo *pBlit) {
    pBlit->mSourcePositionY += pBlit->mSourceStepY;
    return pBlit->mSourcePositionY >> kACanvasFractionBits;
}

} // namespace

// NTSC-U/C: 0x0086f6f0, PAL: 0x008b3dd0
APalette *ACanvas::palDefault = nullptr;

// NTSC-U/C: 0x00557af8, PAL: 0x00598c50
void ACanvas::QuantizeToRamps(ACanvas &dest, const std::vector<const Color *> &colors) const {
    std::vector<NormalKey> keys;
    keys.reserve(kQuantizeReservedKeys);
    for (const auto pColor : colors) {
        NormalKey::InsertUniqueNormalKey(keys, *pColor);
    }
    APalette::BuildRampPalette(keys, *dest.mBitmap.mPalette);

    int nPixels = mBitmap.mWidth * mBitmap.mHeight;
    unsigned char *pOut = DestRow(dest.mBitmap);
    const unsigned int *pIn = static_cast<const unsigned int *>(mBitmap.mPixels);
    if (keys.empty()) {
        memset(pOut, 0, nPixels);
        return;
    }
    for (; nPixels != 0; --nPixels) {
        const unsigned int nPixel = *pIn++;
        if ((nPixel & kAlphaMask) == 0) {
            *pOut++ = kTransparentIndex;
            continue;
        }
        const NormalKey key(static_cast<float>(nPixel & kChannelMask),
                            static_cast<float>((nPixel >> kGreenShift) & kChannelMask),
                            static_cast<float>((nPixel >> kBlueShift) & kChannelMask));
        auto nearest = keys.cbegin();
        float flBest = nearest->RatioDistance(key);
        for (auto it = nearest + 1; it != keys.cend(); ++it) {
            const float flDistance = it->RatioDistance(key);
            if (flDistance < flBest) {
                flBest = flDistance;
                nearest = it;
            }
        }
        const int nShade =
            static_cast<int>(key.mScale / nearest->mScale * kShadeScale + kRoundHalf);
        const int nRampBase = static_cast<int>(nearest - keys.cbegin()) * kRampLength;
        if (nShade == 0) {
            *pOut++ = kRampBlackIndex;
        } else {
            *pOut++ = nRampBase + (nShade < kRampLength ? nShade : kRampLastShade);
        }
    }
}

// NTSC-U/C: 0x005eb1a0, PAL: 0x0062d2e8
ACanvas::ACanvas(const ABitmap &bitmap) : mBitmap(bitmap) {
    mClip.mLeft = 0;
    mClip.mTop = 0;
    mClip.mRight = bitmap.mWidth;
    mClip.mBottom = bitmap.mHeight;
}

// NTSC-U/C: 0x005e8bc8, PAL: 0x0062ad10
ACanvas *ACanvas::NewCompatibleCanvas(const ABitmap &bitmap, bool bAllocatePixels) {
    ABitmap copy = bitmap;
    if (bAllocatePixels) {
        if (copy.mFormat == kABitmapFormatLinear4) {
            copy.mBytesPerRow = static_cast<short>((copy.mWidth + 2) / 2);
        } else {
            copy.mBytesPerRow =
                static_cast<short>(copy.mWidth * ABitmap::bmPixelSize[copy.mFormat]);
        }
        copy.mPixels = MemAllocTagged(
            static_cast<long long>(copy.mHeight) * copy.mBytesPerRow, __FILE__, __LINE__);
        if (copy.mPixels == nullptr) {
            return nullptr;
        }
    }
    switch (bitmap.mFormat) {
    case kABitmapFormatLinear4:
        return new ACanvasLin4(copy);
    case kABitmapFormatLinear8:
    case kABitmapFormatRle8:
        return new ACanvasLin8(copy);
    case kABitmapFormatLinear15:
        return new ACanvasLin15(copy);
    case kABitmapFormatLinear24:
        return new ACanvasLin24(copy);
    case kABitmapFormatLinear32:
        return new ACanvasLin32(copy);
    default:
        return nullptr;
    }
}

// NTSC-U/C: 0x005e8e38, PAL: 0x0062af80
ACanvas *ACanvas::SubCanvas(const ABitmap &source, int nX, int nY, int nWidth, int nHeight) {
    ABitmap rect(source, nX, nY, nWidth, nHeight);
    switch (rect.mFormat) {
    case kABitmapFormatLinear4: // Yes, the binary gives a four bit rectangle an ACanvasLin8.
    case kABitmapFormatLinear8:
        return new ACanvasLin8(rect);
    case kABitmapFormatLinear15:
        return new ACanvasLin15(rect);
    case kABitmapFormatLinear24:
        return new ACanvasLin24(rect);
    case kABitmapFormatLinear32:
        return new ACanvasLin32(rect);
    default:
        return nullptr;
    }
}

// NTSC-U/C: 0x005eb200, PAL: 0x0062d348
ACanvas *ACanvas::NewCompatibleLinearCanvas(const ABitmap &bitmap) {
    ABitmap copy = bitmap;
    if (copy.mFormat == kABitmapFormatRle8) {
        copy.mFormat = kABitmapFormatLinear8;
    }
    return NewCompatibleCanvas(copy, true);
}

// NTSC-U/C: 0x005ead68, PAL: 0x0062ceb0
ACanvas::~ACanvas() {
}

// NTSC-U/C: 0x005eb3d0, PAL: 0x0062d518
unsigned char ACanvas::ClipCode(int nX, int nY) const {
    unsigned char nCode = 0;
    if (nX < mClip.mLeft) {
        nCode |= kACanvasClipLeft;
    }
    if (nX >= mClip.mRight) {
        nCode |= kACanvasClipRight;
    }
    if (nY < mClip.mTop) {
        nCode |= kACanvasClipAbove;
    }
    if (nY >= mClip.mBottom) {
        nCode |= kACanvasClipBelow;
    }
    return nCode;
}

// NTSC-U/C: 0x005eb418, PAL: 0x0062d560
int ACanvas::ClipRle8Bitmap(
    const ABitmap &source, int *pnX, int *pnY, ARle8Reader *pReader, Rle8Clip *pSpan) const {
    pSpan->mStopColumn = source.mWidth;
    if (mClip.mRight < *pnX + source.mWidth) {
        pSpan->mStopColumn = static_cast<short>(mClip.mRight - *pnX);
    }
    pSpan->mSkipLeft = 0;
    if (*pnX < mClip.mLeft) {
        pSpan->mSkipLeft = static_cast<short>(mClip.mLeft - *pnX);
        *pnX = mClip.mLeft;
    }
    if (pSpan->mSkipLeft >= pSpan->mStopColumn) {
        return 0;
    }

    pSpan->mStopRow = static_cast<short>(*pnY + source.mHeight);
    if (mClip.mBottom < pSpan->mStopRow) {
        pSpan->mStopRow = mClip.mBottom;
    }
    if (*pnY < mClip.mTop) {
        pReader->SkipRow(mClip.mTop - *pnY);
        *pnY = mClip.mTop;
    }
    return *pnY < pSpan->mStopRow;
}

// NTSC-U/C: 0x005e8fd8, PAL: 0x0062b120
int ACanvas::ClipLine(int *pnX0, int *pnY0, int *pnX1, int *pnY1) const {
    unsigned char nCode0 = ClipCode(*pnX0 >> kACanvasFractionBits, *pnY0 >> kACanvasFractionBits);
    for (;;) {
        unsigned char nCode1 =
            ClipCode(*pnX1 >> kACanvasFractionBits, *pnY1 >> kACanvasFractionBits);
        if (static_cast<unsigned char>(nCode0 | nCode1) == 0) {
            return 1;
        }
        if (static_cast<unsigned char>(nCode0 & nCode1) != 0) {
            return 0;
        }
        if (nCode1 == 0) {
            const int nX = *pnX0;
            *pnX0 = *pnX1;
            *pnX1 = nX;
            const int nY = *pnY0;
            *pnY0 = *pnY1;
            *pnY1 = nY;
            nCode1 = nCode0;
            nCode0 = 0;
        }
        if ((nCode1 & (kACanvasClipLeft | kACanvasClipRight)) != 0) {
            const int nEdge = (nCode1 & kACanvasClipLeft) != 0 ?
                                  mClip.mLeft << kACanvasFractionBits :
                                  (mClip.mRight << kACanvasFractionBits) - AFix::epsilon;
            const int nSlope = ((*pnY1 - *pnY0) << kACanvasFractionBits) / (*pnX1 - *pnX0);
            *pnY1 = *pnY0 + ((nSlope * (nEdge - *pnX0)) >> kACanvasFractionBits);
            *pnX1 = nEdge;
        } else {
            const int nEdge = (nCode1 & kACanvasClipAbove) != 0 ?
                                  mClip.mTop << kACanvasFractionBits :
                                  (mClip.mBottom << kACanvasFractionBits) - AFix::epsilon;
            const int nSlope = ((*pnX1 - *pnX0) << kACanvasFractionBits) / (*pnY1 - *pnY0);
            *pnX1 = *pnX0 + ((nSlope * (nEdge - *pnY0)) >> kACanvasFractionBits);
            *pnY1 = nEdge;
        }
    }
}

// NTSC-U/C: 0x005e91b8, PAL: 0x0062b300
int ACanvas::ClipBitmap(ABitmap *pBitmap, int *pnX, int *pnY) const {
    if (*pnY < mClip.mTop) {
        pBitmap->mPixels = static_cast<unsigned char *>(pBitmap->mPixels) +
                           (mClip.mTop - *pnY) * pBitmap->mBytesPerRow;
        pBitmap->mHeight = static_cast<short>(pBitmap->mHeight - (mClip.mTop - *pnY));
        *pnY = mClip.mTop;
    }
    if (mClip.mBottom < *pnY + pBitmap->mHeight) {
        pBitmap->mHeight = static_cast<short>(mClip.mBottom - *pnY);
    }

    if (*pnX < mClip.mLeft) {
        unsigned char *pPixels = static_cast<unsigned char *>(pBitmap->mPixels);
        if (pBitmap->mFormat == kABitmapFormatLinear4) {
            // Yes, the binary advances by half the destination column rather than by half the
            // columns clipped away, and takes the new flag from this canvas's own bitmap.
            if (((mClip.mLeft - *pnX) & 1) != 0) {
                if (pBitmap->mOddNibbleStart != 0) {
                    pPixels += (*pnX + 1) / 2;
                } else {
                    pPixels += *pnX / 2;
                }
                pBitmap->mOddNibbleStart = mBitmap.mOddNibbleStart ^ 1;
            } else {
                pPixels += *pnX / 2;
            }
        } else {
            pPixels += (mClip.mLeft - *pnX) * ABitmap::bmPixelSize[pBitmap->mFormat];
        }
        pBitmap->mPixels = pPixels;
        pBitmap->mWidth = static_cast<short>(pBitmap->mWidth - (mClip.mLeft - *pnX));
        *pnX = mClip.mLeft;
    }
    if (mClip.mRight < *pnX + pBitmap->mWidth) {
        pBitmap->mWidth = static_cast<short>(mClip.mRight - *pnX);
    }
    return pBitmap->mWidth > 0 && pBitmap->mHeight > 0;
}

// NTSC-U/C: 0x005eb520, PAL: 0x0062d668
void ACanvas::DrawPixel(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        DrawPixelU(nX, nY);
    }
}

// NTSC-U/C: 0x005eb590, PAL: 0x0062d6d8
void ACanvas::DrawPixel8(int nX, int nY, int nIndex) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        DrawPixel8U(nX, nY, nIndex & kChannelMask);
    }
}

// NTSC-U/C: 0x005eb608, PAL: 0x0062d750
void ACanvas::DrawPixel15(int nX, int nY, unsigned short nColor) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        DrawPixel15U(nX, nY, nColor);
    }
}

// NTSC-U/C: 0x005eb680, PAL: 0x0062d7c8
void ACanvas::DrawPixel24(int nX, int nY, const unsigned char *pRGB) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        DrawPixel24U(nX, nY, pRGB);
    }
}

// NTSC-U/C: 0x005eb6f0, PAL: 0x0062d838
void ACanvas::DrawPixel32(int nX, int nY, unsigned int nColor) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        DrawPixel32U(nX, nY, nColor);
    }
}

// NTSC-U/C: 0x005eb760, PAL: 0x0062d8a8
void ACanvas::DrawPixelNative(int nX, int nY, unsigned int nColor) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        DrawPixelNativeU(nX, nY, nColor);
    }
}

// NTSC-U/C: 0x005eb7d0, PAL: 0x0062d918
int ACanvas::GetPixel8(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixel8U(nX, nY);
    }
    return 0;
}

// NTSC-U/C: 0x005eb848, PAL: 0x0062d990
unsigned short ACanvas::GetPixel15(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixel15U(nX, nY);
    }
    return 0;
}

// NTSC-U/C: 0x005eb8c0, PAL: 0x0062da08
void ACanvas::GetPixel24(int nX, int nY, unsigned char *pRGB) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        GetPixel24U(nX, nY, pRGB);
        return;
    }
    memset(pRGB, 0, kRGBByteCount);
}

// NTSC-U/C: 0x005eb948, PAL: 0x0062da90
unsigned int ACanvas::GetPixel32(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixel32U(nX, nY);
    }
    return 0;
}

// NTSC-U/C: 0x005eb9c0, PAL: 0x0062db08
unsigned int ACanvas::GetPixelNative(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixelNativeU(nX, nY);
    }
    return 0;
}

// NTSC-U/C: 0x005eba38, PAL: 0x0062db80
void ACanvas::DrawHorzLineU(int nY, int nLeft, int nRight) {
    for (int x = nLeft; x < nRight; ++x) {
        DrawPixelU(x, nY);
    }
}

// NTSC-U/C: 0x005ebab8, PAL: 0x0062dc00
void ACanvas::DrawHorzLine(int nY, int nLeft, int nRight) {
    if (nY < mClip.mTop || nY >= mClip.mBottom) {
        return;
    }
    if (nLeft < mClip.mLeft) {
        nLeft = mClip.mLeft;
    }
    if (mClip.mRight < nRight) {
        nRight = mClip.mRight;
    }
    if (nLeft < nRight) {
        DrawHorzLineU(nY, nLeft, nRight);
    }
}

// NTSC-U/C: 0x005ebb30, PAL: 0x0062dc78
void ACanvas::DrawVertLineU(int nX, int nTop, int nBottom) {
    for (int y = nTop; y < nBottom; ++y) {
        DrawPixelU(nX, y);
    }
}

// NTSC-U/C: 0x005ebbb0, PAL: 0x0062dcf8
void ACanvas::DrawVertLine(int nX, int nTop, int nBottom) {
    if (nX < mClip.mLeft || nX >= mClip.mRight) {
        return;
    }
    if (nTop < mClip.mTop) {
        nTop = mClip.mTop;
    }
    if (mClip.mBottom < nBottom) {
        nBottom = mClip.mBottom;
    }
    if (nTop < nBottom) {
        DrawVertLineU(nX, nTop, nBottom);
    }
}

// NTSC-U/C: 0x005ebc28, PAL: 0x0062dd70
void ACanvas::DrawRectU(ARect rect) {
    for (int y = rect.mTop; y < rect.mBottom; ++y) {
        DrawHorzLineU(y, rect.mLeft, rect.mRight);
    }
}

// NTSC-U/C: 0x005ebc98, PAL: 0x0062dde0
void ACanvas::DrawRect(ARect rect) {
    rect = rect.Intersection(mClip);
    if (rect.mLeft < rect.mRight && rect.mTop < rect.mBottom) {
        DrawRectU(rect);
    }
}

// NTSC-U/C: 0x005e9378, PAL: 0x0062b4c0
void ACanvas::DrawBoxU(ARect rect) {
    DrawHorzLineU(rect.mTop, rect.mLeft, rect.mRight);
    DrawHorzLineU(rect.mBottom - 1, rect.mLeft, rect.mRight);
    DrawVertLineU(rect.mLeft, rect.mTop + 1, rect.mBottom - 1);
    DrawVertLineU(rect.mRight - 1, rect.mTop + 1, rect.mBottom - 1);
}

// NTSC-U/C: 0x005e9440, PAL: 0x0062b588
void ACanvas::DrawBox(ARect rect) {
    DrawHorzLine(rect.mTop, rect.mLeft, rect.mRight);
    DrawHorzLine(rect.mBottom - 1, rect.mLeft, rect.mRight);
    DrawVertLine(rect.mLeft, rect.mTop + 1, rect.mBottom - 1);
    DrawVertLine(rect.mRight - 1, rect.mTop + 1, rect.mBottom - 1);
}

// NTSC-U/C: 0x005eaeb8, PAL: 0x0062d000
int ACanvas::ClipRect(ARect *pRect) const {
    *pRect = pRect->Intersection(mClip);
    return pRect->mLeft < pRect->mRight && pRect->mTop < pRect->mBottom;
}

// NTSC-U/C: 0x005ebe10, PAL: 0x0062df58
void ACanvas::DrawClutRect(ARect rect, const unsigned char *pRemap) {
    rect = rect.Intersection(mClip);
    if (rect.mLeft < rect.mRight && rect.mTop < rect.mBottom) {
        DrawClutRectU(rect, pRemap);
    }
}

// NTSC-U/C: 0x005e95e8, PAL: 0x0062b730
void ACanvas::DrawFlatConvexPolygon(const APolygon &polygon) {
    const short nTop = static_cast<short>(FindTopmostPolyVertex(polygon));
    int nY = polygon.mPoints[polygon.mIndices[nTop]].mY >> kACanvasFractionBits;
    APolygonEdge left;
    APolygonEdge right;
    left.mBottom = static_cast<short>(nY);
    right.mBottom = static_cast<short>(nY);
    polygon.SetupEdge(&left, nTop, kEdgeForward);
    polygon.SetupEdge(&right, nTop, kEdgeBackward);
    SetColorNative(polygon.mColor);
    for (;;) {
        if (nY >= left.mBottom) {
            if (nY >= right.mBottom) {
                if (left.mTo == right.mTo) {
                    return;
                }
                int nNext = right.mTo - 1;
                if (nNext < 0) {
                    nNext = polygon.mVertexCount - 1;
                }
                if (nNext == left.mTo) {
                    return;
                }
            }
            polygon.SetupEdge(&left, left.mTo, kEdgeForward);
        }
        if (nY >= right.mBottom) {
            polygon.SetupEdge(&right, right.mTo, kEdgeBackward);
        }
        if (nY >= mClip.mTop) {
            DrawHorzLine(nY,
                         (left.mX + AFix::onehalf) >> kACanvasFractionBits,
                         (right.mX + AFix::onehalf) >> kACanvasFractionBits);
        }
        ++nY;
        left.mX += left.mStepX;
        right.mX += right.mStepX;
        if (nY >= mClip.mBottom) {
            return;
        }
    }
}

// NTSC-U/C: 0x005e98c8, PAL: 0x0062ba10
void ACanvas::DrawTmappedConvexPolygon(const APolygon &polygon) {
    const short nTop = static_cast<short>(FindTopmostPolyVertex(polygon));
    int nY = polygon.mPoints[polygon.mIndices[nTop]].mY >> kACanvasFractionBits;
    APolygonEdge left;
    APolygonEdge right;
    left.mBottom = static_cast<short>(nY);
    right.mBottom = static_cast<short>(nY);
    polygon.SetupTexturedEdge(&left, nTop, kEdgeForward, polygon.mTexture);
    polygon.SetupTexturedEdge(&right, nTop, kEdgeBackward, polygon.mTexture);
    for (;;) {
        if (nY >= left.mBottom) {
            if (nY >= right.mBottom) {
                if (left.mTo == right.mTo) {
                    return;
                }
                int nNext = right.mTo - 1;
                if (nNext < 0) {
                    nNext = polygon.mVertexCount - 1;
                }
                if (nNext == left.mTo) {
                    return;
                }
            }
            polygon.SetupTexturedEdge(&left, left.mTo, kEdgeForward, polygon.mTexture);
        }
        if (nY >= right.mBottom) {
            polygon.SetupTexturedEdge(&right, right.mTo, kEdgeBackward, polygon.mTexture);
        }
        if (nY >= mClip.mTop) {
            int nLeft = (left.mX + AFix::onehalf) >> kACanvasFractionBits;
            const int nRight = (right.mX + AFix::onehalf) >> kACanvasFractionBits;
            const int nColumns = nRight - nLeft;
            if (nColumns > 0) {
                APoint position = left.mTexCoord;
                APoint step;
                step.mX = (right.mTexCoord.mX - position.mX) / nColumns;
                step.mY = (right.mTexCoord.mY - position.mY) / nColumns;
                if (nLeft < mClip.mLeft) {
                    position.mY += step.mY * (mClip.mLeft - nLeft);
                    position.mX += step.mX * (mClip.mLeft - nLeft);
                    nLeft = mClip.mLeft;
                }
                DrawTmapRow8U(nY,
                              nLeft,
                              mClip.mRight < nRight ? mClip.mRight : nRight,
                              polygon.mTexture,
                              &position,
                              &step);
            }
        }
        ++nY;
        left.mX += left.mStepX;
        right.mX += right.mStepX;
        left.mTexCoord.mX += left.mTexStep.mX;
        left.mTexCoord.mY += left.mTexStep.mY;
        right.mTexCoord.mX += right.mTexStep.mX;
        right.mTexCoord.mY += right.mTexStep.mY;
        if (nY >= mClip.mBottom) {
            return;
        }
    }
}

// NTSC-U/C: 0x005ec130, PAL: 0x0062e278
void ACanvas::DrawBitmapU(const ABitmap &source, int nX, int nY) {
    (this->*pfDrawBitmapU[source.mFormat])(source, nX, nY);
}

// NTSC-U/C: 0x005ec1d8, PAL: 0x0062e320
void ACanvas::DrawBitmap(const ABitmap &source, int nX, int nY) {
    (this->*kCopyForFormat[source.mFormat])(source, nX, nY);
}

// NTSC-U/C: 0x005ed990, PAL: 0x0062fad8
void ACanvas::DrawClutBitmapLin4(const ABitmap &source,
                                 int nX,
                                 int nY,
                                 const unsigned char *pRemap) {
    ABitmap clipped = source;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        DrawClutBitmapLin4U(clipped, nX, nY, pRemap);
    }
}

// NTSC-U/C: 0x005edb38, PAL: 0x0062fc80
void ACanvas::DrawClutBitmapLin8(const ABitmap &source,
                                 int nX,
                                 int nY,
                                 const unsigned char *pRemap) {
    ABitmap clipped = source;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        DrawClutBitmapLin8U(clipped, nX, nY, pRemap);
    }
}

// NTSC-U/C: 0x005ee128, PAL: 0x00630270
void ACanvas::DrawBlendBitmapLin4(const ABitmap &source,
                                  int nX,
                                  int nY,
                                  const unsigned char *const *ppBlend) {
    ABitmap clipped = source;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        DrawBlendBitmapLin4U(clipped, nX, nY, ppBlend);
    }
}

// NTSC-U/C: 0x005ee2d0, PAL: 0x00630418
void ACanvas::DrawBlendBitmapLin8(const ABitmap &source,
                                  int nX,
                                  int nY,
                                  const unsigned char *const *ppBlend) {
    ABitmap clipped = source;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        DrawBlendBitmapLin8U(clipped, nX, nY, ppBlend);
    }
}

// NTSC-U/C: 0x005ebd40, PAL: 0x0062de88
void ACanvas::DrawClutRectU(ARect rect, const unsigned char *pRemap) {
    for (int y = rect.mTop; y < rect.mBottom; ++y) {
        for (int x = rect.mLeft; x < rect.mRight; ++x) {
            DrawPixel8U(x, y, pRemap[GetPixel8U(x, y)]);
        }
    }
}

// NTSC-U/C: 0x005ebec8, PAL: 0x0062e010
void ACanvas::DrawLineU(int nX0, int nY0, int nX1, int nY1) {
    int nStepX = 0;
    int nStepY = 0;
    for (int nRemaining = SetupLine(nX0, nY0, nX1, nY1, &nStepX, &nStepY); nRemaining > 0;
         --nRemaining) {
        DrawPixelU(nX0 >> kACanvasFractionBits, nY0 >> kACanvasFractionBits);
        nX0 += nStepX;
        nY0 += nStepY;
    }
}

// NTSC-U/C: 0x005ebf70, PAL: 0x0062e0b8
void ACanvas::DrawLine(int nX0, int nY0, int nX1, int nY1) {
    if (ClipLine(&nX0, &nY0, &nX1, &nY1) != 0) {
        DrawLineU(nX0, nY0, nX1, nY1);
    }
}

// NTSC-U/C: 0x005e9508, PAL: 0x0062b650
int ACanvas::SetupLine(int nX0, int nY0, int nX1, int nY1, int *pnStepX, int *pnStepY) {
    const int nDeltaX = nX1 - nX0;
    const int nDeltaY = nY1 - nY0;
    const int nLengthX = abs(nDeltaX);
    const int nLengthY = abs(nDeltaY);
    if (nLengthX == 0 && nLengthY == 0) {
        return 0;
    }
    if (nLengthY < nLengthX) {
        *pnStepX = nDeltaX >= 0 ? kFixedOne : -kFixedOne;
        *pnStepY = (nDeltaY << kACanvasFractionBits) / nLengthX;
        return (nLengthX >> kACanvasFractionBits) + 1;
    }
    if (nLengthY > 0) {
        *pnStepX = (nDeltaX << kACanvasFractionBits) / nLengthY;
        *pnStepY = nDeltaY >= 0 ? kFixedOne : -kFixedOne;
        return (nLengthY >> kACanvasFractionBits) + 1;
    }
    return 0; // Yes, the binary tests the length again although this return is unreachable.
}

// NTSC-U/C: 0x005ec050, PAL: 0x0062e198
void ACanvas::DrawTmapRow8U(int nY,
                            int nLeft,
                            int nRight,
                            const ABitmap *pSource,
                            APoint *pSourcePosition,
                            const APoint *pSourceStep) {
    for (int x = nLeft; x < nRight; ++x) {
        const unsigned char *pPixel =
            SourceRow(*pSource) +
            (pSourcePosition->mY >> kACanvasFractionBits) * pSource->mBytesPerRow +
            (pSourcePosition->mX >> kACanvasFractionBits);
        DrawPixel8U(x, nY, *pPixel);
        pSourcePosition->mX += pSourceStep->mX;
        pSourcePosition->mY += pSourceStep->mY;
    }
}

// NTSC-U/C: 0x005ec280, PAL: 0x0062e3c8
void ACanvas::DrawBitmapLin4U(const ABitmap &source, int nX, int nY) {
    const unsigned char *pRow = SourceRow(source);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        const unsigned char *pByte = pRow;
        unsigned int bHighNibble = source.mOddNibbleStart;
        for (int x = nX; x < nX + source.mWidth; ++x) {
            const int nIndex = bHighNibble != 0 ? (*pByte >> kNibbleBits) : (*pByte & kNibbleMask);
            const unsigned char *pNext = pByte + 1;
            if (bHighNibble != 0) {
                pByte = pNext;
            }
            bHighNibble ^= 1;
            // Yes, the test compares a whole source byte against the low byte of the key rather
            // than the nibble just consumed, and it reads the byte the walk has already advanced
            // to.
            if (source.mHasTransparentColor == 0 ||
                *pByte != static_cast<unsigned char>(source.mTransparentColor)) {
                DrawPixel8U(x, y, nIndex);
            }
        }
        pRow += source.mBytesPerRow;
    }
}

// NTSC-U/C: 0x005ec3c0, PAL: 0x0062e508
void ACanvas::DrawBitmapLin4(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        DrawBitmapLin4U(clipped, nX, nY);
    }
}

// NTSC-U/C: 0x005ec450, PAL: 0x0062e598
void ACanvas::DrawBitmapLin8U(const ABitmap &source, int nX, int nY) {
    const unsigned char *pPixel = SourceRow(source);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        for (int x = nX; x < nX + source.mWidth; ++x) {
            // The binary compares the index with the low byte of the transparent colour only.
            if (source.mHasTransparentColor == 0 ||
                *pPixel != static_cast<unsigned char>(source.mTransparentColor)) {
                DrawPixel8U(x, y, *pPixel);
            }
            ++pPixel;
        }
        pPixel += source.mBytesPerRow - source.mWidth;
    }
}

// NTSC-U/C: 0x005ec570, PAL: 0x0062e6b8
void ACanvas::DrawBitmapLin8(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        DrawBitmapLin8U(clipped, nX, nY);
    }
}

// NTSC-U/C: 0x005ec600, PAL: 0x0062e748
void ACanvas::DrawBitmapLin15U(const ABitmap &source, int nX, int nY) {
    const unsigned short *pPixel = static_cast<const unsigned short *>(source.mPixels);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        for (int x = nX; x < nX + source.mWidth; ++x) {
            // The binary compares the pixel with the low halfword of the transparent colour only.
            if (source.mHasTransparentColor == 0 ||
                *pPixel != static_cast<unsigned short>(source.mTransparentColor)) {
                DrawPixel15U(x, y, *pPixel);
            }
            ++pPixel;
        }
        pPixel += source.mBytesPerRow / 2 - source.mWidth;
    }
}

// NTSC-U/C: 0x005ec738, PAL: 0x0062e880
void ACanvas::DrawBitmapLin15(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        DrawBitmapLin15U(clipped, nX, nY);
    }
}

// NTSC-U/C: 0x005ec7c8, PAL: 0x0062e910
void ACanvas::DrawBitmapLin24U(const ABitmap &source, int nX, int nY) {
    const unsigned char *pPixel = SourceRow(source);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        for (int x = nX; x < nX + source.mWidth; ++x) {
            bool bDraw = true;
            if (source.mHasTransparentColor != 0) {
                unsigned char abPixel[4];
                abPixel[0] = pPixel[0];
                abPixel[1] = pPixel[1];
                abPixel[2] = pPixel[2];
                abPixel[3] = 0;
                bDraw = source.mTransparentColor != *reinterpret_cast<unsigned int *>(abPixel);
            }
            if (bDraw) {
                DrawPixel24U(x, y, pPixel);
            }
            pPixel += kRGBByteCount;
        }
        pPixel += source.mBytesPerRow - kRGBByteCount * source.mWidth;
    }
}

// NTSC-U/C: 0x005ec928, PAL: 0x0062ea70
void ACanvas::DrawBitmapLin24(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        DrawBitmapLin24U(clipped, nX, nY);
    }
}

// NTSC-U/C: 0x005ec9b8, PAL: 0x0062eb00
void ACanvas::DrawBitmapLin32U(const ABitmap &source, int nX, int nY) {
    const unsigned char *pRow = SourceRow(source);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        const unsigned int *pPixel =
            static_cast<const unsigned int *>(static_cast<const void *>(pRow));
        for (int x = nX; x < nX + source.mWidth; ++x) {
            if (source.mHasTransparentColor == 0 || source.mTransparentColor != *pPixel) {
                DrawPixel32U(x, y, *pPixel);
            }
            ++pPixel;
        }
        pRow += source.mBytesPerRow;
    }
}

// NTSC-U/C: 0x005ecad8, PAL: 0x0062ec20
void ACanvas::DrawBitmapLin32(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        DrawBitmapLin32U(clipped, nX, nY);
    }
}

// NTSC-U/C: 0x005ecb68, PAL: 0x0062ecb0
void ACanvas::DrawBitmapRle8U(const ABitmap &source, int nX, int nY) {
    ABitmap row(ACanvas::tempBuff,
                kABitmapFormatLinear8,
                source.mHasTransparentColor != 0,
                source.mWidth,
                1,
                0);
    row.mTransparentColor = source.mTransparentColor;
    row.mPalette = source.mPalette;
    ARle8Reader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    for (int y = nY; y < nY + source.mHeight; ++y) {
        reader.UnpackRow(ACanvas::tempBuff);
        DrawBitmapLin8U(row, nX, y);
    }
}

// NTSC-U/C: 0x005e9cb8, PAL: 0x0062be00
void ACanvas::DrawBitmapRle8(const ABitmap &source, int nX, int nY) {
    if (nY >= mClip.mTop && mClip.mBottom >= nY + source.mHeight && nX >= mClip.mLeft &&
        mClip.mRight >= nX + source.mWidth) {
        DrawBitmapRle8U(source, nX, nY);
        return;
    }

    ARle8Reader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    Rle8Clip clip;
    if (ClipRle8Bitmap(source, &nX, &nY, &reader, &clip) == 0) {
        return;
    }

    ABitmap row(ACanvas::tempBuff,
                kABitmapFormatLinear8,
                source.mHasTransparentColor != 0,
                source.mWidth,
                1,
                0);
    row.mPixels = ACanvas::tempBuff + clip.mSkipLeft;
    row.mWidth = static_cast<short>(clip.mStopColumn - clip.mSkipLeft);
    row.mTransparentColor = source.mTransparentColor;
    row.mPalette = source.mPalette;
    for (int y = nY; y < clip.mStopRow; ++y) {
        reader.UnpackRow(ACanvas::tempBuff);
        DrawBitmapLin8U(row, nX, y);
    }
}

// NTSC-U/C: 0x005ecef0, PAL: 0x0062f038
void ACanvas::GetBitmapLin4(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        GetBitmapLin4U(clipped, nX, nY);
    }
}

// NTSC-U/C: 0x005ecdb8, PAL: 0x0062ef00
void ACanvas::GetBitmapLin4U(const ABitmap &dest, int nX, int nY) {
    unsigned char *pRow = DestRow(dest);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        unsigned char *pByte = pRow;
        unsigned int bHighNibble = dest.mOddNibbleStart;
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            const int nIndex = GetPixel8U(x, y);
            if (bHighNibble != 0) {
                *pByte =
                    static_cast<unsigned char>((*pByte & kNibbleMask) | (nIndex << kNibbleBits));
                ++pByte;
            } else {
                *pByte = static_cast<unsigned char>(nIndex | (*pByte & ~kNibbleMask));
            }
            bHighNibble ^= 1;
        }
        pRow += dest.mBytesPerRow;
    }
}

// NTSC-U/C: 0x005ed070, PAL: 0x0062f1b8
void ACanvas::GetBitmapLin8(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        GetBitmapLin8U(clipped, nX, nY);
    }
}

// NTSC-U/C: 0x005ecf80, PAL: 0x0062f0c8
void ACanvas::GetBitmapLin8U(const ABitmap &dest, int nX, int nY) {
    unsigned char *pPixel = DestRow(dest);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            *pPixel = static_cast<unsigned char>(GetPixel8U(x, y));
            ++pPixel;
        }
        pPixel += dest.mBytesPerRow - dest.mWidth;
    }
}

// NTSC-U/C: 0x005ed208, PAL: 0x0062f350
void ACanvas::GetBitmapLin15(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        GetBitmapLin15U(clipped, nX, nY);
    }
}

// NTSC-U/C: 0x005ed100, PAL: 0x0062f248
void ACanvas::GetBitmapLin15U(const ABitmap &dest, int nX, int nY) {
    unsigned short *pPixel = static_cast<unsigned short *>(dest.mPixels);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            *pPixel = GetPixel15U(x, y);
            ++pPixel;
        }
        pPixel += dest.mBytesPerRow / 2 - dest.mWidth;
    }
}

// NTSC-U/C: 0x005ed398, PAL: 0x0062f4e0
void ACanvas::GetBitmapLin24(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        GetBitmapLin24U(clipped, nX, nY);
    }
}

// NTSC-U/C: 0x005ed298, PAL: 0x0062f3e0
void ACanvas::GetBitmapLin24U(const ABitmap &dest, int nX, int nY) {
    unsigned char *pPixel = DestRow(dest);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            GetPixel24U(x, y, pPixel);
            pPixel += kRGBByteCount;
        }
        pPixel += dest.mBytesPerRow - kRGBByteCount * dest.mWidth;
    }
}

// NTSC-U/C: 0x005ed520, PAL: 0x0062f668
void ACanvas::GetBitmapLin32(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        GetBitmapLin32U(clipped, nX, nY);
    }
}

// NTSC-U/C: 0x005ed428, PAL: 0x0062f570
void ACanvas::GetBitmapLin32U(const ABitmap &dest, int nX, int nY) {
    unsigned int *pPixel = static_cast<unsigned int *>(dest.mPixels);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            *pPixel = GetPixel32U(x, y);
            ++pPixel;
        }
        pPixel += dest.mBytesPerRow / 4 - dest.mWidth;
    }
}

// NTSC-U/C: 0x005e9ee8, PAL: 0x0062c030
void ACanvas::DrawCharU(int nCharCode, const AFont *pFont, int nX, int nY) {
    const ABitmap *pGlyph = GlyphForCode(pFont, nCharCode);
    (this->*pfDrawBitmapU[pGlyph->mFormat])(*pGlyph, nX, nY - pFont->mBaseline);
}

// NTSC-U/C: 0x005e9fc8, PAL: 0x0062c110
void ACanvas::DrawChar(int nCharCode, const AFont *pFont, int nX, int nY) {
    const ABitmap *pGlyph = GlyphForCode(pFont, nCharCode);
    // The compiled copy is 0x1c bytes, four more than an ABitmap. See the note in abitmap.h.
    ABitmap clipped = *pGlyph;
    nY -= pFont->mBaseline;
    if (ClipBitmap(&clipped, &nX, &nY) != 0) {
        (this->*pfDrawBitmapU[clipped.mFormat])(clipped, nX, nY);
    }
}

// NTSC-U/C: 0x005ed5b0, PAL: 0x0062f6f8
void ACanvas::DrawTextU(const char *pText, const AFont *pFont, int nX, int nY) {
    const int nStartX = nX;
    for (unsigned char ch = *pText++; ch != 0; ch = *pText++) {
        if (ch == kNewline) {
            nY += pFont->mLineHeight;
            nX = nStartX;
            continue;
        }
        DrawCharU(ch, pFont, nX, nY);
        nX += GlyphForCode(pFont, ch)->mWidth;
    }
}

// NTSC-U/C: 0x005ea120, PAL: 0x0062c268
void ACanvas::DrawText(const char *pText, const AFont *pFont, int nX, int nY) {
    if (nY - pFont->mBaseline >= mClip.mBottom) {
        return;
    }
    if (nY + pFont->mLineHeight - pFont->mBaseline < mClip.mTop) {
        // Yes, the binary walks to the end of the string and then returns without drawing.
        while (*pText != '\0') {
            ++pText;
        }
        return;
    }
    const int nStartX = nX;
    for (unsigned char ch = *pText++; ch != 0; ch = *pText++) {
        if (ch == kNewline) {
            nY += pFont->mLineHeight;
            if (nY - pFont->mBaseline >= mClip.mBottom) {
                return;
            }
            nX = nStartX;
            continue;
        }
        DrawChar(ch, pFont, nX, nY);
        nX += GlyphForCode(pFont, ch)->mWidth;
    }
}

// NTSC-U/C: 0x005ed858, PAL: 0x0062f9a0
void ACanvas::DrawClutBitmapLin4U(const ABitmap &source,
                                  int nX,
                                  int nY,
                                  const unsigned char *pRemap) {
    ARowInfo span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = ACanvas::tempBuff;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = ACanvas::palDefault;
        }
    }
    const unsigned char *pRow = SourceRow(source);
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        Unpack4(pRow, ACanvas::tempBuff, source.mWidth, source.mOddNibbleStart);
        DrawClutBitmapRowLin8U(span, pRemap);
        pRow += source.mBytesPerRow;
    }
}

// NTSC-U/C: 0x005eda30, PAL: 0x0062fb78
void ACanvas::DrawClutBitmapLin8U(const ABitmap &source,
                                  int nX,
                                  int nY,
                                  const unsigned char *pRemap) {
    ARowInfo span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = const_cast<unsigned char *>(SourceRow(source));
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = ACanvas::palDefault;
        }
    }
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        DrawClutBitmapRowLin8U(span, pRemap);
        span.mSource += source.mBytesPerRow;
    }
}

// NTSC-U/C: 0x005edd08, PAL: 0x0062fe50
void ACanvas::DrawClutBitmapRowLin8U(const ARowInfo &span, const unsigned char *pRemap) {
    const unsigned char *pPixel = span.mSource;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned int nIndex = *pPixel++;
        if (!span.mHasTransparentColor || nIndex != span.mTransparentColor) {
            DrawPixel8U(x, span.mY, pRemap[nIndex]);
        }
    }
}

// NTSC-U/C: 0x005edfb8, PAL: 0x00630100
void ACanvas::DrawBlendBitmapLin4U(const ABitmap &source,
                                   int nX,
                                   int nY,
                                   const unsigned char *const *ppBlend) {
    ARowInfo span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = ACanvas::tempBuff;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = ACanvas::palDefault;
        }
    }
    const unsigned char *pRow = SourceRow(source);
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        Unpack4(pRow, ACanvas::tempBuff, source.mWidth, source.mOddNibbleStart);
        DrawBlendBitmapRowLin8U(span, ppBlend);
        pRow += source.mBytesPerRow;
    }
}

// NTSC-U/C: 0x005ee1c8, PAL: 0x00630310
void ACanvas::DrawBlendBitmapLin8U(const ABitmap &source,
                                   int nX,
                                   int nY,
                                   const unsigned char *const *ppBlend) {
    ARowInfo span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = const_cast<unsigned char *>(SourceRow(source));
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = ACanvas::palDefault;
        }
    }
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        DrawBlendBitmapRowLin8U(span, ppBlend);
        span.mSource += source.mBytesPerRow;
    }
}

// NTSC-U/C: 0x005ee4a0, PAL: 0x006305e8
void ACanvas::DrawBlendBitmapRowLin8U(const ARowInfo &span, const unsigned char *const *ppBlend) {
    const unsigned char *pPixel = span.mSource;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned int nIndex = *pPixel++;
        // Index zero is the transparent one here, unlike DrawClutBitmapRowLin8U(), which compares
        // against the span transparent colour.
        if (nIndex == 0 && span.mHasTransparentColor) {
            continue;
        }
        const int nDestIndex = GetPixel8U(x, span.mY);
        DrawPixel8U(x, span.mY, ppBlend[nIndex][nDestIndex]);
    }
}

// NTSC-U/C: 0x005ee968, PAL: 0x00630ab0
void ACanvas::DrawScaledBitmapRowLin8U(const AScaledRowInfo &span) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 ||
            nIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            DrawPixel8U(x, span.mY, nIndex);
        }
        nPosition += span.mSourceStep;
    }
}

// NTSC-U/C: 0x005eea18, PAL: 0x00630b60
void ACanvas::DrawScaledBitmapRowLin15U(const AScaledRowInfo &span) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned short nColor = *reinterpret_cast<const unsigned short *>(
            span.mSource + 2 * (nPosition >> kACanvasFractionBits));
        if (span.mHasTransparentColor == 0 ||
            nColor != static_cast<unsigned short>(span.mTransparentColor)) {
            DrawPixel15U(x, span.mY, nColor);
        }
        nPosition += span.mSourceStep;
    }
}

// NTSC-U/C: 0x005eeac8, PAL: 0x00630c10
void ACanvas::DrawScaledBitmapRowLin24U(const AScaledRowInfo &span) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned char *pRGB =
            span.mSource + kRGBByteCount * (nPosition >> kACanvasFractionBits);
        bool bDraw = true;
        if (span.mHasTransparentColor != 0) {
            unsigned char abPixel[4];
            abPixel[0] = pRGB[0];
            abPixel[1] = pRGB[1];
            abPixel[2] = pRGB[2];
            abPixel[3] = 0;
            bDraw = span.mTransparentColor != *reinterpret_cast<unsigned int *>(abPixel);
        }
        if (bDraw) {
            DrawPixel24U(x, span.mY, pRGB);
        }
        nPosition += span.mSourceStep;
    }
}

// NTSC-U/C: 0x005eebb8, PAL: 0x00630d00
void ACanvas::DrawScaledBitmapRowLin32U(const AScaledRowInfo &span) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned int nColor = *reinterpret_cast<const unsigned int *>(
            span.mSource + 4 * (nPosition >> kACanvasFractionBits));
        if (span.mHasTransparentColor == 0 || span.mTransparentColor != nColor) {
            DrawPixel32U(x, span.mY, nColor);
        }
        nPosition += span.mSourceStep;
    }
}

// NTSC-U/C: 0x005eedb8, PAL: 0x00630f00
void ACanvas::DrawScaledClutBitmapRowLin8U(const AScaledRowInfo &span,
                                           const unsigned char *pRemap) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 ||
            nIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            DrawPixel8U(x, span.mY, pRemap[nIndex]);
        }
        nPosition += span.mSourceStep;
    }
}

// NTSC-U/C: 0x005eefc0, PAL: 0x00631108
void ACanvas::DrawScaledBlendBitmapRowLin8U(const AScaledRowInfo &span,
                                            const unsigned char *const *ppBlend) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 ||
            nIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            const int nDestIndex = GetPixel8U(x, span.mY);
            DrawPixel8U(x, span.mY, ppBlend[nIndex][nDestIndex]);
        }
        nPosition += span.mSourceStep;
    }
}

// NTSC-U/C: 0x005eddb8, PAL: 0x0062ff00
void ACanvas::Unpack4(const unsigned char *pSource,
                      unsigned char *pDest,
                      int nCount,
                      int bStartHighNibble) {
    while (nCount-- > 0) {
        if (bStartHighNibble != 0) {
            *pDest = static_cast<unsigned char>(*pSource++ >> kNibbleBits);
        } else {
            *pDest = static_cast<unsigned char>(*pSource & kNibbleMask);
        }
        ++pDest;
        bStartHighNibble ^= 1;
    }
}

// NTSC-U/C: 0x005ed6a8, PAL: 0x0062f7f0
// A format the chain does not test draws nothing at all, rather than falling back to a
// generic path.
void ACanvas::DrawClutBitmapU(const ABitmap &source, int nX, int nY, const unsigned char *pRemap) {
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        DrawClutBitmapLin4U(source, nX, nY, pRemap);
        break;
    case kABitmapFormatLinear8:
        DrawClutBitmapLin8U(source, nX, nY, pRemap);
        break;
    case kABitmapFormatRle8:
        DrawClutBitmapRle8U(source, nX, nY, pRemap);
        break;
    default:
        break;
    }
}

// NTSC-U/C: 0x005ed718, PAL: 0x0062f860
// The run-length format is handed over unclipped, because DrawClutBitmapRle8() clips for itself.
void ACanvas::DrawClutBitmap(const ABitmap &source, int nX, int nY, const unsigned char *pRemap) {
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        DrawClutBitmapLin4(source, nX, nY, pRemap);
        break;
    case kABitmapFormatLinear8:
        DrawClutBitmapLin8(source, nX, nY, pRemap);
        break;
    case kABitmapFormatRle8:
        DrawClutBitmapRle8(source, nX, nY, pRemap);
        break;
    default:
        break;
    }
}

// NTSC-U/C: 0x005edbd8, PAL: 0x0062fd20
void ACanvas::DrawClutBitmapRle8U(const ABitmap &source,
                                  int nX,
                                  int nY,
                                  const unsigned char *pRemap) {
    ARowInfo span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = ACanvas::tempBuff;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = ACanvas::palDefault;
        }
    }
    ARle8Reader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    // Yes, the binary advances nY rather than span.mY, so the bound recedes with the row and a
    // source with any rows never ends. DrawClutBitmapU() has no caller, so the loop never runs.
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++nY) {
        reader.UnpackRow(ACanvas::tempBuff);
        DrawClutBitmapRowLin8U(span, pRemap);
    }
}

// NTSC-U/C: 0x005ea280, PAL: 0x0062c3c8
// The clipping matches DrawBitmapRle8(), without its test for a source wholly inside the
// clip rectangle.
void ACanvas::DrawClutBitmapRle8(const ABitmap &source,
                                 int nX,
                                 int nY,
                                 const unsigned char *pRemap) {
    ARle8Reader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    Rle8Clip clip;
    if (ClipRle8Bitmap(source, &nX, &nY, &reader, &clip) == 0) {
        return;
    }

    ARowInfo span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(nX + (clip.mStopColumn - clip.mSkipLeft));
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = ACanvas::tempBuff + clip.mSkipLeft;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = ACanvas::palDefault;
        }
    }
    for (span.mY = static_cast<short>(nY); span.mY < clip.mStopRow; ++span.mY) {
        reader.UnpackRow(ACanvas::tempBuff);
        DrawClutBitmapRowLin8U(span, pRemap);
    }
}

// NTSC-U/C: 0x005ede08, PAL: 0x0062ff50
void ACanvas::DrawBlendBitmapU(const ABitmap &source,
                               int nX,
                               int nY,
                               const unsigned char *const *ppBlend) {
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        DrawBlendBitmapLin4U(source, nX, nY, ppBlend);
        break;
    case kABitmapFormatLinear8:
        DrawBlendBitmapLin8U(source, nX, nY, ppBlend);
        break;
    case kABitmapFormatRle8:
        DrawBlendBitmapRle8U(source, nX, nY, ppBlend);
        break;
    default:
        break;
    }
}

// NTSC-U/C: 0x005ede78, PAL: 0x0062ffc0
void ACanvas::DrawBlendBitmap(const ABitmap &source,
                              int nX,
                              int nY,
                              const unsigned char *const *ppBlend) {
    // As in DrawClutBitmap(), the run-length format is handed over unclipped.
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        DrawBlendBitmapLin4(source, nX, nY, ppBlend);
        break;
    case kABitmapFormatLinear8:
        DrawBlendBitmapLin8(source, nX, nY, ppBlend);
        break;
    case kABitmapFormatRle8:
        DrawBlendBitmapRle8(source, nX, nY, ppBlend);
        break;
    default:
        break;
    }
}

// NTSC-U/C: 0x005ee370, PAL: 0x006304b8
// Instruction for instruction DrawClutBitmapRle8U() with the blend slot called in
// place of the remap slot.
void ACanvas::DrawBlendBitmapRle8U(const ABitmap &source,
                                   int nX,
                                   int nY,
                                   const unsigned char *const *ppBlend) {
    ARowInfo span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = ACanvas::tempBuff;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = ACanvas::palDefault;
        }
    }
    ARle8Reader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    // Yes, the binary advances nY rather than span.mY here as well, and DrawBlendBitmapU() has no
    // caller either.
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++nY) {
        reader.UnpackRow(ACanvas::tempBuff);
        DrawBlendBitmapRowLin8U(span, ppBlend);
    }
}

// NTSC-U/C: 0x005ea460, PAL: 0x0062c5a8
// Instruction for instruction DrawClutBitmapRle8() with the blend slot called in place of
// the remap slot.
void ACanvas::DrawBlendBitmapRle8(const ABitmap &source,
                                  int nX,
                                  int nY,
                                  const unsigned char *const *ppBlend) {
    ARle8Reader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    Rle8Clip clip;
    if (ClipRle8Bitmap(source, &nX, &nY, &reader, &clip) == 0) {
        return;
    }

    ARowInfo span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(nX + (clip.mStopColumn - clip.mSkipLeft));
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = ACanvas::tempBuff + clip.mSkipLeft;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = ACanvas::palDefault;
        }
    }
    for (span.mY = static_cast<short>(nY); span.mY < clip.mStopRow; ++span.mY) {
        reader.UnpackRow(ACanvas::tempBuff);
        DrawBlendBitmapRowLin8U(span, ppBlend);
    }
}

// NTSC-U/C: 0x005ecc68, PAL: 0x0062edb0
void ACanvas::GetBitmapU(const ABitmap &dest, int nX, int nY) {
    // NTSC-U/C: 0x0077dcf8, PAL: 0x007c1ae8
    static const ABitmapCopyMember kReadNoClipForFormat[kABitmapFormatCount] = {
        &ACanvas::GetBitmapLin4U,
        &ACanvas::GetBitmapLin8U,
        &ACanvas::GetBitmapLin15U,
        &ACanvas::GetBitmapLin24U,
        &ACanvas::GetBitmapLin32U,
        &ACanvas::ReadRectRle8};
    (this->*kReadNoClipForFormat[dest.mFormat])(dest, nX, nY);
}

// NTSC-U/C: 0x005ecd10, PAL: 0x0062ee58
void ACanvas::GetBitmap(const ABitmap &dest, int nX, int nY) {
    // NTSC-U/C: 0x0077dd28, PAL: 0x007c1b18
    static const ABitmapCopyMember kReadForFormat[kABitmapFormatCount] = {&ACanvas::GetBitmapLin4,
                                                                          &ACanvas::GetBitmapLin8,
                                                                          &ACanvas::GetBitmapLin15,
                                                                          &ACanvas::GetBitmapLin24,
                                                                          &ACanvas::GetBitmapLin32,
                                                                          &ACanvas::ReadRectRle8};
    (this->*kReadForFormat[dest.mFormat])(dest, nX, nY);
}

// NTSC-U/C: 0x005eb190, PAL: 0x0062d2d8
void ACanvas::ReadRectRle8([[maybe_unused]] const ABitmap &dest,
                           [[maybe_unused]] int nX,
                           [[maybe_unused]] int nY) {
}

// NTSC-U/C: 0x005ea780, PAL: 0x0062c8c8
int ACanvas::ClipAndSetupScaledBitmap(const ABitmap &source,
                                      const ARect &rect,
                                      AScaledRowInfo *pBlit) const {
    if (!(rect.mLeft < rect.mRight && rect.mTop < rect.mBottom)) {
        return 0;
    }

    pBlit->mSourceStepY =
        (source.mHeight << kACanvasFractionBits) / static_cast<short>(rect.mBottom - rect.mTop);
    pBlit->mSourcePositionY = pBlit->mSourceStepY / 2;
    pBlit->mTop = rect.mTop;
    if (rect.mTop < mClip.mTop) {
        pBlit->mSourcePositionY += pBlit->mSourceStepY * (mClip.mTop - rect.mTop);
        pBlit->mTop = mClip.mTop;
    }
    pBlit->mBottom = rect.mBottom;
    if (mClip.mBottom < rect.mBottom) {
        pBlit->mBottom = mClip.mBottom;
    }

    pBlit->mSourceStep =
        (source.mWidth << kACanvasFractionBits) / static_cast<short>(rect.mRight - rect.mLeft);
    pBlit->mSourcePosition = pBlit->mSourceStep / 2;
    pBlit->mLeft = rect.mLeft;
    if (rect.mLeft < mClip.mLeft) {
        pBlit->mSourcePosition += pBlit->mSourceStep * (mClip.mLeft - rect.mLeft);
        pBlit->mLeft = mClip.mLeft;
    }
    pBlit->mRight = rect.mRight;
    if (mClip.mRight < rect.mRight) {
        pBlit->mRight = mClip.mRight;
    }

    pBlit->mSource =
        DestRow(source) + (pBlit->mSourcePositionY >> kACanvasFractionBits) * source.mBytesPerRow;
    pBlit->mPalette = source.mPalette;
    if (pBlit->mPalette == nullptr) {
        pBlit->mPalette = mBitmap.mPalette;
        if (pBlit->mPalette == nullptr) {
            pBlit->mPalette = ACanvas::palDefault;
        }
    }
    pBlit->mHasTransparentColor = source.mHasTransparentColor;
    pBlit->mTransparentColor = source.mTransparentColor;
    return pBlit->mTop < pBlit->mBottom && pBlit->mLeft < pBlit->mRight;
}

// NTSC-U/C: 0x005ee580, PAL: 0x006306c8
void ACanvas::DrawScaledBitmap(const ABitmap &source, const ARect &rect) {
    // NTSC-U/C: 0x0077dd58, PAL: 0x007c1b48
    static const ABitmapStretchMember kStretchForFormat[kABitmapFormatCount] = {
        &ACanvas::DrawScaledBitmapLin4,
        &ACanvas::DrawScaledBitmapLin8,
        &ACanvas::DrawScaledBitmapLin15,
        &ACanvas::DrawScaledBitmapLin24,
        &ACanvas::DrawScaledBitmapLin32,
        &ACanvas::DrawScaledBitmapRle8};
    (this->*kStretchForFormat[source.mFormat])(source, rect);
}

// NTSC-U/C: 0x005ea640, PAL: 0x0062c788
void ACanvas::DrawScaledBitmapLin4(const ABitmap &source, const ARect &rect) {
    AScaledRowInfo blit;
    if (ClipAndSetupScaledBitmap(source, rect, &blit) == 0) {
        return;
    }
    blit.mSource = ACanvas::tempBuff;
    // Yes, the walk starts at the first source row rather than at the row
    // ClipAndSetupScaledBitmap() selected. A rectangle clipped at the top samples from too high in
    // the source.
    const unsigned char *pRow = SourceRow(source);
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        Unpack4(pRow, ACanvas::tempBuff, source.mWidth, source.mOddNibbleStart);
        DrawScaledBitmapRowLin8U(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        pRow += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// NTSC-U/C: 0x005ee628, PAL: 0x00630770
void ACanvas::DrawScaledBitmapLin8(const ABitmap &source, const ARect &rect) {
    AScaledRowInfo blit;
    if (ClipAndSetupScaledBitmap(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        DrawScaledBitmapRowLin8U(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// NTSC-U/C: 0x005ee6f8, PAL: 0x00630840
void ACanvas::DrawScaledBitmapLin15(const ABitmap &source, const ARect &rect) {
    AScaledRowInfo blit;
    if (ClipAndSetupScaledBitmap(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        DrawScaledBitmapRowLin15U(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// NTSC-U/C: 0x005ee7c8, PAL: 0x00630910
void ACanvas::DrawScaledBitmapLin24(const ABitmap &source, const ARect &rect) {
    AScaledRowInfo blit;
    if (ClipAndSetupScaledBitmap(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        DrawScaledBitmapRowLin24U(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// NTSC-U/C: 0x005ee898, PAL: 0x006309e0
void ACanvas::DrawScaledBitmapLin32(const ABitmap &source, const ARect &rect) {
    AScaledRowInfo blit;
    if (ClipAndSetupScaledBitmap(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        DrawScaledBitmapRowLin32U(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// NTSC-U/C: 0x005ea960, PAL: 0x0062caa8
void ACanvas::DrawScaledBitmapRle8(const ABitmap &source, const ARect &rect) {
    AScaledRowInfo blit;
    if (ClipAndSetupScaledBitmap(source, rect, &blit) == 0) {
        return;
    }
    ARle8Reader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    if (nRow != 0) {
        reader.SkipRow(nRow);
    }
    reader.UnpackRow(ACanvas::tempBuff);
    blit.mSource = ACanvas::tempBuff;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        DrawScaledBitmapRowLin8U(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        if (nNextRow != nRow) {
            if (nNextRow - nRow != 1) {
                reader.SkipRow(nNextRow - nRow - 1);
            }
            reader.UnpackRow(ACanvas::tempBuff);
        }
        nRow = nNextRow;
    }
}

// NTSC-U/C: 0x005eec70, PAL: 0x00630db8
void ACanvas::DrawScaledClutBitmap(const ABitmap &source,
                                   const ARect &rect,
                                   const unsigned char *pRemap) {
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        DrawScaledClutBitmapLin4(source, rect, pRemap);
        break;
    case kABitmapFormatLinear8:
        DrawScaledClutBitmapLin8(source, rect, pRemap);
        break;
    case kABitmapFormatRle8:
        DrawScaledClutBitmapRle8(source, rect, pRemap);
        break;
    default:
        break;
    }
}

// NTSC-U/C: 0x005eecd0, PAL: 0x00630e18
void ACanvas::DrawScaledClutBitmapLin4([[maybe_unused]] const ABitmap &source,
                                       [[maybe_unused]] const ARect &rect,
                                       [[maybe_unused]] const unsigned char *pRemap) {
}

// NTSC-U/C: 0x005eecd8, PAL: 0x00630e20
void ACanvas::DrawScaledClutBitmapLin8(const ABitmap &source,
                                       const ARect &rect,
                                       const unsigned char *pRemap) {
    AScaledRowInfo blit;
    if (ClipAndSetupScaledBitmap(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        DrawScaledClutBitmapRowLin8U(blit, pRemap);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// NTSC-U/C: 0x005eaa98, PAL: 0x0062cbe0
void ACanvas::DrawScaledClutBitmapRle8(const ABitmap &source,
                                       const ARect &rect,
                                       const unsigned char *pRemap) {
    AScaledRowInfo blit;
    if (ClipAndSetupScaledBitmap(source, rect, &blit) == 0) {
        return;
    }
    ARle8Reader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    if (nRow != 0) {
        reader.SkipRow(nRow);
    }
    reader.UnpackRow(ACanvas::tempBuff);
    blit.mSource = ACanvas::tempBuff;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        DrawScaledClutBitmapRowLin8U(blit, pRemap);
        const int nNextRow = AdvanceStretchRow(&blit);
        if (nNextRow != nRow) {
            if (nNextRow - nRow != 1) {
                reader.SkipRow(nNextRow - nRow - 1);
            }
            reader.UnpackRow(ACanvas::tempBuff);
        }
        nRow = nNextRow;
    }
}

// NTSC-U/C: 0x005eee78, PAL: 0x00630fc0
void ACanvas::DrawScaledBlendBitmap(const ABitmap &source,
                                    const ARect &rect,
                                    const unsigned char *const *ppBlend) {
    // Yes, the four bit arm reads eight bit pixels and the eight bit arm is empty.
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        DrawScaledBlendBitmapLin4(source, rect, ppBlend);
        break;
    case kABitmapFormatLinear8:
        DrawScaledBlendBitmapLin8(source, rect, ppBlend);
        break;
    case kABitmapFormatRle8:
        DrawScaledBlendBitmapRle8(source, rect, ppBlend);
        break;
    default:
        break;
    }
}

// NTSC-U/C: 0x005eeed8, PAL: 0x00631020
void ACanvas::DrawScaledBlendBitmapLin8([[maybe_unused]] const ABitmap &source,
                                        [[maybe_unused]] const ARect &rect,
                                        [[maybe_unused]] const unsigned char *const *ppBlend) {
}

// NTSC-U/C: 0x005eeee0, PAL: 0x00631028
void ACanvas::DrawScaledBlendBitmapLin4(const ABitmap &source,
                                        const ARect &rect,
                                        const unsigned char *const *ppBlend) {
    AScaledRowInfo blit;
    if (ClipAndSetupScaledBitmap(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        DrawScaledBlendBitmapRowLin8U(blit, ppBlend);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// NTSC-U/C: 0x005eabe0, PAL: 0x0062cd28
void ACanvas::DrawScaledBlendBitmapRle8(const ABitmap &source,
                                        const ARect &rect,
                                        const unsigned char *const *ppBlend) {
    AScaledRowInfo blit;
    if (ClipAndSetupScaledBitmap(source, rect, &blit) == 0) {
        return;
    }
    ARle8Reader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    if (nRow != 0) {
        reader.SkipRow(nRow);
    }
    reader.UnpackRow(ACanvas::tempBuff);
    blit.mSource = ACanvas::tempBuff;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        DrawScaledBlendBitmapRowLin8U(blit, ppBlend);
        const int nNextRow = AdvanceStretchRow(&blit);
        if (nNextRow != nRow) {
            if (nNextRow - nRow != 1) {
                reader.SkipRow(nNextRow - nRow - 1);
            }
            reader.UnpackRow(ACanvas::tempBuff);
        }
        nRow = nNextRow;
    }
}

// One row of unpacked pixels. The bound is the address-space limit the header records, not a
// recovered size: the nearest referenced address sits just above 0x400 bytes past the buffer.
unsigned char ACanvas::tempBuff[0x400];
