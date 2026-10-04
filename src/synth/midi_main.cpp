#include "synth/midi_main.h"

#include <csl.h>
#include <eekernel.h>
#include <libsdr.h>
#include <msin.h>
#include <sifdev.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app/application.h"
#include "os/async.h"
#include "os/cycles.h"
#include "os/iop.h"
#include "os/loadfile.h"
#include "os/log.h"
#include "os/mem.h"
#include "rnd/amovieset.h"
#include "sch/tickclock.h"
#include "script/configquery.h"
#include "synth/callbackxferhdtoiop.h"

// The driver's diagnostic request, submitted by SynthCommand's third command and by the tail of the
// voice report.
constexpr int kSoundSelectorInfo = 0xd0;

// libsdr's trampoline routes a zero first argument through its callback thread. Every call site in
// the game passes 1, which is the SDK's own `blocking` argument.
constexpr int kSdRemoteBlocking = 1;

constexpr int kSpu2CoreCount = 2;

constexpr int kSpu2VoicesPerCore = 24;

// Mixer routing each core starts with. Core 1 additionally takes the input core 0 feeds it.
constexpr int kSpu2Core0Mix = 0xf00;
constexpr int kSpu2Core1Mix = 0xfcc;

constexpr int kSpu2MaxVolume = 0x3fff;

// Highest byte of sound RAM, which is where core 0's effect work area ends. Each core is given the
// block below the previous one.
constexpr unsigned kSpu2EffectAreaTop = 0x1fffff;
constexpr unsigned kSpu2EffectAreaSize = 0x20000;

// Stream frames in one bar, which SetSynthStreamBar() scales a bar by.
constexpr int kSynthStreamFramesPerBar = 19200;

// The game's own IOP sound driver, and the spin ezMidiInit() waits between binds.
constexpr int kSoundDriverRpcServer = 0x12346;
constexpr int kSoundDriverBindSpin = 9999;

// Selector bits SubmitSoundDriverRequest() tests. The synchronous bit waits for a reply and skips
// the pending flag, and the block bit ships a whole SoundDriverCommand from the argument.
constexpr int kSoundSelectorSynchronous = 0x8000;
constexpr int kSoundSelectorCommandBlock = 0x1000;

// The request a selector without the block bit sends, the argument in its first word.
constexpr int kSoundDriverRequestSize = 0x10;

// sceSifCallRpc() blocks in mode zero, and sceSifCheckStatRpc() reports 1 while a call runs.
constexpr int kSifRpcModeWait = 0;
constexpr int kSifRpcStillRunning = 1;

// The words the driver replies with, and the request a value-carrying selector sends from.
constexpr int kSoundDriverReplyWords = 16;

// Selectors InitSynthDriver() and the three forwarders submit. InitSynthDriver() retains the reply
// to the first as the IOP address PollSynthEvents() writes to.
constexpr int kSoundSelectorAllocEventBuffer = 0x8010;
constexpr int kSoundSelectorMono = 0x110;
constexpr int kSoundSelectorRemix = 0x100;
constexpr int kSoundSelectorPause = 0xf0;

// The argument InitSynthDriver() passes with kSoundSelectorAllocEventBuffer.
constexpr uintptr_t kMidiEventBufferRequest = 0x4000;

// Bytes of each staging buffer and each bank buffer InitSynthDriver() takes from the IOP heap.
constexpr int kIopStagingBufferSize = 0x2000;
constexpr int kBankIopBufferSize = 0x10000;

// The bank address index InitSynthDriver() leaves selected. The first address is used once.
constexpr int kBankIopIndexAfterInit = 1;

// PollSynthEvents() alternates between two driver event buffers of this many bytes, and ships the
// stream buffer's two header words with its payload.
constexpr int kMidiEventBufferCount = 2;
constexpr int kMidiEventBufferSize = 0x400;
constexpr int kMidiStreamHeaderSize = 2 * sizeof(unsigned int);

// The frame size PollSynthStream() asks the synth movie to read.
constexpr int kSynthStreamReadSize = 0x4000;

// The SIF records here start on cache lines, as the image places them.
// NTSC-U/C: 0x008e5bc0, PAL: 0x0092abc0
alignas(64) sceSifClientData gCd;

// Set while a request sent without waiting is still running on the driver.
// NTSC-U/C: 0x00780878, PAL: 0x007c4590
int g_bSoundRequestPending = 0;

// The driver's reply, whose first word SubmitSoundDriverRequest() reports.
// NTSC-U/C: 0x008e5b80, PAL: 0x0092ab80
alignas(64) unsigned int g_anSoundDriverReply[kSoundDriverReplyWords] = {};

// The descriptor ezTransToIOP() hands to the SIF DMA.
// NTSC-U/C: 0x008e5be8, PAL: 0x0092abe8
alignas(16) sceSifDmaData transData;

// Set once InitSynthDriver() has brought the driver up.
// NTSC-U/C: 0x006e9b88, PAL: 0x0072d54c
int g_bSynthDriverReady = 0;

// The IOP address of the driver's event buffers, from InitSynthDriver().
// NTSC-U/C: 0x006e9dc0, PAL: 0x0072d780
int g_nMidiEventIopAddress = 0;

// The event buffer PollSynthEvents() writes next.
// NTSC-U/C: 0x006e9bd0, PAL: 0x0072d590
int g_nMidiEventBufferIndex = 0;

#ifdef VIDEO_STANDARD_PAL
// The driver gathers its errors into two logs, each a length word and then the text.
constexpr int kHardSynthErrorLogCount = 2;
constexpr int kHardSynthErrorLogSize = 0x400;

