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
#include "rndartt/afixed.h"
#include "rndartt/afont.h"
#include "rndartt/apalette.h"
#include "rndartt/apoint.h"
#include "rndartt/apolygon.h"
#include "rndartt/apolygonedge.h"
#include "rndartt/arowspan.h"
#include "rndartt/astretchblit.h"
#include "rndartt/astretchspan.h"
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

// This translation unit's copy of the tag ABitmap::ABitmap() also uses.
// 0x00837d80
const char *const kBitmapAllocTag = "abitmap.h";
constexpr int kBitmapAllocLine = 0x47;

// One pointer to member per ABitmapFormat code. DrawGlyphNoClip() and DrawGlyph() index the first
// two tables by the glyph format code, ReadRectNoClip() and ReadRect() the next two by the
// destination format code, and StretchBlit() the last by the source format code. The last three
// address protected members, so each is a static inside the member that indexes it.
typedef void (ACanvas::*ABitmapCopyMember)(const ABitmap &, int, int);
typedef void (ACanvas::*ABitmapStretchMember)(const ABitmap &, const ARect &);

// 0x0077dc98
const ABitmapCopyMember kCopyNoClipForFormat[kABitmapFormatCount] = {&ACanvas::Blit4NoClip,
                                                                     &ACanvas::Blit8NoClip,
                                                                     &ACanvas::Blit15NoClip,
                                                                     &ACanvas::Blit24NoClip,
                                                                     &ACanvas::Blit32NoClip,
                                                                     &ACanvas::BlitRle8NoClip};

// 0x0077dcc8
const ABitmapCopyMember kCopyForFormat[kABitmapFormatCount] = {&ACanvas::Blit4,
                                                               &ACanvas::Blit8,
                                                               &ACanvas::Blit15,
                                                               &ACanvas::Blit24,
                                                               &ACanvas::Blit32,
                                                               &ACanvas::BlitRle8};

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
inline int AdvanceStretchRow(AStretchBlit *pBlit) {
    pBlit->mSourcePositionY += pBlit->mSourceStepY;
    return pBlit->mSourcePositionY >> kACanvasFractionBits;
}

} // namespace

// 0x0086f6f0
APalette *g_pDefaultPalette = nullptr;

// 0x00557af8
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

// 0x005eb1a0
ACanvas::ACanvas(const ABitmap &bitmap) : mBitmap(bitmap) {
    mClip.mLeft = 0;
    mClip.mTop = 0;
    mClip.mRight = bitmap.mWidth;
    mClip.mBottom = bitmap.mHeight;
}

// 0x005e8bc8
ACanvas *ACanvas::CreateForBitmap(const ABitmap &bitmap, bool bAllocatePixels) {
    ABitmap copy = bitmap;
    if (bAllocatePixels) {
        if (copy.mFormat == kABitmapFormatLinear4) {
            copy.mBytesPerRow = static_cast<short>((copy.mWidth + 2) / 2);
        } else {
            copy.mBytesPerRow =
                static_cast<short>(copy.mWidth * g_abBitmapBytesPerPixel[copy.mFormat]);
        }
        copy.mPixels = MemAllocTagged(static_cast<long long>(copy.mHeight) * copy.mBytesPerRow,
                                      kBitmapAllocTag,
                                      kBitmapAllocLine);
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

// 0x005eb200
ACanvas *ACanvas::CreateWithOwnedPixels(const ABitmap &bitmap) {
    ABitmap copy = bitmap;
    if (copy.mFormat == kABitmapFormatRle8) {
        copy.mFormat = kABitmapFormatLinear8;
    }
    return CreateForBitmap(copy, true);
}

// 0x005ead68
ACanvas::~ACanvas() {
}

// 0x005eb3d0
unsigned char ACanvas::ClipCodeForPoint(int nX, int nY) const {
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

// 0x005eb418
int ACanvas::ClipBlitSpan(
    const ABitmap &source, int *pnX, int *pnY, ARleReader *pReader, AClipSpan *pSpan) const {
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
        pReader->SkipRows(mClip.mTop - *pnY);
        *pnY = mClip.mTop;
    }
    return *pnY < pSpan->mStopRow;
}

// 0x005e8fd8
int ACanvas::ClipLineToRect(int *pnX0, int *pnY0, int *pnX1, int *pnY1) const {
    unsigned char nCode0 =
        ClipCodeForPoint(*pnX0 >> kACanvasFractionBits, *pnY0 >> kACanvasFractionBits);
    for (;;) {
        unsigned char nCode1 =
            ClipCodeForPoint(*pnX1 >> kACanvasFractionBits, *pnY1 >> kACanvasFractionBits);
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
                                  (mClip.mRight << kACanvasFractionBits) - g_nFixedEpsilon;
            const int nSlope = ((*pnY1 - *pnY0) << kACanvasFractionBits) / (*pnX1 - *pnX0);
            *pnY1 = *pnY0 + ((nSlope * (nEdge - *pnX0)) >> kACanvasFractionBits);
            *pnX1 = nEdge;
        } else {
            const int nEdge = (nCode1 & kACanvasClipAbove) != 0 ?
                                  mClip.mTop << kACanvasFractionBits :
                                  (mClip.mBottom << kACanvasFractionBits) - g_nFixedEpsilon;
            const int nSlope = ((*pnX1 - *pnX0) << kACanvasFractionBits) / (*pnY1 - *pnY0);
            *pnX1 = *pnX0 + ((nSlope * (nEdge - *pnY0)) >> kACanvasFractionBits);
            *pnY1 = nEdge;
        }
    }
}

// 0x005e91b8
int ACanvas::ClipBlitToRect(ABitmap *pBitmap, int *pnX, int *pnY) const {
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
            pPixels += (mClip.mLeft - *pnX) * g_abBitmapBytesPerPixel[pBitmap->mFormat];
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

// 0x005eb520
void ACanvas::PutPixel(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixelNoClip(nX, nY);
    }
}

// 0x005eb590
void ACanvas::PutPixelIndexed(int nX, int nY, int nIndex) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixelIndexedNoClip(nX, nY, nIndex & kChannelMask);
    }
}

