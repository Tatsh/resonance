#include "os/devconsole.h"

#include <cstdint>
#include <eekernel.h>
#include <libdev.h>
#include <libdma.h>
#include <libgraph.h>
#include <libvifpk.h>

#include "os/iop.h"
#include "os/log.h"

namespace {

// Two VIF1 packet buffers in scratchpad memory. Only the first is ever filled.
constexpr std::uintptr_t kPacketBuffer = 0x70000080;
constexpr std::uintptr_t kUnusedPacketBuffer = 0x70000180;

// A packet's base becomes a DMA tag address by keeping the low 28 bits and setting the scratchpad
// flag.
constexpr std::uintptr_t kDmaAddressMask = 0x0fffffff;
constexpr std::uintptr_t kDmaScratchpadFlag = 0x80000000;

constexpr int kDmaResetEnable = 1;
constexpr unsigned short kDmaChannelMaskVif1 = 2;
constexpr unsigned int kDmaChcrTagTransfer = 0x40;

constexpr int kGsSyncPathWait = 0;
constexpr unsigned short kGsSyncPathNoTimeout = 0;
constexpr int kGsSyncVWait = 0;
constexpr int kFlushCacheWriteBackData = 0;

// sceGsResetGraph() arguments. Reset mode 0, interlaced, NTSC, and frame mode.
constexpr short kGsResetAll = 0;
constexpr short kGsInterlace = 1;
constexpr short kGsNtsc = 2;
constexpr short kGsFrameMode = 1;

// The double buffer the console draws over.
constexpr short kGsPsmCt32 = 0;
constexpr short kGsPsmZ24 = 0x31;
constexpr short kDisplayWidth = 640;
constexpr short kDisplayHeight = 224;
constexpr short kGsZTestGEqual = 2;
constexpr short kClearEnabled = 1;

// The clear colour, written into the low three bytes of each half's RGBAQ value word.
constexpr int kClearRgbaqWord = 4;
constexpr unsigned long kClearRed = 0x40;
constexpr unsigned long kClearGreen = 0x40;
constexpr unsigned long kClearBlue = 0x80;
constexpr unsigned long kRgbMask = 0xffffff;
constexpr int kGreenShift = 8;
constexpr int kBlueShift = 16;

// A one-register A+D GIFtag with end of packet, as the alpha environment is sent.
constexpr unsigned long kAdGifTagLo = 0x1000000000008000UL;
constexpr unsigned long kAdGifTagHi = 0xe;
constexpr unsigned int kWordsPerQuadword = 4;
constexpr short kPabeOff = 0;
constexpr unsigned int kVifPacketOptionNone = 0;
constexpr int kDirectNoStall = 0;

// The console geometry, in GS primitive coordinates and character cells.
constexpr unsigned int kConsoleGsX = 0x6d00;
constexpr unsigned int kConsoleGsY = 0x7a80;
constexpr unsigned int kConsoleColumns = 75;
constexpr unsigned int kConsoleRows = 30;

inline void SetClearRgb(sceGsClear &clear) {
    unsigned long &rgbaq = clear.mWords[kClearRgbaqWord];
    rgbaq =
        (rgbaq & ~kRgbMask) | kClearRed | (kClearGreen << kGreenShift) | (kClearBlue << kBlueShift);
}

} // namespace

// 0x0077d5d0
// Nothing in the image reads it.
int g_nDebugGsResetWord;

// 0x008e4f20
int g_nDebugConsole;

// 0x008e4f30
sceGsDBuff g_debugDoubleBuffer;

// 0x005e5d08
int InitDebugGs() {
    sceGifTag adTag;
    adTag.mWords[0] = kAdGifTagLo;
    adTag.mWords[1] = kAdGifTagHi;
    g_nDebugGsResetWord = 0;

    sceDevVif0Reset();
    sceDevVu0Reset();
    sceGsResetPath();
    sceDmaReset(kDmaResetEnable);

    sceVif1Packet packet;
    sceVif1Packet unusedPacket;
    sceVif1PkInit(&packet, reinterpret_cast<void *>(kPacketBuffer));
    sceVif1PkInit(&unusedPacket, reinterpret_cast<void *>(kUnusedPacketBuffer));

    sceDmaEnv env;
    sceDmaGetEnv(&env);
    env.mChannelMask = kDmaChannelMaskVif1;
    sceDmaPutEnv(&env);
    sceDmaChan *pVif1 = sceDmaGetChan(SCE_DMA_VIF1);
    pVif1->chcr |= kDmaChcrTagTransfer;

    sceGsSyncPath(kGsSyncPathWait, kGsSyncPathNoTimeout);
    (void)sceGsSyncV(kGsSyncVWait); // Yes, the binary discards the field.
    sceGsResetGraph(kGsResetAll, kGsInterlace, kGsNtsc, kGsFrameMode);
    sceGsSetDefDBuff(&g_debugDoubleBuffer,
                     kGsPsmCt32,
                     kDisplayWidth,
                     kDisplayHeight,
                     kGsZTestGEqual,
                     kGsPsmZ24,
                     kClearEnabled);
    SetClearRgb(g_debugDoubleBuffer.clear1);
    SetClearRgb(g_debugDoubleBuffer.clear0);
    FlushCache(kFlushCacheWriteBackData);

    sceVif1PkReset(&packet);
    sceVif1PkCnt(&packet, kVifPacketOptionNone);
    sceVif1PkOpenDirectCode(&packet, kDirectNoStall);
    sceVif1PkOpenGifTag(&packet, adTag);
    const int nPairs =
        sceGsSetDefAlphaEnv(reinterpret_cast<sceGsAlphaEnv *>(packet.pCurrent), kPabeOff);
    sceVif1PkReserve(&packet, nPairs * kWordsPerQuadword);
    sceVif1PkCloseGifTag(&packet);
    sceVif1PkCloseDirectCode(&packet);
    sceVif1PkEnd(&packet, kVifPacketOptionNone);
    sceVif1PkTerminate(&packet);

    const std::uintptr_t nBase = reinterpret_cast<std::uintptr_t>(packet.pBase);
    sceDmaSend(pVif1, reinterpret_cast<void *>((nBase & kDmaAddressMask) | kDmaScratchpadFlag));
    while (sceGsSyncV(kGsSyncVWait) == 0) {
    }
    return 1;
}

// 0x005e5ed0
void ShowScreenMessage([[maybe_unused]] const char *pszText, [[maybe_unused]] int nDuration) {
}

// 0x005e5ed8
void OpenDebugConsole() {
    sceDevConsInit();
    g_nDebugConsole = sceDevConsOpen(kConsoleGsX, kConsoleGsY, kConsoleColumns, kConsoleRows);
    sceDevConsClear(g_nDebugConsole);
}

// 0x005e5f18
void InitDebugConsole() {
    (void)InitDebugGs(); // Yes, the binary discards the result.
    sceDevConsInit();
    g_nDebugConsole = sceDevConsOpen(kConsoleGsX, kConsoleGsY, kConsoleColumns, kConsoleRows);
    sceDevConsClear(g_nDebugConsole);
}

// 0x005e5f60
void ClearDebugConsole() {
    sceDevConsClear(g_nDebugConsole);
}