// PollSynthEvents() writes the error logs out on every sixteenth call.
constexpr int kHardSynthErrorLogPollMask = 0xf;

constexpr int kHardSynthErrorFilePathSize = 0x100;

struct HardSynthErrorLog {
    int mLength;
    char mText[kHardSynthErrorLogSize - sizeof(int)];
};

// The IOP buffers of the error logs. InitSynthDriver() takes them from the IOP heap.
// NTSC-U/C: absent, PAL: 0x008d8e60
int g_anHardSynthErrorLogIopAddress[kHardSynthErrorLogCount];

// The EE copies of the error logs. PollSynthEvents() writes them to the error file.
// NTSC-U/C: absent, PAL: 0x008d8e80
alignas(64) HardSynthErrorLog g_aHardSynthErrorLogs[kHardSynthErrorLogCount];

// The open error file, or null while the level specifies none or the file failed to open.
// NTSC-U/C: absent, PAL: 0x0072d544
FILE *g_pHardSynthErrorFile = nullptr;

// The count of PollSynthEvents() calls. The count paces the error log writes.
// NTSC-U/C: absent, PAL: 0x0072d594
int g_nSynthEventPollCount = 0;

// The path g_pHardSynthErrorFile was opened from.
// NTSC-U/C: absent, PAL: 0x008d9680
char g_szHardSynthErrorFilePath[kHardSynthErrorFilePathSize];
#endif

// SIF DMA moves whole quadwords. Both commands the driver receives start on the cache line the
// original placed them on.
// NTSC-U/C: 0x00894cc0, PAL: 0x008d9cc0
alignas(64) SoundDriverCommand g_chunkCommand;

// NTSC-U/C: 0x00894bc0, PAL: 0x008d9bc0
alignas(64) SoundDriverCommand g_bankCommand;

// NTSC-U/C: 0x00894748, PAL: 0x008d8e48
int g_anIopStagingAddress[kIopStagingBufferCount] = {};

// NTSC-U/C: 0x006e9b80, PAL: 0x0072d540
int g_nIopStagingIndex = 0;

// NTSC-U/C: 0x006e9b84, PAL: 0x0072d548
int g_nBankIopAddress = 0;

// NTSC-U/C: 0x006e9b90, PAL: 0x0072d550
HxStr g_bdBankName;

// NTSC-U/C: 0x006e9b98, PAL: 0x0072d558
HxStr g_hdBankName;

// NTSC-U/C: 0x006e9ba4, PAL: 0x0072d564
int g_nBankDestAddress = 0x5010;

// NTSC-U/C: 0x006e9bb4, PAL: 0x0072d574
int g_nSynthXferTag;

// NTSC-U/C: 0x006e9bc4, PAL: 0x0072d584
void (*g_pfnBankLoadProgress)();

// NTSC-U/C: 0x006e9dc8, PAL: 0x0072d788
int g_nHdXferInFlight;

// NTSC-U/C: 0x006e9dcc, PAL: 0x0072d78c
void *g_pHdXferBuffer;

// NTSC-U/C: 0x006e9dd0, PAL: 0x0072d790
void *g_pBdXferBuffer;

// NTSC-U/C: 0x006e9dd4, PAL: 0x0072d794
CallbackXferBdToIop *g_pBdXfer;

// NTSC-U/C: 0x006e9ba8, PAL: 0x0072d568
int g_anBankDestAddress[kBankDestBufferCount] = {0x1d6b0, 0xf2b38};

// NTSC-U/C: 0x006e9bb0, PAL: 0x0072d570
int g_nBankDestIndex = 1;

// NTSC-U/C: 0x00894750, PAL: 0x008d8e50
int g_anBankIopAddress[kBankIopAddressCount];

// NTSC-U/C: 0x0089475c, PAL: 0x008d8e5c
int g_nBankIopIndex;

// NTSC-U/C: 0x00894c40, PAL: 0x008d9c40
alignas(64) char g_szHdBankPath[kHdBankPathSize];

// NTSC-U/C: 0x006e9bb8, PAL: 0x0072d578
std::vector<BankSlot> g_bankSlots;

// The streamed audio, or null while none plays.
// NTSC-U/C: 0x006e9c58, PAL: 0x0072d618
Rnd::AMovieSet *g_pSynthStream;

// The frame PollSynthStream() passes to g_pSynthStream.
// NTSC-U/C: 0x006e9c5c, PAL: 0x0072d61c
int g_nSynthStreamFrame;

// The song tick StartSoundBankMovie() opened the movie at. Nothing reads it.
// NTSC-U/C: 0x006e9dc4, PAL: 0x0072d784
int g_nSoundBankMovieTick;

void SetBankLoadProgressHook(void (*pfnProgress)()) {
    g_pfnBankLoadProgress = pfnProgress;
}

// Selector that reports a bank complete.
constexpr int kSoundSelectorBankComplete = 0x1050;

// Selector that releases a loaded bank. It puts the bank's tag in the register that a block
// selector puts an address in, which is why the tag travels through a pointer parameter.
constexpr int kSoundSelectorReleaseBank = 0x8130;

// Tag a released slot is marked with.
constexpr int kBankSlotTagNone = -1;