// 0x005eb608
void ACanvas::PutPixel15(int nX, int nY, unsigned short nColor) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixel15NoClip(nX, nY, nColor);
    }
}

// 0x005eb680
void ACanvas::PutPixelRGB(int nX, int nY, const unsigned char *pRGB) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixelRGBNoClip(nX, nY, pRGB);
    }
}

// 0x005eb6f0
void ACanvas::PutPixel(int nX, int nY, unsigned int nColor) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixelNoClip(nX, nY, nColor);
    }
}

// 0x005eb760
void ACanvas::PutPixelNative(int nX, int nY, unsigned int nColor) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixelNativeNoClip(nX, nY, nColor);
    }
}

// 0x005eb7d0
int ACanvas::GetPixelIndexed(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixelIndexedNoClip(nX, nY);
    }
    return 0;
}

// 0x005eb848
unsigned short ACanvas::GetPixel15(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixel15NoClip(nX, nY);
    }
    return 0;
}

// 0x005eb8c0
void ACanvas::GetPixelRGB(int nX, int nY, unsigned char *pRGB) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        GetPixelRGBNoClip(nX, nY, pRGB);
        return;
    }
    memset(pRGB, 0, kRGBByteCount);
}

// 0x005eb948
unsigned int ACanvas::GetPixel(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixelNoClip(nX, nY);
    }
    return 0;
}

// 0x005eb9c0
unsigned int ACanvas::GetPixelNative(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixelNativeNoClip(nX, nY);
    }
    return 0;
}

// 0x005eba38
void ACanvas::FillRowNoClip(int nY, int nLeft, int nRight) {
    for (int x = nLeft; x < nRight; ++x) {
        PutPixelNoClip(x, nY);
    }
}

// 0x005ebab8
void ACanvas::FillRow(int nY, int nLeft, int nRight) {
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
        FillRowNoClip(nY, nLeft, nRight);
    }
}

// 0x005ebb30
void ACanvas::FillColumnNoClip(int nX, int nTop, int nBottom) {
    for (int y = nTop; y < nBottom; ++y) {
        PutPixelNoClip(nX, y);
    }
}

// 0x005ebbb0
void ACanvas::FillColumn(int nX, int nTop, int nBottom) {
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
        FillColumnNoClip(nX, nTop, nBottom);
    }
}

// 0x005ebc28
void ACanvas::FillRectNoClip(ARect rect) {
    for (int y = rect.mTop; y < rect.mBottom; ++y) {
        FillRowNoClip(y, rect.mLeft, rect.mRight);
    }
}

// 0x005ebc98
void ACanvas::FillRect(ARect rect) {
    rect = rect.Intersection(mClip);
    if (rect.mLeft < rect.mRight && rect.mTop < rect.mBottom) {
        FillRectNoClip(rect);
    }
}

// 0x005e9378
void ACanvas::FrameRectNoClip(ARect rect) {
    FillRowNoClip(rect.mTop, rect.mLeft, rect.mRight);
    FillRowNoClip(rect.mBottom - 1, rect.mLeft, rect.mRight);
    FillColumnNoClip(rect.mLeft, rect.mTop + 1, rect.mBottom - 1);
    FillColumnNoClip(rect.mRight - 1, rect.mTop + 1, rect.mBottom - 1);
}

// 0x005e9440
void ACanvas::FrameRect(ARect rect) {
    FillRow(rect.mTop, rect.mLeft, rect.mRight);
    FillRow(rect.mBottom - 1, rect.mLeft, rect.mRight);
    FillColumn(rect.mLeft, rect.mTop + 1, rect.mBottom - 1);
    FillColumn(rect.mRight - 1, rect.mTop + 1, rect.mBottom - 1);
}

// 0x005eaeb8
int ACanvas::ClipRect(ARect *pRect) const {
    *pRect = pRect->Intersection(mClip);
    return pRect->mLeft < pRect->mRight && pRect->mTop < pRect->mBottom;
}

// 0x005ebe10
void ACanvas::RemapRectIndicesClipped(ARect rect, const unsigned char *pRemap) {
    rect = rect.Intersection(mClip);
    if (rect.mLeft < rect.mRight && rect.mTop < rect.mBottom) {
        RemapRectIndices(rect, pRemap);
    }
}

