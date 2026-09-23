#include "gfx/gfxdevice.h"

#include <cstdint>
#include <cstring>
#include <ee_regs.h>
#include <eekernel.h>
#include <libdma.h>
#include <libgraph.h>

#include "app/longop.h"
#include "gfx/gsdoublebuffer.h"
#include "gfx/renderstats.h"
#include "gfx/vramtable.h"
#include "os/failsink.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "profile/profiler.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/multimesh.h"
#include "rnd/particlesys.h"
#include "rnd/pscam.h"
#include "rnd/psenviron.h"
#include "rnd/psmat.h"
#include "rnd/psmesh.h"
#include "rnd/psmultimesh.h"
#include "rnd/psparticlesys.h"
#include "rnd/pstex.h"
#include "rnd/tex.h"

namespace {

// GIFtag fields the packet routines touch, all within the tag's low 64 bits. NLOOP occupies the
// low fifteen bits, EOP the next one, FLG bits 58 and 59, and NREG the top four.
constexpr unsigned long long kGifTagNLoopMask = 0x7fff;
constexpr int kGifTagEopShift = 15;
constexpr unsigned long long kGifTagEop = 1ULL << kGifTagEopShift;
constexpr int kGifTagFlgShift = 58;
constexpr unsigned long long kGifTagFlgMask = 3;
constexpr int kGifTagNRegShift = 60;

// The first register descriptor of a tag's REGS field, in the high 64 bits, and the value that
// makes the tag an A+D tag.
constexpr unsigned long long kGifTagFirstRegMask = 0xf;
constexpr unsigned long long kGifRegAd = 0xe;

// VIF codes the VU1 path places ahead of GIF data. FLUSHA waits for the paths to go idle, and
// DIRECT passes the quadword count in its immediate straight through to the GIF.
constexpr unsigned long long kVifCodeFlushA = 0x13000000;
constexpr unsigned long long kVifCodeDirect = 0x50000000;
constexpr int kVifCodeHighWordShift = 32;

// The two halves of the scratchpad the packet buffer alternates between.
constexpr std::uintptr_t kGifBufferHalf0 = 0x70000000;
constexpr std::uintptr_t kGifBufferHalf1 = 0x70002000;

// A buffer address becomes a DMA address by keeping the low 28 bits and moving the scratchpad
// selector, bit 30, up to bit 31, where the DMA controller reads it.
constexpr std::uintptr_t kDmaAddressMask = 0x0fffffff;
constexpr std::uintptr_t kScratchpadAddressBit = 0x40000000;

// PRIM and the primitive types that continue across vertices. A strip or a fan is restarted only
// by a PRIM write.
constexpr int kGsRegPrim = 0;
constexpr unsigned long long kGsPrimTypeMask = 7;
constexpr unsigned long long kGsPrimLineStrip = 2;
constexpr unsigned long long kGsPrimTriStrip = 4;
constexpr unsigned long long kGsPrimTriFan = 5;

// The context 1 registers RestoreFrameBufferTarget() restores, and the fields it sets. FRAME_1
// takes FBP, FBW, and PSM without FBMSK, and XYOFFSET_1 takes both offsets.
constexpr int kGsRegFrame1 = 0x4c;
constexpr int kGsRegXyOffset1 = 0x18;
constexpr unsigned long long kGsFrameMask = 0x3f3f01ff;
constexpr unsigned long long kGsXyOffsetMask = 0x0000ffff0000ffffULL;

// A GS page is 2048 words, and a word is four bytes.
constexpr int kGsPageWordShift = 11;
constexpr int kBytesPerWordShift = 2;

// Arguments to sceGsSyncPath() that wait for every path without a timeout.
constexpr int kGsSyncPathWait = 0;
constexpr unsigned short kGsSyncPathNoTimeout = 0;

// RGBAQ as the device packs a colour. Red, green, and blue scale to 255, alpha to 128, and Q is
// 1.0f in the upper word.
constexpr float kRgbaqColorScale = 255.0f;
constexpr float kRgbaqAlphaScale = 128.0f;
constexpr int kRgbaqGreenShift = 8;
constexpr int kRgbaqBlueShift = 16;
constexpr int kRgbaqAlphaShift = 24;
constexpr unsigned long long kRgbaqQOne = 0x3f800000ULL << 32;

// The value word of the RGBAQ pair, the third register pair of an sceGsClear.
constexpr int kClearRgbaqWord = 4;

// The write-back mode of FlushCache().
constexpr int kFlushCacheWriteBackData = 0;

// Registers and fields SetupGsDrawContext() programs.
constexpr int kGsRegTex0_1 = 0x06;
constexpr int kGsRegClamp1 = 0x08;
constexpr int kGsRegTex1_1 = 0x14;
constexpr int kGsRegTexA = 0x3b;
constexpr int kGsRegAlpha1 = 0x42;
constexpr int kGsRegDimX = 0x44;
constexpr int kGsRegTest1 = 0x47;
constexpr int kGsRegZbuf1 = 0x4e;
constexpr unsigned long long kAllBits = ~0ULL;
constexpr unsigned long long kTestZTestMask = 0x60000;
constexpr unsigned long long kTestZTestAlways = 0x20000;
constexpr unsigned long long kZbufZmsk = 1ULL << 32;
constexpr unsigned long long kAlphaMask = 0xff000000ffULL;
// A = Cs, B = Cd, C = FIX, and D = Cd. The source is blended over the destination by FIX.
constexpr unsigned long long kAlphaBlendByFix = 0x64;
constexpr int kAlphaFixShift = 32;
constexpr unsigned long long kDimXMatrix = 0x1212303012120303ULL;
constexpr unsigned long long kTex1MinMask = 0x1c0;
constexpr unsigned long long kTex1MinLinear = 0x40;
constexpr unsigned long long kClampMask = 0xf;
constexpr unsigned long long kClampBothAxes = 5;
constexpr unsigned long long kTexAHalfAlpha = 0x0000008000000080ULL;

// FRAME_1 fields the texture binding copies, and the TEX0_1 fields it fills. The texture is
// 1024 by 1024 with its alpha taken from the texture and applied as a decal.
constexpr unsigned long long kFrameFbpMask = 0x1ff;
constexpr int kFrameFbwShift = 16;
constexpr unsigned long long kFrameFbwMask = 0x3f;
constexpr int kFramePsmShift = 24;
constexpr unsigned long long kFramePsmMask = 0x3f;
constexpr int kBlocksPerPageShift = 5;
constexpr int kTex0TbwShift = 14;
constexpr int kTex0PsmShift = 20;
constexpr int kTex0TwShift = 26;
constexpr int kTex0ThShift = 30;
constexpr int kTex0TccShift = 34;
constexpr int kTex0TfxShift = 35;
constexpr unsigned long long kTex0Log2Size1024 = 10;
constexpr unsigned long long kTex0TccRgba = 1;
constexpr unsigned long long kTex0TfxDecal = 1;
constexpr unsigned long long kTex0Mask = 0xffffffffffULL;

// The feedback sprite travels under one REGLIST tag, PRIM and RGBAQ then two UV and XYZ2 pairs.
constexpr unsigned long long kFeedbackTagLo = 0x6400000000008000ULL;
constexpr unsigned long long kFeedbackTagHi = 0x535310;
// A textured, alpha-blended sprite with texel coordinates, drawn in mid grey at alpha 100.
constexpr unsigned long long kFeedbackPrim = 0x156;
constexpr unsigned long long kFeedbackRgbaq = 0x3f80000064808080ULL;
constexpr unsigned long long kFeedbackZ = 160000ULL << 32;
// Coordinates are in sixteenths. The primitive origin is 2048, and a texel is sampled at its
// centre.
constexpr int kGsCoordinateCentre = 0x8000;
constexpr int kGsSubpixelShift = 4;
constexpr int kHalfSizeToSubpixelShift = 3;
constexpr int kTexelCentre = 8;
constexpr int kGsYShift = 16;

inline unsigned long long PackRgbaq(const Color &color) {
    const int nRed = static_cast<int>(color.r * kRgbaqColorScale);
    const int nGreen = static_cast<int>(color.g * kRgbaqColorScale);
    const int nBlue = static_cast<int>(color.b * kRgbaqColorScale);
    const int nAlpha = static_cast<int>(color.a * kRgbaqAlphaScale);
    return static_cast<unsigned long long>(nRed) |
           (static_cast<unsigned long long>(nGreen) << kRgbaqGreenShift) |
           (static_cast<unsigned long long>(nBlue) << kRgbaqBlueShift) |
           (static_cast<unsigned long long>(nAlpha) << kRgbaqAlphaShift) | kRgbaqQOne;
}

inline unsigned long long PackCoordinates(int nX, int nY) {
    return static_cast<unsigned long long>(nX) | (static_cast<unsigned long long>(nY) << kGsYShift);
}

// The debug overlay draws at depth 0xffff, with coordinates in sixteenths of a pixel.
constexpr unsigned long long kDebugZ = 0xffffULL << 32;
constexpr float kSubpixelsPerPixel = 16.0f;
constexpr unsigned long long kGsPrimSpriteValue = 6;

// REGLIST tags the overlay opens. Text sends PRIM, RGBAQ, and six XYZ2 per glyph, and a bar sends
// PRIM, RGBAQ, and two XYZ2.
constexpr unsigned long long kTextTagLo = 0x8400000000000000ULL;
constexpr unsigned long long kTextTagHi = 0x55555510;
constexpr unsigned long long kBarTagLo = 0x4400000000000000ULL;
constexpr unsigned long long kBarTagHi = 0x5510;

// The stroke font. Letters of either case map to the first 26 glyphs and '.' through '9' to the
// last 12. A glyph is six points of a line strip, x then y, in units of the cell.
constexpr int kDebugGlyphLetters = 26;
constexpr int kDebugGlyphPunctuationAndDigits = 12;
constexpr int kDebugGlyphCount = kDebugGlyphLetters + kDebugGlyphPunctuationAndDigits;
constexpr int kDebugGlyphSegments = 3;
constexpr int kFloatsPerSegment = 4;
constexpr int kDebugGlyphFloats = kDebugGlyphSegments * kFloatsPerSegment;
constexpr int kBlankAdvanceCells = 2;
constexpr double kGlyphAdvanceCells = 1.5;

// 0x006f2f28
const float g_aafDebugGlyphStrokes[kDebugGlyphCount][kDebugGlyphFloats] = {
    {0.0f, 1.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f},
    {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f},
    {0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f},
    {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f},
    {0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, 0.0f, 0.5f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f},
    {0.5f, 1.0f, 1.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    {0.5f, 0.5f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f, 1.0f},
    {0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.5f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f, 1.0f},
    {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.5f, 0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f},
    {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f},
};

// Which glyph draws a character, or -1 for a character that only advances the pen.
inline int DebugGlyphIndex(char chText) {
    if (static_cast<unsigned>(chText - 'A') < kDebugGlyphLetters) {
        return chText - 'A';
    }
    if (static_cast<unsigned>(chText - 'a') < kDebugGlyphLetters) {
        return chText - 'a';
    }
    if (static_cast<unsigned>(chText - '.') < kDebugGlyphPunctuationAndDigits) {
        return chText - '.' + kDebugGlyphLetters;
    }
    return -1;
}

// Profiler records the overlay reads from the previous frame's copy.
constexpr int kProfileTimerSync = 18;
constexpr int kProfileTimerFrame = 19;

inline float TimerMilliseconds(const ProfileTimer &timer) {
    return static_cast<float>(timer.mCycles) * g_flCyclesToMilliseconds;
}

// Text and bars start 30 pixels in from the top left corner of the display, each line 20 pixels
// below the last, with 8-pixel text cells and 10-pixel bars.
constexpr int kOverlayOrigin = 0x81e;
constexpr int kOverlayLinePixels = 20;
constexpr float kOverlayLineSpacing = kOverlayLinePixels;
constexpr float kOverlayTextCell = 8.0f;
constexpr float kTimingBarHeight = 10.0f;
constexpr float kTimingBarWidthShare = 0.95f;
constexpr int kTimingTickMs = 5;
constexpr float kTimingTickWidth = 1.0f;
constexpr float kTimingBarGrey = 0.5f;
constexpr float kTimingTickGrey = 0.25f;
constexpr float kTimingLabelMinimumMs = 0.05f;
constexpr float kMillisecondsPerSecond = 1000.0f;
constexpr Color kOverlayWhite = {1.0f, 1.0f, 1.0f, 1.0f};

// The frame-rate readout averages over five frames and sits 192 pixels in from the right edge and
// 10 below the top.
constexpr int kFpsSampleFrames = 5;
constexpr int kFpsReadoutRightOffset = 0x740;
constexpr int kFpsReadoutTop = 0x80a;

// The saved packet covers one scratchpad half.
constexpr int kSavedPacketQuadwords = 0x200;

constexpr float kDefaultFeedbackAlpha = 0.78f;

// The context 1 registers SwapBuffers() copies from the draw environment, beside FRAME_1,
// ZBUF_1, and XYOFFSET_1 above.
constexpr int kGsRegPrModeCont = 0x1a;
constexpr int kGsRegScissor1 = 0x40;
constexpr int kGsRegDither = 0x45;
constexpr int kGsRegColClamp = 0x46;

// Init() titles six consecutive records of g_profileTimers for the device's own intervals.
constexpr int kFirstDeviceTimer = 8;
const char *const kapszDeviceTimerNames[] = {"setup", "vram", "billboard", "vert", "prim", "sync"};
constexpr int kBitsPerPixelByte = 8;

// The register shadow InitDisplayMode() seeds. Every byte starts all ones, and FOGCOL starts at 1.
constexpr int kRegShadowFillByte = 0xff;
constexpr int kGsRegFogCol = 0x3d;
constexpr unsigned long long kFogColInitial = 1;

// sceDmaReset() and sceGsResetGraph() arguments: enable DMA, and a full reset for NTSC interlaced
// output in field mode.
constexpr int kDmaResetEnable = 1;
constexpr short kGsResetFull = 0;
constexpr short kGsInterlace = 1;
constexpr short kGsNtsc = 2;
constexpr short kGsFieldMode = 0;

// Pixel depths the display supports, and the frame and depth buffer formats each selects.
constexpr int kDepth16 = 16;
constexpr int kDepth24 = 24;
constexpr int kDepth32 = 32;
constexpr short kPsmCt32 = 0;
constexpr short kPsmCt24 = 1;
constexpr short kPsmCt16 = 2;
constexpr short kPsmZ24 = 0x31;
constexpr short kPsmZ16 = 0x32;
constexpr short kPsmZ16S = 0x3a;
constexpr int kZBufferBytes16 = 2;
constexpr int kZBufferBytes24 = 3;
constexpr int kFallbackPixelBytes = 4;
// ZTST greater, and the clear flag SetDefaults() passes to both halves.
constexpr short kZTestGreater = 3;
constexpr short kClearOnSwap = 1;

// KSEG1-style uncached alias of a main memory address.
constexpr std::uintptr_t kUncachedAddressBit = 0x20000000;

// CHCR.TTE, which makes the VIF channels pass each DMA tag through as data.
constexpr unsigned int kChcrTransferTag = 0x40;

// The VIF MSCAL code that starts VU1's initialisation program at 0x3c0.
constexpr unsigned int kVifMscalInit = 0x140003c0;

// A buffer address becomes a DMA address by keeping the low 28 bits and moving the scratchpad
// selector up to bit 31.
inline void *ToDmaAddress(const void *pAddress) {
    const std::uintptr_t nAddress = reinterpret_cast<std::uintptr_t>(pAddress);
    // The DMA address is a bus address the processor cannot dereference, but the DMA library
    // takes it as a pointer.
    return reinterpret_cast<void *>((nAddress & kDmaAddressMask) |
                                    ((nAddress & kScratchpadAddressBit) << 1));
}

// 0x006f36c0
GsDoubleBuffer g_displayBuffers;

} // namespace

// 0x006f2a80
GfxDevice g_gfxDevice;

// 0x006f2f20
volatile int g_nVblankCounter;

// 0x0049ac50
GfxDevice::GfxDevice()
    : mpSavedWrite(nullptr), mpReservedRegion(nullptr),
      mSavedPacket(kSavedPacketQuadwords, GifQuadword()), mpOpenTag(nullptr), mnSwapVblank(0),
      mnDrawBuffer(0), mpDisplayBuffers(nullptr), mnUseVu1(0), mpOpenVifDirect(nullptr),
      mnFpsCountdown(kFpsSampleFrames), mflFrameMsSum(0.0f), mnFps(0), mflSyncMsSum(0.0f),
      mnSyncMsAverage(0), mFeedbackEnabled(0), mFeedbackRect{0.0f, 0.0f, 1.0f, 1.0f},
      mFeedbackAlpha(kDefaultFeedbackAlpha), mFeedbackInset(0),
      mClearColor{0.0f, 0.0f, 0.0f, 1.0f} {
    mAdTag.mLo = 1ULL << kGifTagNRegShift;
    mAdTag.mHi = kGifRegAd;
}

// 0x0049afe0
void GfxDevice::Terminate() {
    Rnd::RegisterMeshClass();
    Rnd::PsCam::Terminate();
    Rnd::RegisterMatClass();
    Rnd::RegisterTexClass();
    Rnd::PsEnviron::Terminate();
    Rnd::RegisterParticleSysClass();
    Rnd::RegisterMultiMeshClass();
    g_vramTable.~VramTable(); // Yes, the binary calls the destructor on the global directly.
}

// 0x0049fef0
void GfxDevice::ResetVramAndSavePacket() {
    g_vramTable.Clear(1);
    SavePacket();
}

// 0x0049ff28
void GfxDevice::SavePacket() {
    sceDmaSync(sceDmaGetChan(SCE_DMA_GIF), 0, 0);
    sceDmaSync(sceDmaGetChan(SCE_DMA_VIF1), 0, 0);
    std::memcpy(&mSavedPacket[0], mpBuffer, (mpWrite - mpBuffer) * sizeof(GifQuadword));
}

// 0x0049ff98
void GfxDevice::RestorePacket() {
    std::memcpy(mpBuffer, &mSavedPacket[0], (mpWrite - mpBuffer) * sizeof(GifQuadword));
}

// 0x004a00e8
inline void GfxDevice::SendPacket() {
    sceDmaChan *pChannel = sceDmaGetChan(mnUseVu1 != 0 ? SCE_DMA_VIF1 : SCE_DMA_GIF);
    sceDmaSendN(pChannel, ToDmaAddress(mpBuffer), static_cast<int>(mpWrite - mpBuffer));
}

// 0x004a0388
void GfxDevice::FlipFrameBuffer() {
    mpDisplayBuffers->PutDrawEnv(mnDrawBuffer, 1);
    SwapBuffers();
}

// 0x0049ae20
void GfxDevice::Init(int nWidth, int nHeight, int nBitDepth) {
    int nTimer = kFirstDeviceTimer;
    for (const char *pszName : kapszDeviceTimerNames) {
        g_profileTimers[nTimer].mName = HxStr(pszName);
        ++nTimer;
    }
    mnDisplayWidth = nWidth;
    mnDisplayHeight = nHeight;
    mnUseVu1 = 0;
    mnPixelBytes = nBitDepth / kBitsPerPixelByte;
    mpWrite = reinterpret_cast<GifQuadword *>(kGifBufferHalf0);
    mpBuffer = mpWrite;
    InitDisplayMode();

    Rnd::g_pfnNewMesh = Rnd::NewPsMesh;
    Rnd::PsCam::Init();
    Rnd::PsMat::InstallCreator();
    Rnd::PsTex::StaticInit();
    Rnd::PsEnviron::Init();
    Rnd::g_pfnNewParticleSys = Rnd::NewPsParticleSys;
    Rnd::g_pfnNewMultiMesh = Rnd::NewPsMultiMesh;
    g_vramTable.Init();
}

// 0x0049b138
void GfxDevice::InitDisplayMode() {
    std::memset(mGsRegs, kRegShadowFillByte, sizeof(mGsRegs));
    mGsRegs[kGsRegFogCol] = kFogColInitial;
    sceGsResetPath();
    sceDmaReset(kDmaResetEnable);
    sceGsSyncPath(0, 0);
    sceGsSyncV(0);
    sceGsResetGraph(kGsResetFull, kGsInterlace, kGsNtsc, kGsFieldMode);

    short nPsm = kPsmCt32;
    short nZPsm = kPsmZ24;
    switch (mnPixelBytes * kBitsPerPixelByte) {
    case kDepth24:
        mnDepthBytes = kZBufferBytes16;
        nPsm = kPsmCt24;
        nZPsm = kPsmZ16S;
        break;
    case kDepth16:
        mnDepthBytes = kZBufferBytes16;
        nPsm = kPsmCt16;
        nZPsm = kPsmZ16;
        break;
    case kDepth32:
        mnDepthBytes = kZBufferBytes24;
        break;
    default:
        g_failSink.Format("Unsupported video mode\n");
        mnDepthBytes = kZBufferBytes24;
        mnPixelBytes = kFallbackPixelBytes;
        break;
    }

    mpDisplayBuffers = reinterpret_cast<GsDoubleBuffer *>(
        reinterpret_cast<std::uintptr_t>(&g_displayBuffers) | kUncachedAddressBit);
    mpDisplayBuffers->SetDefaults(static_cast<short>(mnDisplayWidth),
                                  static_cast<short>(mnDisplayHeight),
                                  nPsm,
                                  kZTestGreater,
                                  nZPsm,
                                  kClearOnSwap);
    SetClearColor(mClearColor);
    mnSwapVblank = g_nVblankCounter + 1;
    sceGsSyncVCallback(VblankHandler); // The previous handler it returns is discarded.
    FlipFrameBuffer();
    RestorePacket();

    *R_EE_D0_CHCR |= kChcrTransferTag;
    *R_EE_D1_CHCR |= kChcrTransferTag;
    sceDmaSend(sceDmaGetChan(SCE_DMA_VIF0), ToDmaAddress(g_vu0MicrocodeChain));
    sceDmaSend(sceDmaGetChan(SCE_DMA_VIF1), ToDmaAddress(g_vu1MicrocodeChain));

    EnterVu1Path();
    GifQuadword *pQuad = mpWrite;
    mpWrite = pQuad + 1;
    pQuad->mLo = kVifMscalInit;
    pQuad->mHi = 0;
    LeaveVu1Path();
}

// 0x0049b930
void GfxDevice::BeginFrame() {
    SwapBuffers();
    std::memset(&g_renderStats, 0, sizeof(g_renderStats));
    Rnd::g_pDefaultCam->Draw();
    Rnd::PsMat::SelectDefault();
    g_vramTable.BeginFrame();
    g_lastFrameProfileTimers = g_profileTimers;
    for (auto &timer : g_profileTimers) {
        timer.mCycles = 0;
        timer.mDepth = 0;
    }
}

// 0x004a0238
inline void GfxDevice::SwapBuffers() {
    sceGsSyncPath(0, 0);
    while (g_nVblankCounter < mnSwapVblank) {
    }
    mnSwapVblank = g_nVblankCounter + 1;
    mpDisplayBuffers->PutDispEnv(mnDrawBuffer, 1);
    sceGsSyncPath(0, 0);
    mnDrawBuffer = mnDrawBuffer == 0;
    mpDisplayBuffers->PutDrawEnv(mnDrawBuffer, 1);
    sceGsSyncPath(0, 0);

    const sceGsDrawEnv1 &draw =
        mnDrawBuffer != 0 ? mpDisplayBuffers->mHalves[1].mDraw : mpDisplayBuffers->mHalves[0].mDraw;
    mGsRegs[kGsRegFrame1] = draw.frame1;
    mGsRegs[kGsRegZbuf1] = draw.zbuf1;
    mGsRegs[kGsRegXyOffset1] = draw.xyoffset1;
    mGsRegs[kGsRegScissor1] = draw.scissor1;
    mGsRegs[kGsRegPrModeCont] = draw.prmodecont;
    mGsRegs[kGsRegColClamp] = draw.colclamp;
    mGsRegs[kGsRegDither] = draw.dthe;
    mGsRegs[kGsRegTest1] = draw.test1;
    // A shadow no register value matches, so the next PRIM write always reaches the GS.
    mGsRegs[kGsRegPrim] = kAllBits;
}

// 0x0049bac8
void GfxDevice::PresentFrame(int nSwapBuffers) {
    if (mFeedbackEnabled != 0) {
        SetupGsDrawContext();
    }
    FlushGifPacket(0, 0);
    g_vramTable.EndFrame();
    if (nSwapBuffers != 0) {
        SwapBuffers();
        mnDrawBuffer = mnDrawBuffer == 0; // Yes, the binary inverts it a second time here.
    }
}

// 0x0049b478
int GfxDevice::FlushGifPacket(int bRetainOpenTag, int bOnlyWhenFull) {
    if (bOnlyWhenFull != 0 && mpWrite < mpBuffer + kGifBufferQuadwords) {
        return 0;
    }
    if (mpWrite == mpBuffer) {
        return 0;
    }

    ++g_renderStats.mnGifPackets;
    GifQuadword retainedTag;
    if (bRetainOpenTag != 0) {
        retainedTag = *mpOpenTag;
    }
    CloseGifTag(1);

    SendPacket();
    g_vramTable.AdvanceLockCycle();

    if (reinterpret_cast<std::uintptr_t>(mpBuffer) != kGifBufferHalf0) {
        mpBuffer = reinterpret_cast<GifQuadword *>(kGifBufferHalf0);
    } else {
        mpBuffer = reinterpret_cast<GifQuadword *>(kGifBufferHalf1);
    }
    mpWrite = mpBuffer;

    if (bRetainOpenTag != 0) {
        WriteGifTag(&retainedTag);
    }
    RunLongOperationPollProc();
    return 1;
}

// 0x0049b5a8
void GfxDevice::WriteGifTag(const GifQuadword *pTag) {
    ++g_renderStats.mnGifTags;
    CloseGifTag(0); // Inlined in the binary.

    if (mnUseVu1 != 0 && mpOpenVifDirect == nullptr) {
        GifQuadword *pCodes = mpWrite;
        mpWrite = pCodes + 1;
        // Two VIF NOPs, then FLUSHA, then a DIRECT whose count CloseGifTag() fills in.
        pCodes->mLo = 0;
        pCodes->mHi = (kVifCodeDirect << kVifCodeHighWordShift) | kVifCodeFlushA;
        mpOpenVifDirect = mpWrite;
    }

    GifQuadword *pWrite = mpWrite;
    mpOpenTag = pWrite;
    *pWrite = *pTag;
    mpWrite = pWrite + 1;
}

// 0x0049ffd0
void GfxDevice::SetGsReg(int nReg, unsigned long long qwValue, unsigned long long qwMask) {
    if ((mGsRegs[nReg] & qwMask) == (qwValue & qwMask)) {
        return;
    }

    if (mpOpenTag == nullptr || (mpOpenTag->mHi & kGifTagFirstRegMask) != kGifRegAd) {
        WriteGifTag(&mAdTag);
    }
    const unsigned long long qwMerged = (mGsRegs[nReg] & ~qwMask) | (qwValue & qwMask);
    mGsRegs[nReg] = qwMerged;
    GifQuadword *pWrite = mpWrite;
    mpWrite = pWrite + 1;
    pWrite->mLo = qwMerged;
    pWrite->mHi = static_cast<unsigned long long>(nReg);
    FlushGifPacket(0, 1);

    if (nReg != kGsRegPrim) {
        return;
    }
    const unsigned long long qwPrimType = mGsRegs[kGsRegPrim] & kGsPrimTypeMask;
    if (qwPrimType == kGsPrimTriStrip || qwPrimType == kGsPrimTriFan ||
        qwPrimType == kGsPrimLineStrip) {
        mGsRegs[kGsRegPrim] |= kGsPrimTypeMask;
    }
}

// 0x004a0348
void GfxDevice::EnterVu1Path() {
    if (mnUseVu1 != 0) {
        return;
    }
    FlushGifPacket(0, 0);
    mnUseVu1 = 1;
}

// 0x0049b838
void GfxDevice::LeaveVu1Path() {
    if (mnUseVu1 == 0) {
        return;
    }
    CloseGifTag(1); // Inlined in the binary.
    FlushGifPacket(0, 0);
    sceGsSyncPath(kGsSyncPathWait, kGsSyncPathNoTimeout);
    mnUseVu1 = 0;
}

// 0x0049fd98
void GfxDevice::ReserveGifSpace(int nQuadwords) {
    if (mnUseVu1 == 0) {
        return;
    }
    if (mpReservedRegion != nullptr) {
        return;
    }
    CloseGifTag(1);
    GifQuadword *pRegion = mpBuffer + (kGifBufferQuadwords - nQuadwords);
    mpSavedWrite = mpWrite;
    mpReservedRegion = pRegion;
    mpWrite = pRegion;
}

// 0x0049fe08
void GfxDevice::SwapGifWrite() {
    if (mnUseVu1 == 0) {
        return;
    }
    if (mpReservedRegion == nullptr) {
        return;
    }
    GifQuadword *pWrite = mpWrite;
    mpWrite = mpSavedWrite;
    mpSavedWrite = pWrite;
}

// 0x0049fe38
void GfxDevice::FlushReservedGif() {
    if (mnUseVu1 == 0) {
        return;
    }
    if (mpReservedRegion == nullptr) {
        return;
    }
    // A caller arrives here having swapped back to the main stream. That puts the region's write
    // pointer in mpSavedWrite, and its distance from the region start is the amount written.
    const int nQuadwords = static_cast<int>(mpSavedWrite - mpReservedRegion);
    const GifQuadword *pSource = mpReservedRegion;
    for (int nRemaining = nQuadwords; nRemaining > 0; --nRemaining) {
        GifQuadword *pDest = mpWrite;
        *pDest = *pSource;
        ++pSource;
        mpWrite = pDest + 1;
    }
    mpReservedRegion = nullptr;
}

// 0x004a0158
void GfxDevice::CloseGifTag(int bEndOfPacket) {
    GifQuadword *pTag = mpOpenTag;
    if (pTag == nullptr) {
        return;
    }

    const unsigned long long qwTag = pTag->mLo;
    const int nQuadwords = static_cast<int>(mpWrite - pTag);
    // NREG shifted right by FLG is the register count one loop consumes. A packed loop spends a
    // quadword per register, a register list one per two registers, and an image one per four.
    const unsigned int nFlg =
        static_cast<unsigned int>((qwTag >> kGifTagFlgShift) & kGifTagFlgMask);
    const unsigned int nPerLoop = static_cast<unsigned int>(qwTag >> kGifTagNRegShift) >> nFlg;
    const unsigned long long qwNLoop =
        static_cast<unsigned long long>(nQuadwords - 1) / nPerLoop & kGifTagNLoopMask;
    pTag->mLo = (qwTag & ~kGifTagNLoopMask) | qwNLoop;

    // The binary re-reads the tag pointer here rather than reusing the one above.
    GifQuadword *pSameTag = mpOpenTag;
    pSameTag->mLo = (pSameTag->mLo & ~kGifTagEop) |
                    (static_cast<unsigned long long>(bEndOfPacket & 1) << kGifTagEopShift);
    mpOpenTag = nullptr;

    if (bEndOfPacket == 0 || mnUseVu1 == 0) {
        return;
    }
    // The DIRECT code sits in the high word of the quadword before the data it introduces.
    const unsigned long long qwDirectCount =
        static_cast<unsigned long long>(mpWrite - mpOpenVifDirect);
    mpOpenVifDirect[-1].mHi |= qwDirectCount << kVifCodeHighWordShift;
    mpOpenVifDirect = nullptr;
}

// 0x0049b6b8
void GfxDevice::RestoreFrameBufferTarget() {
    const sceGsDrawEnv1 &draw =
        mnDrawBuffer != 0 ? mpDisplayBuffers->mHalves[1].mDraw : mpDisplayBuffers->mHalves[0].mDraw;
    SetGsReg(kGsRegFrame1, draw.frame1, kGsFrameMask);
    SetGsReg(kGsRegXyOffset1, draw.xyoffset1, kGsXyOffsetMask);
}

// 0x0049b368
void GfxDevice::SetClearColor(const Color &color) {
    mClearColor = color;
    mpDisplayBuffers->mHalves[0].mClear.mWords[kClearRgbaqWord] = PackRgbaq(color);
    mpDisplayBuffers->mHalves[1].mClear.mWords[kClearRgbaqWord] = PackRgbaq(color);
    FlushCache(kFlushCacheWriteBackData);
}

// 0x0049ccb8
void GfxDevice::SetupGsDrawContext() {
    SetGsReg(kGsRegTest1, kTestZTestAlways, kTestZTestMask);
    SetGsReg(kGsRegZbuf1, kZbufZmsk, kZbufZmsk);
    SetGsReg(
        kGsRegAlpha1,
        (static_cast<unsigned long long>(mFeedbackAlpha * kRgbaqAlphaScale) << kAlphaFixShift) |
            kAlphaBlendByFix,
        kAlphaMask);
    SetGsReg(kGsRegDimX, kDimXMatrix, kAllBits);

    // The texture is the frame buffer of the half not being drawn.
    const unsigned long long qwFrame = mnDrawBuffer == 0 ?
                                           mpDisplayBuffers->mHalves[1].mDraw.frame1 :
                                           mpDisplayBuffers->mHalves[0].mDraw.frame1;
    // The binary builds TEX0_1 over an uninitialised register, so bits 37 to 39 inside the mask
    // are undefined there. They are zero here.
    const unsigned long long qwTex0 =
        ((qwFrame & kFrameFbpMask) << kBlocksPerPageShift) |
        (((qwFrame >> kFrameFbwShift) & kFrameFbwMask) << kTex0TbwShift) |
        (((qwFrame >> kFramePsmShift) & kFramePsmMask) << kTex0PsmShift) |
        (kTex0Log2Size1024 << kTex0TwShift) | (kTex0Log2Size1024 << kTex0ThShift) |
        (kTex0TccRgba << kTex0TccShift) | (kTex0TfxDecal << kTex0TfxShift);
    SetGsReg(kGsRegTex0_1, qwTex0, kTex0Mask);
    SetGsReg(kGsRegTex1_1, kTex1MinLinear, kTex1MinMask);
    SetGsReg(kGsRegClamp1, kClampBothAxes, kClampMask);
    SetGsReg(kGsRegTexA, kTexAHalfAlpha, kAllBits);

    GifQuadword tag;
    tag.mLo = kFeedbackTagLo;
    tag.mHi = kFeedbackTagHi;
    WriteGifTag(&tag);
    GifQuadword *pPrim = mpWrite;
    mpWrite = pPrim + 1;
    pPrim->mHi = kFeedbackRgbaq;
    pPrim->mLo = kFeedbackPrim;

    const float flWidth = static_cast<float>(mnDisplayWidth);
    const float flHeight = static_cast<float>(mnDisplayHeight);
    const int nLeft = kGsCoordinateCentre - (mnDisplayWidth << kHalfSizeToSubpixelShift);
    const int nTop = kGsCoordinateCentre - (mnDisplayHeight << kHalfSizeToSubpixelShift);

    const int nNearY = static_cast<int>(mFeedbackRect.y * flHeight) << kGsSubpixelShift;
    const int nNearX = static_cast<int>(mFeedbackRect.x * flWidth) << kGsSubpixelShift;
    GifQuadword *pNear = mpWrite;
    mpWrite = pNear + 1;
    pNear->mLo = PackCoordinates(nNearX + mFeedbackInset + kTexelCentre,
                                 nNearY + mFeedbackInset + kTexelCentre);
    pNear->mHi = PackCoordinates(nLeft + nNearX, nTop + nNearY) | kFeedbackZ;

    const int nFarY = static_cast<int>((mFeedbackRect.y + mFeedbackRect.h) * flHeight)
                      << kGsSubpixelShift;
    const int nFarX = static_cast<int>((mFeedbackRect.x + mFeedbackRect.w) * flWidth)
                      << kGsSubpixelShift;
    GifQuadword *pFar = mpWrite;
    mpWrite = pFar + 1;
    pFar->mHi = PackCoordinates(nLeft + nFarX, nTop + nFarY) | kFeedbackZ;
    pFar->mLo = PackCoordinates(nFarX - mFeedbackInset + kTexelCentre,
                                nFarY - mFeedbackInset + kTexelCentre);
    FlushGifPacket(0, 1);
}

// 0x0049bc20
void GfxDevice::DrawDebugText(const char *pszText, const Rect &rect, const Color &color) {
    GifQuadword primAndColor;
    primAndColor.mLo = kGsPrimLineStrip;
    primAndColor.mHi = PackRgbaq(color);
    const int nCellWidth = static_cast<int>(rect.w * kSubpixelsPerPixel);
    const int nCellHeight = static_cast<int>(rect.h * kSubpixelsPerPixel);
    const int nPenY = static_cast<int>(rect.y * kSubpixelsPerPixel);
    int nPenX = static_cast<int>(rect.x * kSubpixelsPerPixel);

    for (const char *pchText = pszText; *pchText != '\0'; ++pchText) {
        const int nGlyph = DebugGlyphIndex(*pchText);
        if (nGlyph < 0) {
            nPenX += nCellWidth * kBlankAdvanceCells;
            continue;
        }

        GifQuadword *pHeader = g_gfxDevice.mpWrite;
        *pHeader = primAndColor;
        g_gfxDevice.mpWrite = pHeader + 1;
        const float flCellWidth = static_cast<float>(nCellWidth);
        const float flCellHeight = static_cast<float>(nCellHeight);
        const float *pflStroke = g_aafDebugGlyphStrokes[nGlyph];
        for (int i = 0; i < kDebugGlyphSegments; ++i) {
            GifQuadword *pPoints = g_gfxDevice.mpWrite;
            g_gfxDevice.mpWrite = pPoints + 1;
            pPoints->mLo = PackCoordinates(nPenX + static_cast<int>(pflStroke[0] * flCellWidth),
                                           nPenY + static_cast<int>(pflStroke[1] * flCellHeight)) |
                           kDebugZ;
            pPoints->mHi = PackCoordinates(nPenX + static_cast<int>(pflStroke[2] * flCellWidth),
                                           nPenY + static_cast<int>(pflStroke[3] * flCellHeight)) |
                           kDebugZ;
            pflStroke += kFloatsPerSegment;
        }
        nPenX += static_cast<int>(nCellWidth * kGlyphAdvanceCells);
        g_gfxDevice.FlushGifPacket(1, 1);
    }
}

// 0x0049c630
void GfxDevice::DrawTimingBar(const Rect &rect, const Color &color) {
    GifQuadword *pHeader = g_gfxDevice.mpWrite;
    g_gfxDevice.mpWrite = pHeader + 1;
    pHeader->mLo = kGsPrimSpriteValue;
    pHeader->mHi = PackRgbaq(color);

    GifQuadword *pCorners = g_gfxDevice.mpWrite;
    g_gfxDevice.mpWrite = pCorners + 1;
    pCorners->mLo = PackCoordinates(static_cast<int>(rect.x * kSubpixelsPerPixel),
                                    static_cast<int>(rect.y * kSubpixelsPerPixel)) |
                    kDebugZ;
    pCorners->mHi = PackCoordinates(static_cast<int>((rect.x + rect.w) * kSubpixelsPerPixel),
                                    static_cast<int>((rect.y + rect.h) * kSubpixelsPerPixel)) |
                    kDebugZ;
    g_gfxDevice.FlushGifPacket(1, 1);
}

// 0x0049bec8
void GfxDevice::DrawRenderStatsOverlay() {
    SetGsReg(kGsRegTest1, kTestZTestAlways, kTestZTestMask);
    GifQuadword tag;
    tag.mLo = kTextTagLo;
    tag.mHi = kTextTagHi;
    WriteGifTag(&tag);

    Rect rect;
    rect.x = static_cast<float>(kOverlayOrigin - mnDisplayWidth / 2);
    rect.y = static_cast<float>(kOverlayOrigin - mnDisplayHeight / 2);
    rect.w = kOverlayTextCell;
    rect.h = kOverlayTextCell;
    const Color white = kOverlayWhite;

    const float flFrameMs = TimerMilliseconds(g_lastFrameProfileTimers[kProfileTimerFrame]);
    const int nFps = flFrameMs == 0.0f ? 0 : static_cast<int>(kMillisecondsPerSecond / flFrameMs);
    DrawDebugText(FormatString("fps %d", nFps), rect, white);

    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("points %d", g_renderStats.mnPoints), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("draws %d", g_renderStats.mnMeshDraws), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("clippedtris %d", g_renderStats.mnFacesClipped), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("tris %d", g_renderStats.mnTriangles), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("splittris %d", g_renderStats.mnSplitTriangles), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("clippedlines %d", g_renderStats.mnEdgesClipped), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("lines %d", g_renderStats.mnLines), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("clippedsprites %d", g_renderStats.mnSpritesCulled), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("sprites %d", g_renderStats.mnSpritesDrawn), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("verts %d", g_renderStats.mnVertsTransformed), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("tags %d", g_renderStats.mnGifTags), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("packets %d", g_renderStats.mnGifPackets), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("mats %d", g_renderStats.mnMatSelects), rect, white);
    rect.y += kOverlayLineSpacing;
    DrawDebugText(FormatString("litverts %d", g_renderStats.mnLitVerts), rect, white);

