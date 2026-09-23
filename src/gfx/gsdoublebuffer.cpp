#include "gfx/gsdoublebuffer.h"

#include <gs_privileged.h>
#include <libgraph.h>

namespace {

// Pixel storage formats whose bytes per pixel SetDefaults() distinguishes. Any other format is
// counted at four bytes.
constexpr short kGsPsmCt32 = 0;
constexpr short kGsPsmCt24 = 1;
constexpr short kGsPsmCt16 = 2;
constexpr int kBytesPerPixelCt32 = 4;
constexpr int kBytesPerPixelCt24 = 3;
constexpr int kBytesPerPixelCt16 = 2;

// One GS page is 8 KiB, and the page count is the byte count shifted down arithmetically.
constexpr int kGsPageShift = 13;

// The depth buffer follows two frame buffers of equal size.
constexpr int kFrameBufferCount = 2;

// PMODE read circuit enables and the blend value SetDispEnvs() writes into ALP.
constexpr unsigned long long kPmodeEn1 = 1;
constexpr unsigned long long kPmodeEn2 = 2;
constexpr int kPmodeAlpShift = 8;
constexpr unsigned long long kPmodeAlpMask = 0xff;
constexpr unsigned long long kPmodeHalfBlend = 0x80;

// DISPFB fields.
constexpr int kDispFbFbpShift = 0;
constexpr unsigned long long kDispFbFbpMask = 0x1ff;
constexpr int kDispFbDbyShift = 43;
constexpr unsigned long long kDispFbDbyMask = 0x7ff;

// DISPLAY fields.
constexpr int kDisplayDxShift = 0;
constexpr unsigned long long kDisplayDxMask = 0xfff;
constexpr int kDisplayDhShift = 44;
constexpr unsigned long long kDisplayDhMask = 0x7ff;

// How read circuit 2 differs from read circuit 1.
constexpr unsigned long long kCircuit2LineOffset = 1;
constexpr unsigned long long kCircuit2HeightReduction = 1;
constexpr unsigned long long kCircuit2XOffset = 2;

// FRAME_1 and ZBUF_1 fields.
constexpr unsigned long long kFrameFbpMask = 0x1ff;
constexpr int kZbufPsmShift = 24;
constexpr unsigned long long kZbufPsmMask = 0xf;
constexpr unsigned long long kZbufZmsk = 1ULL << 32;

// GIFtag fields, all in the low word except the register list.
constexpr unsigned long long kGifTagNLoopMask = 0x7fff;
constexpr unsigned long long kGifTagEop = 1ULL << 15;
constexpr int kGifTagNRegShift = 60;
constexpr unsigned long long kGifTagNRegMask = 0xf;
constexpr unsigned long long kGifTagRegsMask = 0xf;
constexpr unsigned long long kGifRegAd = 0xe;

// Register pairs a draw environment sends with and without its clear.
constexpr unsigned long long kDrawEnvPairs = 8;
constexpr unsigned long long kClearPairs = 6;
constexpr unsigned long long kDrawAndClearPairs = kDrawEnvPairs + kClearPairs;

// The clear rectangle is centred on the GS primitive coordinate origin.
constexpr short kGsCoordinateCentre = 0x800;

inline unsigned long long WithField(unsigned long long qwValue,
                                    int nShift,
                                    unsigned long long qwMask,
                                    unsigned long long qw) {
    return (qwValue & ~(qwMask << nShift)) | ((qw & qwMask) << nShift);
}

inline unsigned long long
GetField(unsigned long long qwValue, int nShift, unsigned long long qwMask) {
    return (qwValue >> nShift) & qwMask;
}

inline void
BuildDispEnv(GsDoubleBuffer::DispEnv &env, short nWidth, short nHeight, short nPsm, short nFbp) {
    sceGsSetDefDispEnv(&env.mEnv, nPsm, nWidth, nHeight, 0, 0);

    const unsigned long long qwDisplay = env.mEnv.display;
    env.mEnv.display =
        WithField(qwDisplay,
                  kDisplayDhShift,
                  kDisplayDhMask,
                  GetField(qwDisplay, kDisplayDhShift, kDisplayDhMask) - kCircuit2HeightReduction);
    env.mEnv.pmode = WithField(env.mEnv.pmode, kPmodeAlpShift, kPmodeAlpMask, kPmodeHalfBlend);
    const unsigned long long qwDispFb =
        WithField(env.mEnv.dispfb, kDispFbFbpShift, kDispFbFbpMask, static_cast<unsigned>(nFbp));

    env.mEnv.pmode |= kPmodeEn1 | kPmodeEn2;
    env.mEnv.dispfb = WithField(qwDispFb, kDispFbDbyShift, kDispFbDbyMask, kCircuit2LineOffset);
    env.mDispFb1 = qwDispFb;
    env.mDisplay1 = qwDisplay;
    env.mEnv.display =
        WithField(env.mEnv.display,
                  kDisplayDxShift,
                  kDisplayDxMask,
                  GetField(env.mEnv.display, kDisplayDxShift, kDisplayDxMask) + kCircuit2XOffset);
}

inline void BuildDrawHalf(GsDoubleBuffer::DrawHalf &half,
                          short nWidth,
                          short nHeight,
                          short nPsm,
                          short nFbp,
                          unsigned long long qwZbuf,
                          short nZTest,
                          short nZPsm) {
    sceGsSetDefDrawEnv(&half.mDraw, nPsm, nWidth, nHeight, nZTest, nZPsm);
    half.mDraw.frame1 =
        (half.mDraw.frame1 & ~kFrameFbpMask) | (static_cast<unsigned>(nFbp) & kFrameFbpMask);
    half.mDraw.zbuf1 = qwZbuf;

    half.mGifTag = {};
    unsigned long long qwTag = half.mGifTag.mWords[0];
    qwTag = (qwTag & ~kGifTagNLoopMask) | kDrawAndClearPairs | kGifTagEop;
    qwTag = WithField(qwTag, kGifTagNRegShift, kGifTagNRegMask, 1);
    half.mGifTag.mWords[0] = qwTag;
    half.mGifTag.mWords[1] = (half.mGifTag.mWords[1] & ~kGifTagRegsMask) | kGifRegAd;

    sceGsSetDefClear(&half.mClear,
                     nZTest,
                     static_cast<short>(kGsCoordinateCentre - (nWidth >> 1)),
                     static_cast<short>(kGsCoordinateCentre - (nHeight >> 1)),
                     nWidth,
                     nHeight,
                     0,
                     0,
                     0,
                     0,
                     0);
}

inline void WriteDisplayRegisters(GsDoubleBuffer::DispEnv &env, int bEnableCircuit1) {
    if (bEnableCircuit1 != 0) {
        env.mEnv.pmode |= kPmodeEn1;
    } else {
        env.mEnv.pmode &= ~kPmodeEn1;
    }
    env.mEnv.pmode |= kPmodeEn2;

    *GS_REG_PMODE = env.mEnv.pmode;
    *GS_REG_SMODE2 = env.mEnv.smode2;
    *GS_REG_DISPFB2 = env.mEnv.dispfb;
    *GS_REG_DISPLAY2 = env.mEnv.display;
    *GS_REG_BGCOLOR = env.mEnv.bgcolor;
    *GS_REG_DISPLAY1 = env.mDisplay1;
    *GS_REG_DISPFB1 = env.mDispFb1;
}

} // namespace