// 0x005e95e8
void ACanvas::FillPolygon(const APolygon &polygon) {
    const short nTop = static_cast<short>(polygon.FindTopVertex());
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
            FillRow(nY,
                    (left.mX + g_nFixedHalf) >> kACanvasFractionBits,
                    (right.mX + g_nFixedHalf) >> kACanvasFractionBits);
        }
        ++nY;
        left.mX += left.mStepX;
        right.mX += right.mStepX;
        if (nY >= mClip.mBottom) {
            return;
        }
    }
}

// 0x005e98c8
void ACanvas::FillTexturedPolygon(const APolygon &polygon) {
    const short nTop = static_cast<short>(polygon.FindTopVertex());
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
            int nLeft = (left.mX + g_nFixedHalf) >> kACanvasFractionBits;
            const int nRight = (right.mX + g_nFixedHalf) >> kACanvasFractionBits;
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
                TextureRowIndexed(nY,
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

// 0x005ec130
void ACanvas::BlitNoClip(const ABitmap &source, int nX, int nY) {
    (this->*kCopyNoClipForFormat[source.mFormat])(source, nX, nY);
}

// 0x005ec1d8
void ACanvas::Blit(const ABitmap &source, int nX, int nY) {
    (this->*kCopyForFormat[source.mFormat])(source, nX, nY);
}

// 0x005ed990
void ACanvas::BlitRemap4Clipped(const ABitmap &source,
                                int nX,
                                int nY,
                                const unsigned char *pRemap) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        BlitRemap4(clipped, nX, nY, pRemap);
    }
}

// 0x005edb38
void ACanvas::BlitRemap8Clipped(const ABitmap &source,
                                int nX,
                                int nY,
                                const unsigned char *pRemap) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        BlitRemap8(clipped, nX, nY, pRemap);
    }
}

// 0x005ee128
void ACanvas::BlitBlend4Clipped(const ABitmap &source,
                                int nX,
                                int nY,
                                const unsigned char *const *ppBlend) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        BlitBlend4(clipped, nX, nY, ppBlend);
    }
}

// 0x005ee2d0
void ACanvas::BlitBlend8Clipped(const ABitmap &source,
                                int nX,
                                int nY,
                                const unsigned char *const *ppBlend) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        BlitBlend8(clipped, nX, nY, ppBlend);
    }
}

// 0x005ebd40
void ACanvas::RemapRectIndices(ARect rect, const unsigned char *pRemap) {
    for (int y = rect.mTop; y < rect.mBottom; ++y) {
        for (int x = rect.mLeft; x < rect.mRight; ++x) {
            PutPixelIndexedNoClip(x, y, pRemap[GetPixelIndexedNoClip(x, y)]);
        }
    }
}

// 0x005ebec8
void ACanvas::DrawLineNoClip(int nX0, int nY0, int nX1, int nY1) {
    int nStepX = 0;
    int nStepY = 0;
    for (int nRemaining = SetupLineSteps(nX0, nY0, nX1, nY1, &nStepX, &nStepY); nRemaining > 0;
         --nRemaining) {
        PutPixelNoClip(nX0 >> kACanvasFractionBits, nY0 >> kACanvasFractionBits);
        nX0 += nStepX;
        nY0 += nStepY;
    }
}

// 0x005ebf70
void ACanvas::DrawLine(int nX0, int nY0, int nX1, int nY1) {
    if (ClipLineToRect(&nX0, &nY0, &nX1, &nY1) != 0) {
        DrawLineNoClip(nX0, nY0, nX1, nY1);
    }
}

// 0x005e9508
int ACanvas::SetupLineSteps(int nX0, int nY0, int nX1, int nY1, int *pnStepX, int *pnStepY) {
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

// 0x005ec050
void ACanvas::TextureRowIndexed(int nY,
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
        PutPixelIndexedNoClip(x, nY, *pPixel);
        pSourcePosition->mX += pSourceStep->mX;
        pSourcePosition->mY += pSourceStep->mY;
    }
}

// 0x005ec280
void ACanvas::Blit4NoClip(const ABitmap &source, int nX, int nY) {
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
            // Yes, the test compares a whole source byte against the key rather than the nibble
            // just consumed, and it reads the byte the walk has already advanced to.
            if (source.mHasTransparentColor == 0 || *pByte != source.mTransparentColor) {
                PutPixelIndexedNoClip(x, y, nIndex);
            }
        }
        pRow += source.mBytesPerRow;
    }
}

// 0x005ec3c0
void ACanvas::Blit4(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        Blit4NoClip(clipped, nX, nY);
    }
}

// 0x005ec450
void ACanvas::Blit8NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned char *pPixel = SourceRow(source);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        for (int x = nX; x < nX + source.mWidth; ++x) {
            if (source.mHasTransparentColor == 0 || *pPixel != source.mTransparentColor) {
                PutPixelIndexedNoClip(x, y, *pPixel);
            }
            ++pPixel;
        }
        pPixel += source.mBytesPerRow - source.mWidth;
    }
}

// 0x005ec570
void ACanvas::Blit8(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        Blit8NoClip(clipped, nX, nY);
    }
}