void RegisterBankSlot(int nTag, int nDest, int nIopAddress) {
    bool bClaimed = false;
    for (auto &slot : g_bankSlots) {
        if (slot.mDest == nDest) {
            if (slot.mIopAddress != 0) {
                SubmitSoundDriverRequest(kSoundSelectorReleaseBank,
                                         static_cast<uintptr_t>(slot.mTag));
            }
            slot.mTag = nTag;
            slot.mIopAddress = nIopAddress;
            bClaimed = true;
            // The walk continues rather than stopping here, so a later slot using the same tag is
            // released below.
            continue;
        }
        if (slot.mTag == nTag) {
            slot.mTag = kBankSlotTagNone;
            slot.mIopAddress = 0;
        }
    }
    if (!bClaimed) {
        BankSlot slot{nTag, nDest, nIopAddress};
        g_bankSlots.push_back(slot);
    }
}

void ReleaseBankSlotAt(int nDest) {
    for (auto &slot : g_bankSlots) {
        if (slot.mDest == nDest) {
            if (slot.mIopAddress != 0) {
                SubmitSoundDriverRequest(kSoundSelectorReleaseBank,
                                         static_cast<uintptr_t>(slot.mTag));
                slot.mTag = kBankSlotTagNone;
                slot.mIopAddress = 0;
            }
            return;
        }
    }
}

void ReleaseAllBankSlots() {
    for (const auto &slot : g_bankSlots) {
        if (slot.mIopAddress != 0) {
            SubmitSoundDriverRequest(kSoundSelectorReleaseBank, static_cast<uintptr_t>(slot.mTag));
        }
    }
    g_bankSlots.clear();
}

int IsBankXferBusy() {
    if (g_nHdXferInFlight != 0) {
        return 1;
    }
    if (g_pBdXfer != nullptr && g_pBdXfer->mRemaining != 0) {
        return 1;
    }
    return 0;
}

// Scratch the four-character code is copied into. The second word is never written and terminates
// the string.
char g_szFourCc[2 * sizeof(int)];

char *FourCcToString(const void *pFourCc) {
    *reinterpret_cast<int *>(g_szFourCc) = *static_cast<const int *>(pFourCc);
    return g_szFourCc;
}

void SetSynthStreamBar(int nBar) {
    if (g_pSynthStream != nullptr) {
        g_nSynthStreamFrame = nBar * kSynthStreamFramesPerBar;
    }
}

// The rotation StartHdBankXfer() and XferBankFromMemory() share. The wrap returns to the
// second entry, so the first is used once and never again.
inline void RotateBankIopAddress() {
    g_nBankIopAddress = g_anBankIopAddress[g_nBankIopIndex];
    ++g_nBankIopIndex;
    if (g_nBankIopIndex == kBankIopAddressCount) {
        g_nBankIopIndex = 1;
    }
}

int XferBankFromMemory(const void *pData, int nLength) {
    ReleaseBankSlotAt(g_nBankDestAddress);
    RotateBankIopAddress();
    RegisterBankSlot(g_nSynthXferTag, g_nBankDestAddress, g_nBankIopAddress);
    g_bankCommand.mBankAddress = g_nBankIopAddress;
    g_bankCommand.mStagingAddress = 0;
    g_bankCommand.mLength = 0;
    g_bankCommand.mDest = g_nBankDestAddress;
    g_bankCommand.mTag = g_nSynthXferTag;
    memset(g_bankCommand.mPayload, 0, kSoundDriverCommandPayloadSize);
    ezTransToIOP(g_nBankIopAddress, pData, nLength);
    SubmitSoundDriverRequest(kSoundSelectorBankComplete,
                             reinterpret_cast<uintptr_t>(&g_bankCommand));
    return 0;
}

int StartBdBankXfer(const char *pszPath) {
    AsyncCheck(1);
    g_bankCommand.mBankAddress = g_nBankIopAddress;
    g_bankCommand.mDest = g_nBankDestAddress;
    g_bankCommand.mTag = g_nSynthXferTag;
    strcpy(g_bankCommand.mPayload, pszPath);
    FlushCache(0);
    const char *pszColon = strchr(pszPath, ':');
    const char *pszName = (pszColon != nullptr) ? pszColon + 1 : pszPath;
    const int nLength = FileTrueSize(pszName);
    // The measured length reaches the block whether or not the measurement succeeded.
    g_bankCommand.mLength = nLength;
    if (nLength <= 0) {
        printf("file open failed. %s \n", pszName);
        return -1;
    }
    const int nFile = FileOpen(pszName, 0);
    g_pBdXferBuffer = MemAllocTagged(kBankChunkSize + kBankBufferAlignment - 1, __FILE__, __LINE__);
    const uintptr_t nRaw = reinterpret_cast<uintptr_t>(g_pBdXferBuffer) + kBankBufferAlignment - 1;
    char *pReadBuffer =
        reinterpret_cast<char *>(nRaw & ~static_cast<uintptr_t>(kBankBufferAlignment - 1));
    const int nChunkLength = (nLength <= kBankChunkSize) ? nLength : kBankChunkSize;
    CallbackXferBdToIop *pXfer =
        new CallbackXferBdToIop(nFile, pReadBuffer, g_bankCommand.mDest, nChunkLength, nLength);
    g_pBdXfer = pXfer;
    g_hdXfer.mpBdXfer = g_pBdXfer;
    pXfer->mRequestId = AsyncSubmitRequest(nFile, pReadBuffer, nChunkLength, 0, pXfer, 0);
    return nLength;
}