    int nLoads;
    int nBlocks;
    rect.y += kOverlayLineSpacing;
    g_vramTable.GetLastFrameLoads(&nLoads, &nBlocks);
    DrawDebugText(FormatString("vramk %d", nBlocks >> 2), rect, white);
}

// 0x0049c388
void GfxDevice::DrawFpsReadout() {
    mflFrameMsSum += TimerMilliseconds(g_lastFrameProfileTimers[kProfileTimerFrame]);
    mflSyncMsSum += TimerMilliseconds(g_lastFrameProfileTimers[kProfileTimerSync]);
    if (--mnFpsCountdown == 0) {
        const int nFps =
            mflFrameMsSum == 0.0f ?
                0 :
                static_cast<int>(kMillisecondsPerSecond * kFpsSampleFrames / mflFrameMsSum);
        mnFpsCountdown = kFpsSampleFrames;
        mnFps = nFps;
        mnSyncMsAverage = static_cast<int>(mflSyncMsSum / kFpsSampleFrames);
        mflFrameMsSum = 0.0f;
        mflSyncMsSum = 0.0f;
    }

    SetGsReg(kGsRegTest1, kTestZTestAlways, kTestZTestMask);
    GifQuadword tag;
    tag.mLo = kTextTagLo;
    tag.mHi = kTextTagHi;
    WriteGifTag(&tag);

    Rect rect;
    rect.x = static_cast<float>(mnDisplayWidth / 2 + kFpsReadoutRightOffset);
    rect.y = static_cast<float>(kFpsReadoutTop - mnDisplayHeight / 2);
    rect.w = kOverlayTextCell;
    rect.h = kOverlayTextCell;
    const Color white = kOverlayWhite;
    DrawDebugText(FormatString("fps %d sync %d", mnFps, mnSyncMsAverage), rect, white);
}