// 0x0058e9b0
void GsDoubleBuffer::SetDefaults(
    short nWidth, short nHeight, short nPsm, short nZTest, short nZPsm, short nClear) {
    mWidth = nWidth;
    mHeight = nHeight;
    mPsm = nPsm;
    mZPsm = nZPsm;

    int nBytesPerPixel;
    switch (nPsm) {
    case kGsPsmCt32:
        nBytesPerPixel = kBytesPerPixelCt32;
        break;
    case kGsPsmCt24:
        nBytesPerPixel = kBytesPerPixelCt24;
        break;
    case kGsPsmCt16:
        nBytesPerPixel = kBytesPerPixelCt16;
        break;
    default:
        nBytesPerPixel = kBytesPerPixelCt32;
        break;
    }

    const int nPages = (nWidth * nHeight * nBytesPerPixel) >> kGsPageShift;
    mFbp0 = 0;
    mZbp = static_cast<short>(nPages * kFrameBufferCount);
    mFbp1 = static_cast<short>(nPages);
    SetDispEnvs(nWidth, nHeight, nPsm, mFbp0, mFbp1);
    SetDrawEnvs(nWidth, nHeight, nPsm, mFbp0, mFbp1, mZbp, nZTest, nZPsm, nClear);
}

// 0x0058e330
void GsDoubleBuffer::SetDispEnvs(
    short nWidth, short nHeight, short nPsm, short nFbp0, short nFbp1) {
    BuildDispEnv(mDisp[0], nWidth, nHeight, nPsm, nFbp0);
    BuildDispEnv(mDisp[1], nWidth, nHeight, nPsm, nFbp1);
}

// 0x0058e538
void GsDoubleBuffer::SetDrawEnvs(short nWidth,
                                 short nHeight,
                                 short nPsm,
                                 short nFbp0,
                                 short nFbp1,
                                 short nZbp,
                                 short nZTest,
                                 short nZPsm,
                                 [[maybe_unused]] short nClear) {
    // The page is sign-extended into the whole word, and a zero depth test also masks depth
    // writes.
    unsigned long long qwZbuf = static_cast<unsigned long long>(static_cast<long long>(nZbp)) |
                                ((static_cast<unsigned>(nZPsm) & kZbufPsmMask) << kZbufPsmShift);
    if (nZTest == 0) {
        qwZbuf |= kZbufZmsk;
    }
    BuildDrawHalf(mHalves[0], nWidth, nHeight, nPsm, nFbp0, qwZbuf, nZTest, nZPsm);
    BuildDrawHalf(mHalves[1], nWidth, nHeight, nPsm, nFbp1, qwZbuf, nZTest, nZPsm);
}

// 0x0058e7e8
void GsDoubleBuffer::PutDrawEnv(int nHalf, int bClear) {
    const unsigned long long qwLoops = bClear != 0 ? kDrawAndClearPairs : kDrawEnvPairs;
    mHalves[0].mGifTag.mWords[0] = (mHalves[0].mGifTag.mWords[0] & ~kGifTagNLoopMask) | qwLoops;
    mHalves[1].mGifTag.mWords[0] = (mHalves[1].mGifTag.mWords[0] & ~kGifTagNLoopMask) | qwLoops;
    if (nHalf == 0) {
        sceGsPutDrawEnv(&mHalves[0].mGifTag);
    } else {
        sceGsPutDrawEnv(&mHalves[1].mGifTag);
    }
}

// 0x0058e860
void GsDoubleBuffer::PutDispEnv(int nHalf, int bEnableCircuit1) {
    if (nHalf == 0) {
        WriteDisplayRegisters(mDisp[0], bEnableCircuit1);
    } else {
        WriteDisplayRegisters(mDisp[1], bEnableCircuit1);
    }
}