// 0x005ec600
void ACanvas::Blit15NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned short *pPixel = static_cast<const unsigned short *>(source.mPixels);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        for (int x = nX; x < nX + source.mWidth; ++x) {
            if (source.mHasTransparentColor == 0 || *pPixel != source.mTransparentColor) {
                PutPixel15NoClip(x, y, *pPixel);
            }
            ++pPixel;
        }
        pPixel += source.mBytesPerRow / 2 - source.mWidth;
    }
}

// 0x005ec738
void ACanvas::Blit15(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        Blit15NoClip(clipped, nX, nY);
    }
}

// 0x005ec7c8
void ACanvas::Blit24NoClip(const ABitmap &source, int nX, int nY) {
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
                PutPixelRGBNoClip(x, y, pPixel);
            }
            pPixel += kRGBByteCount;
        }
        pPixel += source.mBytesPerRow - kRGBByteCount * source.mWidth;
    }
}

// 0x005ec928
void ACanvas::Blit24(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        Blit24NoClip(clipped, nX, nY);
    }
}

// 0x005ec9b8
void ACanvas::Blit32NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned int *pPixel = static_cast<const unsigned int *>(source.mPixels);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        for (int x = nX; x < nX + source.mWidth; ++x) {
            if (source.mHasTransparentColor == 0 || source.mTransparentColor != *pPixel) {
                PutPixelNoClip(x, y, *pPixel);
            }
            ++pPixel;
        }
        pPixel += source.mBytesPerRow / 4 - source.mWidth;
    }
}

// 0x005ecad8
void ACanvas::Blit32(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        Blit32NoClip(clipped, nX, nY);
    }
}

// 0x005ecb68
void ACanvas::BlitRle8NoClip(const ABitmap &source, int nX, int nY) {
    ABitmap row(g_abCanvasRowScratch,
                kABitmapFormatLinear8,
                source.mHasTransparentColor != 0,
                source.mWidth,
                1,
                0);
    row.mTransparentColor = source.mTransparentColor;
    row.mPalette = source.mPalette;
    ARleReader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    for (int y = nY; y < nY + source.mHeight; ++y) {
        reader.DecodeRow(g_abCanvasRowScratch);
        Blit8NoClip(row, nX, y);
    }
}

// 0x005e9cb8
void ACanvas::BlitRle8(const ABitmap &source, int nX, int nY) {
    if (nY >= mClip.mTop && mClip.mBottom >= nY + source.mHeight && nX >= mClip.mLeft &&
        mClip.mRight >= nX + source.mWidth) {
        BlitRle8NoClip(source, nX, nY);
        return;
    }

    ARleReader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    AClipSpan clip;
    if (ClipBlitSpan(source, &nX, &nY, &reader, &clip) == 0) {
        return;
    }

    ABitmap row(g_abCanvasRowScratch,
                kABitmapFormatLinear8,
                source.mHasTransparentColor != 0,
                source.mWidth,
                1,
                0);
    row.mPixels = g_abCanvasRowScratch + clip.mSkipLeft;
    row.mWidth = static_cast<short>(clip.mStopColumn - clip.mSkipLeft);
    row.mTransparentColor = source.mTransparentColor;
    row.mPalette = source.mPalette;
    for (int y = nY; y < clip.mStopRow; ++y) {
        reader.DecodeRow(g_abCanvasRowScratch);
        Blit8NoClip(row, nX, y);
    }
}

// 0x005ecef0
void ACanvas::ReadRect4(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        ReadRect4NoClip(clipped, nX, nY);
    }
}

// 0x005ecdb8
void ACanvas::ReadRect4NoClip(const ABitmap &dest, int nX, int nY) {
    unsigned char *pRow = DestRow(dest);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        unsigned char *pByte = pRow;
        unsigned int bHighNibble = dest.mOddNibbleStart;
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            const int nIndex = GetPixelIndexedNoClip(x, y);
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

// 0x005ed070
void ACanvas::ReadRect8(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        ReadRect8NoClip(clipped, nX, nY);
    }
}

// 0x005ecf80
void ACanvas::ReadRect8NoClip(const ABitmap &dest, int nX, int nY) {
    unsigned char *pPixel = DestRow(dest);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            *pPixel = static_cast<unsigned char>(GetPixelIndexedNoClip(x, y));
            ++pPixel;
        }
        pPixel += dest.mBytesPerRow - dest.mWidth;
    }
}

// 0x005ed208
void ACanvas::ReadRect15(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        ReadRect15NoClip(clipped, nX, nY);
    }
}

// 0x005ed100
void ACanvas::ReadRect15NoClip(const ABitmap &dest, int nX, int nY) {
    unsigned short *pPixel = static_cast<unsigned short *>(dest.mPixels);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            *pPixel = GetPixel15NoClip(x, y);
            ++pPixel;
        }
        pPixel += dest.mBytesPerRow / 2 - dest.mWidth;
    }
}

// 0x005ed398
void ACanvas::ReadRect24(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        ReadRect24NoClip(clipped, nX, nY);
    }
}