int StartHdBankXfer(const char *pszPath, int nPlacement) {
    AsyncCheck(1);
    strcpy(g_szHdBankPath, pszPath);
    const char *pszColon = strchr(pszPath, ':');
    const char *pszName = (pszColon != nullptr) ? pszColon + 1 : pszPath;
    const int nLength = FileTrueSize(pszName);
    if (nLength <= 0) {
        printf("file open failed. %s \n", pszName);
        return -1;
    }
    if (nPlacement >= 0 && nPlacement < kBankPlacementFirstBuffer) {
        g_nBankIopAddress = g_anBankIopAddress[0];
    } else if (nPlacement >= kBankPlacementFirstBuffer && nPlacement <= kBankPlacementRotate) {
        RotateBankIopAddress();
    }
    if (g_nBankIopAddress < 0) {
        printf("\nCan't alloc heap \n");
        return -1;
    }
    g_pHdXferBuffer = MemAllocTagged(nLength + kBankBufferAlignment, __FILE__, __LINE__);
    const uintptr_t nRaw = reinterpret_cast<uintptr_t>(g_pHdXferBuffer) + kBankBufferAlignment - 1;
    char *pReadBuffer =
        reinterpret_cast<char *>(nRaw & ~static_cast<uintptr_t>(kBankBufferAlignment - 1));
    g_nHdXferInFlight = AsyncLoadFileByPath(pszName, pReadBuffer, nLength, &g_hdXfer);
    return 0;
}

void LoadSoundBank(const char *pszBdPath, const char *pszHdPath, int nTag, int nPlacement) {
    const int nPreviousDest = g_nBankDestAddress;
    g_nSynthXferTag = nTag;
    if (g_bdBankName == pszBdPath && g_hdBankName == pszHdPath) {
        return;
    }
    g_bdBankName = pszBdPath;
    g_hdBankName = pszHdPath;
    if (nPlacement == kBankPlacementFirstBuffer) {
        g_nBankDestAddress = g_anBankDestAddress[0];
    } else if (nPlacement == kBankPlacementRotate) {
        g_nBankDestAddress = g_anBankDestAddress[g_nBankDestIndex];
    } else if (nPlacement >= 0 && nPlacement < kBankPlacementFirstBuffer) {
        g_nBankDestAddress = kBankFixedDestAddress;
    }
    ReleaseBankSlotAt(g_nBankDestAddress);
    // Yes, the binary discards both results.
    StartHdBankXfer(pszHdPath, nPlacement);
    StartBdBankXfer(pszBdPath);
    RegisterBankSlot(g_nSynthXferTag, g_nBankDestAddress, g_nBankIopAddress);
    if (nPlacement != kBankPlacementRotate) {
        g_nBankDestAddress = nPreviousDest;
        return;
    }
    g_nBankDestIndex = (g_nBankDestIndex + 1) & (kBankDestBufferCount - 1);
    g_nBankDestAddress = g_anBankDestAddress[g_nBankDestIndex];
}

void SynthCommand(int nCommand) {
    switch (nCommand) {
    case 0:
        break;
    case 1:
        DumpSynthVoices(1);
        break;
    case 2:
        SubmitSoundDriverRequest(kSoundSelectorInfo, 0);
        break;
    default:
        printf("Unrecognized synth cmd %d\n", nCommand);
        break;
    }
}

void DumpSynthVoices(int bActiveOnly) {
    int nActive = 0;
    for (int nCore = 0; nCore < kSpu2CoreCount; ++nCore) {
        const int nMMix = sceSdRemote(kSdRemoteBlocking, rSdGetParam, SD_P_MMIX | nCore);
        const int nEffect =
            sceSdRemote(kSdRemoteBlocking, rSdGetCoreAttr, SD_C_EFFECT_ENABLE | nCore);
        const int nVMixL = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_S_VMIXL | nCore);
        const int nVMixEL = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_S_VMIXEL | nCore);
        const int nVMixR = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_S_VMIXR | nCore);
        const int nVMixER = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_S_VMIXER | nCore);
        const int nEndX = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_S_ENDX | nCore);
        printf("Core    %2.2d: MMix %x eff %d vmix L %x %x R %x %x end %x\n",
               nCore,
               nMMix,
               nEffect,
               nVMixL,
               nVMixEL,
               nVMixR,
               nVMixER,
               nEndX);
        for (int nVoice = 0; nVoice < kSpu2VoicesPerCore; ++nVoice) {
            const int nEntry = SD_VOICE(nCore, nVoice);
            const int nStart = sceSdRemote(kSdRemoteBlocking, rSdGetAddr, SD_VA_SSA | nEntry);
            // Yes, the binary discards this read's result.
            sceSdRemote(kSdRemoteBlocking, rSdGetAddr, SD_VA_NAX | nEntry);
            const int nEnvX = sceSdRemote(kSdRemoteBlocking, rSdGetParam, SD_VP_ENVX | nEntry);
            const int nBit = 1 << nVoice;
            if ((nEndX & nBit) == 0) {
                ++nActive;
            } else if (bActiveOnly) {
                continue;
            }
            printf("  Voice %2.2d at %x env %x - end %d mix %d %d %d %d\n",
                   nVoice,
                   nStart,
                   nEnvX,
                   (nEndX & nBit) != 0,
                   (nVMixL & nBit) != 0,
                   (nVMixEL & nBit) != 0,
                   (nVMixR & nBit) != 0,
                   (nVMixER & nBit) != 0);
        }
    }
    SubmitSoundDriverRequest(kSoundSelectorInfo, 0);
    printf("Using %d voices total\n", nActive);
}

// Selector ConfigureSpu2Effects() submits its chorus block under.
constexpr int kSoundSelectorHardEffect = 0x10e0;

// Indices into the two-element template arguments ConfigureSpu2Effects() reads.
enum Spu2EffectSide {
    kSpu2EffectLeft,
    kSpu2EffectRight,
};

// The four `ps2_heff_stt()` entries, stored in two word and two byte fields.
enum HardEffectSttEntry {
    kHardEffectStt0,
    kHardEffectStt1,
    kHardEffectStt2,
    kHardEffectStt3,
};