// 0x0049c778
void GfxDevice::DrawSubsystemTimingGraph(int nFullScaleMs) {
    SetGsReg(kGsRegTest1, kTestZTestAlways, kTestZTestMask);
    GifQuadword barTag;
    barTag.mLo = kBarTagLo;
    barTag.mHi = kBarTagHi;
    WriteGifTag(&barTag);

    const int nTop = kOverlayOrigin - mnDisplayHeight / 2;
    const int nLeft = kOverlayOrigin - mnDisplayWidth / 2;
    const float flPixelsPerMs = static_cast<float>(mnDisplayWidth) * kTimingBarWidthShare /
                                static_cast<float>(nFullScaleMs);
    Color barColor = {kTimingBarGrey, kTimingBarGrey, kTimingBarGrey, 1.0f};
    Rect rect;
    rect.x = static_cast<float>(nLeft);
    rect.y = static_cast<float>(nTop);
    rect.w = 0.0f;
    rect.h = kTimingBarHeight;
    for (int i = 0; i < static_cast<int>(g_profileTimers.size()); ++i) {
        rect.w = TimerMilliseconds(g_lastFrameProfileTimers[i]) * flPixelsPerMs;
        DrawTimingBar(rect, barColor);
        rect.y += kOverlayLineSpacing;
    }

    // One thin line every five milliseconds, spanning every bar.
    rect.y = static_cast<float>(nTop);
    barColor.b = kTimingTickGrey;
    barColor.r = kTimingTickGrey;
    barColor.g = kTimingTickGrey;
    rect.w = kTimingTickWidth;
    rect.h = static_cast<float>(static_cast<int>(g_profileTimers.size()) * kOverlayLinePixels);
    for (int nMs = 0; nMs < nFullScaleMs; nMs += kTimingTickMs) {
        DrawTimingBar(rect, barColor);
        rect.x += flPixelsPerMs * kTimingTickMs;
    }

    GifQuadword textTag;
    textTag.mLo = kTextTagLo;
    textTag.mHi = kTextTagHi;
    WriteGifTag(&textTag);
    const Color white = kOverlayWhite;
    rect.x = static_cast<float>(nLeft + 1);
    rect.y = static_cast<float>(nTop + 1);
    rect.w = kOverlayTextCell;
    rect.h = kOverlayTextCell;
    for (int i = 0; i < static_cast<int>(g_profileTimers.size()); ++i) {
        const float flMs = TimerMilliseconds(g_lastFrameProfileTimers[i]);
        const HxStr &name = g_profileTimers[i].mName;
        const char *pszName = name.mStr != nullptr ? name.mStr : g_szEmptyString;
        if (kTimingLabelMinimumMs <= flMs) {
            DrawDebugText(FormatString("%s %.1f", pszName, flMs), rect, white);
        } else {
            DrawDebugText(pszName, rect, white);
        }
        rect.y += kOverlayLineSpacing;
    }
}

// 0x0049fec0
int GfxDevice::GetReservedVramWords() const {
    return (mpDisplayBuffers->mZbp << kGsPageWordShift) +
           ((mnDisplayWidth * mnDisplayHeight * mnDepthBytes) >> kBytesPerWordShift);
}