// 0x005ed298
void ACanvas::ReadRect24NoClip(const ABitmap &dest, int nX, int nY) {
    unsigned char *pPixel = DestRow(dest);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            GetPixelRGBNoClip(x, y, pPixel);
            pPixel += kRGBByteCount;
        }
        pPixel += dest.mBytesPerRow - kRGBByteCount * dest.mWidth;
    }
}

// 0x005ed520
void ACanvas::ReadRect32(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        ReadRect32NoClip(clipped, nX, nY);
    }
}

// 0x005ed428
void ACanvas::ReadRect32NoClip(const ABitmap &dest, int nX, int nY) {
    unsigned int *pPixel = static_cast<unsigned int *>(dest.mPixels);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            *pPixel = GetPixelNoClip(x, y);
            ++pPixel;
        }
        pPixel += dest.mBytesPerRow / 4 - dest.mWidth;
    }
}

// 0x005e9ee8
void ACanvas::DrawGlyphNoClip(int nCharCode, const AFont *pFont, int nX, int nY) {
    const ABitmap *pGlyph = GlyphForCode(pFont, nCharCode);
    (this->*kCopyNoClipForFormat[pGlyph->mFormat])(*pGlyph, nX, nY - pFont->mBaseline);
}

// 0x005e9fc8
void ACanvas::DrawGlyph(int nCharCode, const AFont *pFont, int nX, int nY) {
    const ABitmap *pGlyph = GlyphForCode(pFont, nCharCode);
    // The compiled copy is 0x1c bytes, four more than an ABitmap. See the note in abitmap.h.
    ABitmap clipped = *pGlyph;
    nY -= pFont->mBaseline;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        (this->*kCopyForFormat[clipped.mFormat])(clipped, nX, nY);
    }
}

// 0x005ed5b0
void ACanvas::DrawTextNoClip(const char *pText, const AFont *pFont, int nX, int nY) {
    const int nStartX = nX;
    for (unsigned char ch = *pText++; ch != 0; ch = *pText++) {
        if (ch == kNewline) {
            nY += pFont->mLineHeight;
            nX = nStartX;
            continue;
        }
        DrawGlyphNoClip(ch, pFont, nX, nY);
        nX += GlyphForCode(pFont, ch)->mWidth;
    }
}

// 0x005ea120
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
        DrawGlyph(ch, pFont, nX, nY);
        nX += GlyphForCode(pFont, ch)->mWidth;
    }
}

// 0x005ed858
void ACanvas::BlitRemap4(const ABitmap &source, int nX, int nY, const unsigned char *pRemap) {
    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = g_abCanvasRowScratch;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    const unsigned char *pRow = SourceRow(source);
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        UnpackNibbleRow(pRow, g_abCanvasRowScratch, source.mWidth, source.mOddNibbleStart);
        RemapRowIndexed(span, pRemap);
        pRow += source.mBytesPerRow;
    }
}

// 0x005eda30
void ACanvas::BlitRemap8(const ABitmap &source, int nX, int nY, const unsigned char *pRemap) {
    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = const_cast<unsigned char *>(SourceRow(source));
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        RemapRowIndexed(span, pRemap);
        span.mSource += source.mBytesPerRow;
    }
}

// 0x005edd08
void ACanvas::RemapRowIndexed(const ARowSpan &span, const unsigned char *pRemap) {
    const unsigned char *pPixel = span.mSource;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned int nIndex = *pPixel++;
        if (!span.mHasTransparentColor || nIndex != span.mTransparentColor) {
            PutPixelIndexedNoClip(x, span.mY, pRemap[nIndex]);
        }
    }
}

// 0x005edfb8
void ACanvas::BlitBlend4(const ABitmap &source,
                         int nX,
                         int nY,
                         const unsigned char *const *ppBlend) {
    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = g_abCanvasRowScratch;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    const unsigned char *pRow = SourceRow(source);
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        UnpackNibbleRow(pRow, g_abCanvasRowScratch, source.mWidth, source.mOddNibbleStart);
        BlendRowIndexed(span, ppBlend);
        pRow += source.mBytesPerRow;
    }
}

// 0x005ee1c8
void ACanvas::BlitBlend8(const ABitmap &source,
                         int nX,
                         int nY,
                         const unsigned char *const *ppBlend) {
    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = const_cast<unsigned char *>(SourceRow(source));
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        BlendRowIndexed(span, ppBlend);
        span.mSource += source.mBytesPerRow;
    }
}

// 0x005ee4a0
void ACanvas::BlendRowIndexed(const ARowSpan &span, const unsigned char *const *ppBlend) {
    const unsigned char *pPixel = span.mSource;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned int nIndex = *pPixel++;
        // Index zero is the transparent one here, unlike RemapRowIndexed(), which compares
        // against the span transparent colour.
        if (nIndex == 0 && span.mHasTransparentColor) {
            continue;
        }
        const int nDestIndex = GetPixelIndexedNoClip(x, span.mY);
        PutPixelIndexedNoClip(x, span.mY, ppBlend[nIndex][nDestIndex]);
    }
}