// The effect depths are the configured value shifted into the high byte of a 16-bit depth.
constexpr int kSpu2EffectDepthShift = 8;

// The command block ConfigureSpu2Effects() fills and submits under kSoundSelectorHardEffect. The
// driver reads the whole 0x80-byte block. The fields are named after the template each is read
// from, and their meaning to the driver is unrecovered.
struct HardEffectCommand {
    short mSttWords[2];            // +0x00 stt entries 0 and 2
    unsigned char mSttBytes[2];    // +0x04 stt entries 1 and 3
    unsigned char mReserved06[2];  // +0x06
    int mChorusRate[2];            // +0x08
    int mChorusDepth[2];           // +0x10
    unsigned char mChorusShape[2]; // +0x18
    unsigned char mReserved1a[6];  // +0x1a
    short mNoPauseChannels;        // +0x20
#ifdef VIDEO_STANDARD_PAL
    unsigned char mReserved22[2];                              // +0x22
    unsigned int mGatherIopAddr[kHardSynthErrorLogCount];      // +0x24
    HardSynthErrorLog *mGatherEEAddr[kHardSynthErrorLogCount]; // +0x2c
    unsigned char mReserved34[0x4c];
#else
    unsigned char mReserved22[0x5e];
#endif
};

// NTSC-U/C: 0x00894d40, PAL: 0x008d9d40
alignas(64) HardEffectCommand g_hardEffectCommand;

void ConfigureSpu2Effects(int bEnable) {
    for (int nCore = 0; nCore < kSpu2CoreCount; ++nCore) {
        if (bEnable != 0 && QueryConfigFlag(kTemplateUseHardEffect, nCore) != 0) {
            sceSdEffectAttr attr; // Yes, the binary never sets the core field.
            attr.mode = QueryConfigValue(kTemplateHardEffectId, nCore) | SD_EFFECT_MODE_CLEAR;
            attr.depth_L = QueryConfigValue(kTemplateHardEffectVolumes, nCore, kSpu2EffectLeft)
                           << kSpu2EffectDepthShift;
            attr.depth_R = QueryConfigValue(kTemplateHardEffectVolumes, nCore, kSpu2EffectRight)
                           << kSpu2EffectDepthShift;
            attr.delay = QueryConfigValue(kTemplateHardDelayTime, nCore);
            attr.feedback = QueryConfigValue(kTemplateHardFeedback, nCore);
            sceSdRemote(kSdRemoteBlocking, rSdSetEffectAttr, nCore, &attr);
            sceSdRemote(kSdRemoteBlocking, rSdSetCoreAttr, SD_C_EFFECT_ENABLE | nCore, 1);
        } else {
            sceSdRemote(kSdRemoteBlocking, rSdSetCoreAttr, SD_C_EFFECT_ENABLE | nCore, 0);
            sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_P_EVOLL | nCore, 0);
            sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_P_EVOLR | nCore, 0);
        }
        sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_P_MVOLL | nCore, kSpu2MaxVolume);
        sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_P_MVOLR | nCore, kSpu2MaxVolume);
    }

    if (bEnable == 0) {
        return;
    }
    HardEffectCommand &command = g_hardEffectCommand;
    command.mSttWords[0] = QueryConfigValue(kTemplateHardEffectStt, kHardEffectStt0);
    command.mSttBytes[0] = QueryConfigValue(kTemplateHardEffectStt, kHardEffectStt1);
    command.mSttWords[1] = QueryConfigValue(kTemplateHardEffectStt, kHardEffectStt2);
    command.mSttBytes[1] = QueryConfigValue(kTemplateHardEffectStt, kHardEffectStt3);
    command.mChorusRate[kSpu2EffectLeft] = QueryConfigValue(kTemplateChorusRate, kSpu2EffectLeft);
    command.mChorusRate[kSpu2EffectRight] = QueryConfigValue(kTemplateChorusRate, kSpu2EffectRight);
    command.mChorusDepth[kSpu2EffectLeft] = QueryConfigValue(kTemplateChorusDepth, kSpu2EffectLeft);
    command.mChorusDepth[kSpu2EffectRight] =
        QueryConfigValue(kTemplateChorusDepth, kSpu2EffectRight);
    command.mChorusShape[kSpu2EffectLeft] = QueryConfigValue(kTemplateChorusShape, kSpu2EffectLeft);
    command.mChorusShape[kSpu2EffectRight] =
        QueryConfigValue(kTemplateChorusShape, kSpu2EffectRight);
    command.mNoPauseChannels = QueryConfigValue(kTemplateNoPauseChannels);