// 0x005ee968
void ACanvas::StretchRowIndexed(const AStretchSpan &span) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 || nIndex != span.mTransparentColor) {
            PutPixelIndexedNoClip(x, span.mY, nIndex);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eea18
void ACanvas::StretchRow15(const AStretchSpan &span) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned short nColor = *reinterpret_cast<const unsigned short *>(
            span.mSource + 2 * (nPosition >> kACanvasFractionBits));
        if (span.mHasTransparentColor == 0 || nColor != span.mTransparentColor) {
            PutPixel15NoClip(x, span.mY, nColor);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eeac8
void ACanvas::StretchRow24(const AStretchSpan &span) {
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
            PutPixelRGBNoClip(x, span.mY, pRGB);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eebb8
void ACanvas::StretchRow32(const AStretchSpan &span) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned int nColor = *reinterpret_cast<const unsigned int *>(
            span.mSource + 4 * (nPosition >> kACanvasFractionBits));
        if (span.mHasTransparentColor == 0 || span.mTransparentColor != nColor) {
            PutPixelNoClip(x, span.mY, nColor);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eedb8
void ACanvas::StretchRowRemap(const AStretchSpan &span, const unsigned char *pRemap) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 || nIndex != span.mTransparentColor) {
            PutPixelIndexedNoClip(x, span.mY, pRemap[nIndex]);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eefc0
void ACanvas::StretchRowBlend(const AStretchSpan &span, const unsigned char *const *ppBlend) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 || nIndex != span.mTransparentColor) {
            const int nDestIndex = GetPixelIndexedNoClip(x, span.mY);
            PutPixelIndexedNoClip(x, span.mY, ppBlend[nIndex][nDestIndex]);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eddb8
void ACanvas::UnpackNibbleRow(const unsigned char *pSource,
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

// 0x005ed6a8
// A format the chain does not test draws nothing at all, rather than falling back to a
// generic path.
void ACanvas::BlitRemapNoClip(const ABitmap &source, int nX, int nY, const unsigned char *pRemap) {
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        BlitRemap4(source, nX, nY, pRemap);
        break;
    case kABitmapFormatLinear8:
        BlitRemap8(source, nX, nY, pRemap);
        break;
    case kABitmapFormatRle8:
        BlitRemapRle8NoClip(source, nX, nY, pRemap);
        break;
    default:
        break;
    }
}

// 0x005ed718
// The description is copied before clipping, because ClipBlitToRect() rewrites the one
// it is given.
void ACanvas::BlitRemap(const ABitmap &source, int nX, int nY, const unsigned char *pRemap) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) == 0) {
        return;
    }
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        BlitRemap4(clipped, nX, nY, pRemap);
        break;
    case kABitmapFormatLinear8:
        BlitRemap8(clipped, nX, nY, pRemap);
        break;
    case kABitmapFormatRle8:
        BlitRemapRle8(clipped, nX, nY, pRemap);
        break;
    default:
        break;
    }
}

// 0x005edbd8
void ACanvas::BlitRemapRle8NoClip(const ABitmap &source,
                                  int nX,
                                  int nY,
                                  const unsigned char *pRemap) {
    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = g_abCanvasRowScratch;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    ARleReader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    // Yes, the binary advances nY rather than span.mY, so the bound recedes with the row and a
    // source with any rows never ends. BlitRemapNoClip() has no caller, so the loop never runs.
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++nY) {
        reader.DecodeRow(g_abCanvasRowScratch);
        RemapRowIndexed(span, pRemap);
    }
}

// 0x005ea280
// The clipping matches BlitRle8(), without its test for a source wholly inside the
// clip rectangle.
void ACanvas::BlitRemapRle8(const ABitmap &source, int nX, int nY, const unsigned char *pRemap) {
    ARleReader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    AClipSpan clip;
    if (ClipBlitSpan(source, &nX, &nY, &reader, &clip) == 0) {
        return;
    }

    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(nX + (clip.mStopColumn - clip.mSkipLeft));
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = g_abCanvasRowScratch + clip.mSkipLeft;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    for (span.mY = static_cast<short>(nY); span.mY < clip.mStopRow; ++span.mY) {
        reader.DecodeRow(g_abCanvasRowScratch);
        RemapRowIndexed(span, pRemap);
    }
}

// 0x005ede08
void ACanvas::BlitBlendNoClip(const ABitmap &source,
                              int nX,
                              int nY,
                              const unsigned char *const *ppBlend) {
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        BlitBlend4(source, nX, nY, ppBlend);
        break;
    case kABitmapFormatLinear8:
        BlitBlend8(source, nX, nY, ppBlend);
        break;
    case kABitmapFormatRle8:
        BlitBlendRle8NoClip(source, nX, nY, ppBlend);
        break;
    default:
        break;
    }
}

// 0x005ede78
void ACanvas::BlitBlend(const ABitmap &source,
                        int nX,
                        int nY,
                        const unsigned char *const *ppBlend) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) == 0) {
        return;
    }
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        BlitBlend4(clipped, nX, nY, ppBlend);
        break;
    case kABitmapFormatLinear8:
        BlitBlend8(clipped, nX, nY, ppBlend);
        break;
    case kABitmapFormatRle8:
        BlitBlendRle8(clipped, nX, nY, ppBlend);
        break;
    default:
        break;
    }
}

// 0x005ee370
// Instruction for instruction BlitRemapRle8NoClip() with the blend slot called in
// place of the remap slot.
void ACanvas::BlitBlendRle8NoClip(const ABitmap &source,
                                  int nX,
                                  int nY,
                                  const unsigned char *const *ppBlend) {
    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = g_abCanvasRowScratch;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    ARleReader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    // Yes, the binary advances nY rather than span.mY here as well, and BlitBlendNoClip() has no
    // caller either.
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++nY) {
        reader.DecodeRow(g_abCanvasRowScratch);
        BlendRowIndexed(span, ppBlend);
    }
}

// 0x005ea460
// Instruction for instruction BlitRemapRle8() with the blend slot called in place of
// the remap slot.
void ACanvas::BlitBlendRle8(const ABitmap &source,
                            int nX,
                            int nY,
                            const unsigned char *const *ppBlend) {
    ARleReader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    AClipSpan clip;
    if (ClipBlitSpan(source, &nX, &nY, &reader, &clip) == 0) {
        return;
    }

    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(nX + (clip.mStopColumn - clip.mSkipLeft));
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = g_abCanvasRowScratch + clip.mSkipLeft;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    for (span.mY = static_cast<short>(nY); span.mY < clip.mStopRow; ++span.mY) {
        reader.DecodeRow(g_abCanvasRowScratch);
        BlendRowIndexed(span, ppBlend);
    }
}

// 0x005ecc68
void ACanvas::ReadRectNoClip(const ABitmap &dest, int nX, int nY) {
    // 0x0077dcf8
    static const ABitmapCopyMember kReadNoClipForFormat[kABitmapFormatCount] = {
        &ACanvas::ReadRect4NoClip,
        &ACanvas::ReadRect8NoClip,
        &ACanvas::ReadRect15NoClip,
        &ACanvas::ReadRect24NoClip,
        &ACanvas::ReadRect32NoClip,
        &ACanvas::ReadRectRle8};
    (this->*kReadNoClipForFormat[dest.mFormat])(dest, nX, nY);
}

// 0x005ecd10
void ACanvas::ReadRect(const ABitmap &dest, int nX, int nY) {
    // 0x0077dd28
    static const ABitmapCopyMember kReadForFormat[kABitmapFormatCount] = {&ACanvas::ReadRect4,
                                                                          &ACanvas::ReadRect8,
                                                                          &ACanvas::ReadRect15,
                                                                          &ACanvas::ReadRect24,
                                                                          &ACanvas::ReadRect32,
                                                                          &ACanvas::ReadRectRle8};
    (this->*kReadForFormat[dest.mFormat])(dest, nX, nY);
}

// 0x005eb190
void ACanvas::ReadRectRle8([[maybe_unused]] const ABitmap &dest,
                           [[maybe_unused]] int nX,
                           [[maybe_unused]] int nY) {
}

// 0x005ea780
int ACanvas::SetupStretchBlit(const ABitmap &source, const ARect &rect, AStretchBlit *pBlit) const {
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
            pBlit->mPalette = g_pDefaultPalette;
        }
    }
    pBlit->mHasTransparentColor = source.mHasTransparentColor;
    pBlit->mTransparentColor = source.mTransparentColor;
    return pBlit->mTop < pBlit->mBottom && pBlit->mLeft < pBlit->mRight;
}

// 0x005ee580
void ACanvas::StretchBlit(const ABitmap &source, const ARect &rect) {
    // 0x0077dd58
    static const ABitmapStretchMember kStretchForFormat[kABitmapFormatCount] = {
        &ACanvas::StretchBlit4,
        &ACanvas::StretchBlit8,
        &ACanvas::StretchBlit15,
        &ACanvas::StretchBlit24,
        &ACanvas::StretchBlit32,
        &ACanvas::StretchBlitRle8};
    (this->*kStretchForFormat[source.mFormat])(source, rect);
}