#ifdef VIDEO_STANDARD_PAL
    char szErrorFilePath[kHardSynthErrorFilePathSize];
    {
        const HxStr errorFile = QueryConfigString(kTemplateHardSynthErrorFile);
        strcpy(szErrorFilePath, errorFile.mStr != nullptr ? errorFile.mStr : g_szEmptyString);
    }
    if (strlen(szErrorFilePath) == 0) {
        for (int nLog = 0; nLog < kHardSynthErrorLogCount; ++nLog) {
            command.mGatherIopAddr[nLog] = 0;
            command.mGatherEEAddr[nLog] = nullptr;
        }
    } else {
        for (int nLog = 0; nLog < kHardSynthErrorLogCount; ++nLog) {
            command.mGatherIopAddr[nLog] =
                static_cast<unsigned int>(g_anHardSynthErrorLogIopAddress[nLog]);
            command.mGatherEEAddr[nLog] = &g_aHardSynthErrorLogs[nLog];
        }
        if (strcmp(szErrorFilePath, g_szHardSynthErrorFilePath) == 0) {
            if (g_pHardSynthErrorFile != nullptr) {
                fflush(g_pHardSynthErrorFile);
            }
        } else {
            if (g_pHardSynthErrorFile != nullptr) {
                fclose(g_pHardSynthErrorFile);
            }
            strcpy(g_szHardSynthErrorFilePath, szErrorFilePath);
            g_pHardSynthErrorFile = fopen(g_szHardSynthErrorFilePath, "w");
            if (g_pHardSynthErrorFile == nullptr) {
                printf("Couldn't open runtime hsyn error file %s\n", g_szHardSynthErrorFilePath);
                g_szHardSynthErrorFilePath[0] = '\0';
            } else {
                fprintf(g_pHardSynthErrorFile,
                        "HSyn Error Gather Log %s\n\n",
                        g_szHardSynthErrorFilePath);
            }
        }
    }
#endif
    SubmitSoundDriverRequest(kSoundSelectorHardEffect, reinterpret_cast<uintptr_t>(&command));
}

void InitSpu2Cores() {
    sceSdRemoteInit(); // Yes, the binary discards this call's result.
    sceSdRemote(kSdRemoteBlocking, rSdInit, 0);
    sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_P_MMIX | 1, kSpu2Core1Mix);
    sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_P_MMIX | 0, kSpu2Core0Mix);
    unsigned nEffectEnd = kSpu2EffectAreaTop;
    for (int nCore = 0; nCore < kSpu2CoreCount; ++nCore) {
        sceSdRemote(kSdRemoteBlocking, rSdSetCoreAttr, SD_C_EFFECT_ENABLE | nCore, 0);
        sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_P_MVOLL | nCore, kSpu2MaxVolume);
        sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_P_MVOLR | nCore, kSpu2MaxVolume);
        sceSdRemote(kSdRemoteBlocking, rSdSetAddr, SD_A_EEA | nCore, nEffectEnd);
        nEffectEnd -= kSpu2EffectAreaSize;
    }
}

// Bytes of the MIDI stream buffer, its two-word header included, which is how the input module
// measures it. The bank block follows directly at 0x00894bc0.
constexpr unsigned kMidiStreamBufferSize = 0x400;

// The layout of sceCslMidiStream with its storage, which the library reaches through a void
// pointer.
struct MidiStreamBuffer {
    unsigned int mBufferSize;
    unsigned int mValidSize;
    unsigned char mData[kMidiStreamBufferSize - 2 * sizeof(unsigned int)];
};

// Buffer groups of the input context. The first is empty and the second has the stream.
enum MidiInputBufferGroup {
    kMidiInputGroupUnused,
    kMidiInputGroupStream,
    kMidiInputGroupCount,
};

// Port the whole driver writes to.
constexpr unsigned kMidiInputPort = 0;

// Channel messages that the driver filters against the retained channel state.
constexpr unsigned kMidiStatusTypeMask = 0xf0;
constexpr unsigned kMidiChannelMask = 0xf;
constexpr unsigned kMidiControlChange = 0xb0;
constexpr unsigned kMidiProgramChange = 0xc0;
constexpr unsigned kMidiControllerBankSelectLsb = 0x20;
constexpr int kMidiChannelCount = 16;

// Shifts that pack the two data bytes above the status.
constexpr int kMidiData1Shift = 8;
constexpr int kMidiData2Shift = 16;

// NTSC-U/C: 0x00894760, PAL: 0x008d9780
alignas(64) sceCslCtx msinCtx;

// NTSC-U/C: 0x00894778, PAL: 0x008d9798
sceCslBuffGrp msinBfGrp[kMidiInputGroupCount];

// NTSC-U/C: 0x00894788, PAL: 0x008d97a8
sceCslBuffCtx msinBfCtx;

// SIF DMA sends the buffer from its start. The original placed the buffer on a cache line.
// NTSC-U/C: 0x008947c0, PAL: 0x008d97c0
alignas(64) MidiStreamBuffer msinBf;

// The program each channel last received.
// NTSC-U/C: 0x006e9bd8, PAL: 0x0072d598
int g_anChannelProgram[kMidiChannelCount] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

// The bank each channel last received.
// NTSC-U/C: 0x006e9c18, PAL: 0x0072d5d8
int g_anChannelBank[kMidiChannelCount] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

void ps2_InitMSin() {
    msinBf.mBufferSize = kMidiStreamBufferSize;
    msinCtx.buffGrpNum = kMidiInputGroupCount;
    msinBfGrp[kMidiInputGroupUnused].buffNum = 0;
    msinBfGrp[kMidiInputGroupStream].buffNum = 1;
    msinBfGrp[kMidiInputGroupStream].buffCtx = &msinBfCtx;
    msinBfCtx.sema = 0;
    msinBfCtx.buff = &msinBf;
    msinBf.mValidSize = 0;
    msinCtx.extmod = nullptr;
    msinCtx.callBack = nullptr;
    msinCtx.conf = nullptr;
    msinCtx.buffGrp = msinBfGrp;
    msinBfGrp[kMidiInputGroupUnused].buffCtx = nullptr;
    if (sceMSIn_Init(&msinCtx) != 0) {
        printf("sceMSIn_Init Error\n");
        return;
    }
    sceMSIn_PutMsg(&msinCtx, kMidiInputPort, kMidiProgramChange);
}