// 0x005ea640
void ACanvas::StretchBlit4(const ABitmap &source, const ARect &rect) {
    AStretchBlit blit;
    if (SetupStretchBlit(source, rect, &blit) == 0) {
        return;
    }
    blit.mSource = g_abCanvasRowScratch;
    // Yes, the walk starts at the first source row rather than at the row SetupStretchBlit()
    // selected, so a rectangle clipped at the top samples from too high in the source.
    const unsigned char *pRow = SourceRow(source);
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        UnpackNibbleRow(pRow, g_abCanvasRowScratch, source.mWidth, source.mOddNibbleStart);
        StretchRowIndexed(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        pRow += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// 0x005ee628
void ACanvas::StretchBlit8(const ABitmap &source, const ARect &rect) {
    AStretchBlit blit;
    if (SetupStretchBlit(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        StretchRowIndexed(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// 0x005ee6f8
void ACanvas::StretchBlit15(const ABitmap &source, const ARect &rect) {
    AStretchBlit blit;
    if (SetupStretchBlit(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        StretchRow15(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// 0x005ee7c8
void ACanvas::StretchBlit24(const ABitmap &source, const ARect &rect) {
    AStretchBlit blit;
    if (SetupStretchBlit(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        StretchRow24(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// 0x005ee898
void ACanvas::StretchBlit32(const ABitmap &source, const ARect &rect) {
    AStretchBlit blit;
    if (SetupStretchBlit(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        StretchRow32(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// 0x005ea960
void ACanvas::StretchBlitRle8(const ABitmap &source, const ARect &rect) {
    AStretchBlit blit;
    if (SetupStretchBlit(source, rect, &blit) == 0) {
        return;
    }
    ARleReader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    if (nRow != 0) {
        reader.SkipRows(nRow);
    }
    reader.DecodeRow(g_abCanvasRowScratch);
    blit.mSource = g_abCanvasRowScratch;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        StretchRowIndexed(blit);
        const int nNextRow = AdvanceStretchRow(&blit);
        if (nNextRow != nRow) {
            if (nNextRow - nRow != 1) {
                reader.SkipRows(nNextRow - nRow - 1);
            }
            reader.DecodeRow(g_abCanvasRowScratch);
        }
        nRow = nNextRow;
    }
}

// 0x005eec70
void ACanvas::StretchBlitRemap(const ABitmap &source,
                               const ARect &rect,
                               const unsigned char *pRemap) {
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        StretchBlitRemap4(source, rect, pRemap);
        break;
    case kABitmapFormatLinear8:
        StretchBlitRemap8(source, rect, pRemap);
        break;
    case kABitmapFormatRle8:
        StretchBlitRemapRle8(source, rect, pRemap);
        break;
    default:
        break;
    }
}

// 0x005eecd0
void ACanvas::StretchBlitRemap4([[maybe_unused]] const ABitmap &source,
                                [[maybe_unused]] const ARect &rect,
                                [[maybe_unused]] const unsigned char *pRemap) {
}

// 0x005eecd8
void ACanvas::StretchBlitRemap8(const ABitmap &source,
                                const ARect &rect,
                                const unsigned char *pRemap) {
    AStretchBlit blit;
    if (SetupStretchBlit(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        StretchRowRemap(blit, pRemap);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// 0x005eaa98
void ACanvas::StretchBlitRemapRle8(const ABitmap &source,
                                   const ARect &rect,
                                   const unsigned char *pRemap) {
    AStretchBlit blit;
    if (SetupStretchBlit(source, rect, &blit) == 0) {
        return;
    }
    ARleReader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    if (nRow != 0) {
        reader.SkipRows(nRow);
    }
    reader.DecodeRow(g_abCanvasRowScratch);
    blit.mSource = g_abCanvasRowScratch;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        StretchRowRemap(blit, pRemap);
        const int nNextRow = AdvanceStretchRow(&blit);
        if (nNextRow != nRow) {
            if (nNextRow - nRow != 1) {
                reader.SkipRows(nNextRow - nRow - 1);
            }
            reader.DecodeRow(g_abCanvasRowScratch);
        }
        nRow = nNextRow;
    }
}

// 0x005eee78
void ACanvas::StretchBlitBlend(const ABitmap &source,
                               const ARect &rect,
                               const unsigned char *const *ppBlend) {
    // Yes, the binary exchanges the first two arms, so a four bit source takes the eight bit path
    // and an eight bit source takes the empty one.
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        StretchBlitBlend8(source, rect, ppBlend);
        break;
    case kABitmapFormatLinear8:
        StretchBlitBlend4(source, rect, ppBlend);
        break;
    case kABitmapFormatRle8:
        StretchBlitBlendRle8(source, rect, ppBlend);
        break;
    default:
        break;
    }
}

// 0x005eeed8
void ACanvas::StretchBlitBlend4([[maybe_unused]] const ABitmap &source,
                                [[maybe_unused]] const ARect &rect,
                                [[maybe_unused]] const unsigned char *const *ppBlend) {
}

// 0x005eeee0
void ACanvas::StretchBlitBlend8(const ABitmap &source,
                                const ARect &rect,
                                const unsigned char *const *ppBlend) {
    AStretchBlit blit;
    if (SetupStretchBlit(source, rect, &blit) == 0) {
        return;
    }
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        StretchRowBlend(blit, ppBlend);
        const int nNextRow = AdvanceStretchRow(&blit);
        blit.mSource += (nNextRow - nRow) * source.mBytesPerRow;
        nRow = nNextRow;
    }
}

// 0x005eabe0
void ACanvas::StretchBlitBlendRle8(const ABitmap &source,
                                   const ARect &rect,
                                   const unsigned char *const *ppBlend) {
    AStretchBlit blit;
    if (SetupStretchBlit(source, rect, &blit) == 0) {
        return;
    }
    ARleReader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    int nRow = blit.mSourcePositionY >> kACanvasFractionBits;
    if (nRow != 0) {
        reader.SkipRows(nRow);
    }
    reader.DecodeRow(g_abCanvasRowScratch);
    blit.mSource = g_abCanvasRowScratch;
    for (blit.mY = blit.mTop; blit.mY < blit.mBottom; ++blit.mY) {
        StretchRowBlend(blit, ppBlend);
        const int nNextRow = AdvanceStretchRow(&blit);
        if (nNextRow != nRow) {
            if (nNextRow - nRow != 1) {
                reader.SkipRows(nNextRow - nRow - 1);
            }
            reader.DecodeRow(g_abCanvasRowScratch);
        }
        nRow = nNextRow;
    }
}