void SendMidiToDriver(unsigned char nStatus, unsigned char nData1, unsigned char nData2) {
    const unsigned nType = nStatus & kMidiStatusTypeMask;
    if (nType == kMidiProgramChange) {
        int &nProgram = g_anChannelProgram[nStatus & kMidiChannelMask];
        if (nProgram == nData1) {
            return;
        }
        nProgram = nData1;
    } else if (nType == kMidiControlChange && nData1 == kMidiControllerBankSelectLsb) {
        int &nBank = g_anChannelBank[nStatus & kMidiChannelMask];
        if (nBank == nData2) {
            return;
        }
        nBank = nData2;
    }
    sceMSIn_PutMsg(&msinCtx,
                   kMidiInputPort,
                   nStatus | (nData1 << kMidiData1Shift) | (nData2 << kMidiData2Shift));
}

// The driver's all-notes-off command. ReleaseSoundBanks() submits it through
// SubmitDriverAllNotesOff().
constexpr int kSoundSelectorAllNotesOff = 0xc0;

void SubmitDriverAllNotesOff() {
    SubmitSoundDriverRequest(kSoundSelectorAllNotesOff, 0);
}

void SubmitDriverSetMono(int bMono) {
    SubmitSoundDriverRequest(kSoundSelectorMono, bMono);
}

void SubmitDriverSetRemix(int bRemix) {
    SubmitSoundDriverRequest(kSoundSelectorRemix, bRemix);
}

void SubmitDriverSetPaused(int bPaused) {
    SubmitSoundDriverRequest(kSoundSelectorPause, bPaused);
}

void PollSynthEvents() {
    if (msinBf.mValidSize != 0) {
        const int nBuffer = g_nMidiEventBufferIndex;
        g_nMidiEventBufferIndex = (nBuffer + 1) & (kMidiEventBufferCount - 1);
        ezTransToIOP(g_nMidiEventIopAddress + nBuffer * kMidiEventBufferSize,
                     &msinBf,
                     msinBf.mValidSize + kMidiStreamHeaderSize);
        msinBf.mValidSize = 0;
    }
#ifdef VIDEO_STANDARD_PAL
    if ((g_nSynthEventPollCount & kHardSynthErrorLogPollMask) == 0) {
        for (auto &log : g_aHardSynthErrorLogs) {
            if (log.mLength <= 0) {
                continue;
            }
            if (g_pHardSynthErrorFile != nullptr) {
                // Yes, the binary passes the log text as the format.
                fprintf(g_pHardSynthErrorFile, log.mText);
            }
            log.mLength = 0;
        }
    }
    ++g_nSynthEventPollCount;
#endif
}

void WaitForBankTransfers() {
    while (IsBankXferBusy() != 0) {
        AsyncPumpCompletedRequests();
    }
}

void ReleaseSoundBanks() {
    SubmitDriverAllNotesOff();
    ReleaseAllBankSlots();
    delete g_pBdXfer;
    g_pBdXfer = nullptr;
    g_hdXfer.mpBdXfer = nullptr;
    g_bdBankName = "";
    g_hdBankName = "";
}

void InitSynthDriver() {
    if (g_bSynthDriverReady != 0) {
        return;
    }
    ezMidiInit(); // Yes, the binary discards the result.
    InitSpu2Cores();
    g_nMidiEventIopAddress =
        SubmitSoundDriverRequest(kSoundSelectorAllocEventBuffer, kMidiEventBufferRequest);
    ps2_InitMSin();
    if (g_anIopStagingAddress[0] == 0) {
        for (int &nAddress : g_anIopStagingAddress) {
            nAddress = static_cast<int>(
                reinterpret_cast<uintptr_t>(sceSifAllocIopHeap(kIopStagingBufferSize)));
        }
    }
    for (int &nAddress : g_anBankIopAddress) {
        nAddress =
            static_cast<int>(reinterpret_cast<uintptr_t>(sceSifAllocIopHeap(kBankIopBufferSize)));
    }
#ifdef VIDEO_STANDARD_PAL
    for (int &nAddress : g_anHardSynthErrorLogIopAddress) {
        nAddress = static_cast<int>(
            reinterpret_cast<uintptr_t>(sceSifAllocIopHeap(kHardSynthErrorLogSize)));
    }
#endif
    g_bSynthDriverReady = 1;
    g_nBankIopIndex = kBankIopIndexAfterInit;
}

void ShutdownSynthDriver() {
}

void PollSynthStream() {
    if (g_pSynthStream != nullptr) {
        g_pSynthStream->Update(g_nSynthStreamFrame, kSynthStreamReadSize);
    }
}

void StopSoundBankMovie() {
    if (g_pSynthStream != nullptr) {
        delete g_pSynthStream;
        g_pSynthStream = nullptr;
    }
}

// Selector that hands one chunk to the driver.
constexpr int kSoundSelectorXferChunk = 0x1070;

// Track of the sound-bank movie whose chunks reach the driver.
constexpr int kSoundBankMovieTrack = 15;

// Payload of an SNDH chunk, a whole bank already in memory.
struct SndhChunk : Rnd::AMovieChunkHdr {
    int mLength;                  // +0x10
    int mTag;                     // +0x14 becomes g_nSynthXferTag
    unsigned char mReserved18[8]; // +0x18
    unsigned char mData[1];       // +0x20
};

// Payload of an SNDB chunk, one piece of a bank at an offset into the destination.
struct SndbChunk : Rnd::AMovieChunkHdr {
    int mOffset;                  // +0x10 bytes past g_nBankDestAddress
    int mLength;                  // +0x14
    unsigned char mReserved18[8]; // +0x18
    unsigned char mData[1];       // +0x20
};

// NTSC-U/C: 0x004646e8, PAL: 0x004a21c8
// The body OnSoundBankMovieChunk() expands for an SNDB chunk. The out-of-line copy
// has no caller.
inline int XferBankChunk(const void *pData, int nLength, int nOffset) {
    g_chunkCommand.mStagingAddress = g_anIopStagingAddress[g_nIopStagingIndex];
    g_nIopStagingIndex = (g_nIopStagingIndex + 1) & (kIopStagingBufferCount - 1);
    g_chunkCommand.mBankAddress = 0;
    g_chunkCommand.mDest = g_nBankDestAddress + nOffset;
    g_chunkCommand.mLength = nLength;
    memset(g_chunkCommand.mPayload, 0, kSoundDriverCommandPayloadSize);
    g_chunkCommand.mTag = g_nSynthXferTag;
    ezTransToIOP(g_chunkCommand.mStagingAddress, pData, nLength);
    SubmitSoundDriverRequest(kSoundSelectorXferChunk, reinterpret_cast<uintptr_t>(&g_chunkCommand));
    return 0;
}

// NTSC-U/C: 0x00462770, PAL: 0x004a01b8
// The handler reads its chunk through the header rather than the payload argument.
void OnSoundBankMovieChunk(Rnd::AMovieChunkHdr *pHeader,
                           [[maybe_unused]] void *pPayload,
                           [[maybe_unused]] void *pData) {
    (void)GetElapsedMilliseconds(); // Yes, the binary discards the time it reads.
    if (pHeader->mTag == Rnd::g_nSndhTag) {
        const SndhChunk *pChunk = static_cast<const SndhChunk *>(pHeader);
        g_nSynthXferTag = pChunk->mTag;
        XferBankFromMemory(pChunk->mData, pChunk->mLength);
    } else if (pHeader->mTag == Rnd::g_nSndbTag) {
        const SndbChunk *pChunk = static_cast<const SndbChunk *>(pHeader);
        XferBankChunk(pChunk->mData, pChunk->mLength, pChunk->mOffset);
    } else if (pHeader->mTag == Rnd::g_nSndpTag) {
        g_nBankDestIndex = (g_nBankDestIndex + 1) & (kBankDestBufferCount - 1);
        g_nBankDestAddress = g_anBankDestAddress[g_nBankDestIndex];
    } else {
        printf("BAD CHUNK HANDED OFF TO SNDBANK MOVIE HANDLER: %s\n", FourCcToString(pHeader));
    }
}

void StartSoundBankMovie(const char *pszPath) {
    // The binary expands StopSoundBankMovie() here rather than calling it.
    if (g_pSynthStream != nullptr) {
        delete g_pSynthStream;
        g_pSynthStream = nullptr;
    }
    g_nSynthStreamFrame = 0;
    int nError;
    g_pSynthStream = new Rnd::AMovieSet(pszPath, 1, &nError);
    if (nError != 0) {
        printf("Problem starting sndbank movie: %s (errcode: %d)\n", pszPath, nError);
    }
    g_pSynthStream->AssignHandler(kSoundBankMovieTrack, OnSoundBankMovieChunk, nullptr);
    const int nTick = Application::shared()->GetSongClock()->SongTick();
    g_nSoundBankMovieTick = nTick;
    g_pSynthStream->mLoopTicks = nTick;
}

int ezMidiInit() {
    sceSifInitRpc(0);
    do {
        if (sceSifBindRpc(&gCd, kSoundDriverRpcServer, 0) < 0) {
            printf("error: sceSifBindRpc \n");
            while (true) {
            }
        }
        int nSpin = kSoundDriverBindSpin;
        while (nSpin-- != 0) {
        }
    } while (gCd.serve == nullptr);
    return 1;
}

int SubmitSoundDriverRequest(int nSelector, uintptr_t nArgument) {
    if (g_bSoundRequestPending != 0) {
        while (sceSifCheckStatRpc(&gCd.rpcd) == kSifRpcStillRunning) {
        }
        g_bSoundRequestPending = 0;
    }

    int nMode;
    int nReplySize = 0;
    if ((nSelector & kSoundSelectorSynchronous) != 0) {
        nMode = kSifRpcModeWait;
        nReplySize = sizeof(g_anSoundDriverReply);
    } else {
        nMode = SIF_RPCM_NOWAIT;
        g_bSoundRequestPending = 1;
    }

    if ((nSelector & kSoundSelectorCommandBlock) != 0) {
        sceSifCallRpc(&gCd,
                      nSelector,
                      nMode,
                      reinterpret_cast<void *>(nArgument),
                      sizeof(SoundDriverCommand),
                      g_anSoundDriverReply,
                      nReplySize,
                      nullptr,
                      nullptr);
    } else {
        g_anSoundDriverReply[0] = static_cast<unsigned int>(nArgument);
        sceSifCallRpc(&gCd,
                      nSelector,
                      nMode,
                      g_anSoundDriverReply,
                      kSoundDriverRequestSize,
                      g_anSoundDriverReply,
                      nReplySize,
                      nullptr,
                      nullptr);
    }
    return g_anSoundDriverReply[0];
}

int ezTransToIOP(int nIopAddress, const void *pSource, int nLength) {
    transData.data = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pSource));
    transData.addr = static_cast<unsigned int>(nIopAddress);
    transData.size = static_cast<unsigned int>(nLength);
    transData.mode = 0;
    FlushCache(WRITEBACK_DCACHE);
    const unsigned int nTransfer = sceSifSetDma(&transData, 1);
    while (sceSifDmaStat(nTransfer) >= 0) {
    }
    return (nTransfer != 0) ? 0 : -1;
}
