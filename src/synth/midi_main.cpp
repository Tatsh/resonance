#include "synth/midi_main.h"

#include <eekernel.h>
#include <libsdr.h>
#include <stdint.h>
#include <string.h>

#include "os/async.h"
#include "os/log.h"
#include "os/mem.h"
#include "synth/callbackxferhdtoiop.h"

// The tag both allocations below bill to. It is the module's original file rather than this one,
// because the whole of midi_main compiled as a single translation unit.
constexpr char kMidiMainFileName[] = "midi_main.cpp";

// Line 575 of midi_main.cpp, which the BD read buffer's allocation passes to the tagged allocator.
constexpr int kStartBdXferLine = 0x23f;

// Line 467 of midi_main.cpp, for the HD read buffer.
constexpr int kStartHdXferLine = 0x1d3;

// Selector submitted by SynthCommand's third command and by the tail of the voice report. What it
// asks the driver to do is unrecovered, so the title records the selector rather than an effect.
constexpr int kSoundSelectorUnknownD0 = 0xd0;

// libsdr's trampoline routes a zero first argument through its callback thread. Every call site in
// the game passes 1, which is the SDK's own `blocking` argument.
constexpr int kSdRemoteBlocking = 1;

constexpr int kSpu2CoreCount = 2;

constexpr int kSpu2VoicesPerCore = 24;

// Both voice-address entries reach libsd with this bit set above the 0x3e voice field. The library
// masks the voice out, so the bit does not change which voice is read; the SDK of this era
// evidently folded it into the two SD_VADDR_* constants the way it folds 0x80 into the core-level
// SD_PARAM_* ones.
constexpr int kSdVoiceAddrEntryBit = 0x40;

// Mixer routing each core starts with. Core 1 additionally takes the input core 0 feeds it.
constexpr int kSpu2Core0Mix = 0xf00;
constexpr int kSpu2Core1Mix = 0xfcc;

constexpr int kSpu2MaxVolume = 0x3fff;

// Highest byte of sound RAM, which is where core 0's effect work area ends. Each core is given the
// block below the previous one.
constexpr unsigned kSpu2EffectAreaTop = 0x1fffff;
constexpr unsigned kSpu2EffectAreaSize = 0x20000;

// 0x00894cc0
SoundDriverCommand g_chunkCommand;

// 0x00894bc0
SoundDriverCommand g_bankCommand;

// 0x00894748
int g_anIopStagingAddress[kIopStagingBufferCount];

// 0x006e9b80
int g_nIopStagingIndex;

// 0x006e9b84
int g_nBankIopAddress;

// 0x006e9b90
HxStr g_bdBankName;

// 0x006e9b98
HxStr g_hdBankName;

// 0x006e9ba4
int g_nBankDestAddress;

// 0x006e9bb4
int g_nSynthXferTag;

// 0x006e9bc4
void (*g_pfnBankLoadProgress)();

// 0x006e9dc8
int g_nHdXferInFlight;

// 0x006e9dcc
void *g_pHdXferBuffer;

// 0x006e9dd0
void *g_pBdXferBuffer;

// 0x006e9dd4
CallbackXferBdToIop *g_pBdXfer;

// 0x006e9ba8
int g_anBankDestAddress[kBankDestBufferCount];

// 0x006e9bb0
int g_nBankDestIndex;

// 0x00894750
int g_anBankIopAddress[kBankIopAddressCount];

// 0x0089475c
int g_nBankIopIndex;

// 0x00894c40
char g_szHdBankPath[kHdBankPathSize];

// 0x006e9bb8
std::vector<BankSlot> g_bankSlots;

// 0x00464378
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

// 0x00461a88
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

// 0x004642c8
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

// Scratch the four-character code is copied into. The second word is never written and terminates
// the string.
char g_szFourCc[2 * sizeof(int)];

// 0x00464b50
char *FourCcToString(const void *pFourCc) {
    *reinterpret_cast<int *>(g_szFourCc) = *static_cast<const int *>(pFourCc);
    return g_szFourCc;
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

// 0x00461db8
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
    XferToIop(g_nBankIopAddress, pData, nLength);
    SubmitSoundDriverRequest(kSoundSelectorBankComplete,
                             reinterpret_cast<uintptr_t>(&g_bankCommand));
    return 0;
}

// 0x00461f28
int StartBdBankXfer(const char *pszPath) {
    AsyncCheck(1);
    g_bankCommand.mBankAddress = g_nBankIopAddress;
    g_bankCommand.mDest = g_nBankDestAddress;
    g_bankCommand.mTag = g_nSynthXferTag;
    strcpy(g_bankCommand.mPayload, pszPath);
    FlushCache(0);
    const char *pszColon = strchr(pszPath, ':');
    const char *pszName = (pszColon != nullptr) ? pszColon + 1 : pszPath;
    const int nLength = GetUncompressedFileLength(pszName);
    // The measured length reaches the block whether or not the measurement succeeded.
    g_bankCommand.mLength = nLength;
    if (nLength <= 0) {
        LogPrintf("file open failed. %s \n", pszName);
        return -1;
    }
    const int nFile = FileOpen(pszName, 0);
    g_pBdXferBuffer = MemAllocTagged(
        kBankChunkSize + kBankBufferAlignment - 1, kMidiMainFileName, kStartBdXferLine);
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

// 0x00461c68
int StartHdBankXfer(const char *pszPath, int nPlacement) {
    AsyncCheck(1);
    strcpy(g_szHdBankPath, pszPath);
    const char *pszColon = strchr(pszPath, ':');
    const char *pszName = (pszColon != nullptr) ? pszColon + 1 : pszPath;
    const int nLength = GetUncompressedFileLength(pszName);
    if (nLength <= 0) {
        LogPrintf("file open failed. %s \n", pszName);
        return -1;
    }
    if (nPlacement >= 0 && nPlacement < kBankPlacementFirstBuffer) {
        g_nBankIopAddress = g_anBankIopAddress[0];
    } else if (nPlacement >= kBankPlacementFirstBuffer && nPlacement <= kBankPlacementRotate) {
        RotateBankIopAddress();
    }
    if (g_nBankIopAddress < 0) {
        LogPrintf("\nCan't alloc heap \n");
        return -1;
    }
    g_pHdXferBuffer =
        MemAllocTagged(nLength + kBankBufferAlignment, kMidiMainFileName, kStartHdXferLine);
    const uintptr_t nRaw = reinterpret_cast<uintptr_t>(g_pHdXferBuffer) + kBankBufferAlignment - 1;
    char *pReadBuffer =
        reinterpret_cast<char *>(nRaw & ~static_cast<uintptr_t>(kBankBufferAlignment - 1));
    g_nHdXferInFlight = AsyncLoadFileByPath(pszName, pReadBuffer, nLength, &g_hdXfer);
    return 0;
}

// 0x004620b0
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

// 0x00464ad0. The command number stays in its second argument register from entry so that the
// report below can print it.
void SynthCommand(int nCommand) {
    switch (nCommand) {
    case 0:
        break;
    case 1:
        DumpSynthVoices(1);
        break;
    case 2:
        SubmitSoundDriverRequest(kSoundSelectorUnknownD0, 0);
        break;
    default:
        LogPrintf("Unrecognized synth cmd %d\n", nCommand);
        break;
    }
}

// 0x00462558
void DumpSynthVoices(int bActiveOnly) {
    int nActive = 0;
    for (int nCore = 0; nCore < kSpu2CoreCount; ++nCore) {
        const int nMMix = sceSdRemote(kSdRemoteBlocking, rSdGetParam, SD_PARAM_MMIX | nCore);
        const int nEffect =
            sceSdRemote(kSdRemoteBlocking, rSdGetCoreAttr, SD_CORE_EFFECT_ENABLE | nCore);
        const int nVMixL = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_SWITCH_VMIXL | nCore);
        const int nVMixEL = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_SWITCH_VMIXEL | nCore);
        const int nVMixR = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_SWITCH_VMIXR | nCore);
        const int nVMixER = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_SWITCH_VMIXER | nCore);
        const int nEndX = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_SWITCH_ENDX | nCore);
        LogPrintf("Core    %2.2d: MMix %x eff %d vmix L %x %x R %x %x end %x\n",
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
            const int nStart = sceSdRemote(
                kSdRemoteBlocking, rSdGetAddr, SD_VADDR_SSA | kSdVoiceAddrEntryBit | nEntry);
            // Yes, the binary discards this read's result.
            sceSdRemote(
                kSdRemoteBlocking, rSdGetAddr, SD_VADDR_NAX | kSdVoiceAddrEntryBit | nEntry);
            const int nEnvX = sceSdRemote(kSdRemoteBlocking, rSdGetParam, SD_VPARAM_ENVX | nEntry);
            const int nBit = 1 << nVoice;
            if ((nEndX & nBit) == 0) {
                ++nActive;
            } else if (bActiveOnly) {
                continue;
            }
            LogPrintf("  Voice %2.2d at %x env %x - end %d mix %d %d %d %d\n",
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
    SubmitSoundDriverRequest(kSoundSelectorUnknownD0, 0);
    LogPrintf("Using %d voices total\n", nActive);
}

// 0x004649f8
void InitSpu2Cores() {
    sceSdRemoteInit(); // Yes, the binary discards this call's result.
    sceSdRemote(kSdRemoteBlocking, rSdInit, 0);
    sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_PARAM_MMIX | 1, kSpu2Core1Mix);
    sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_PARAM_MMIX | 0, kSpu2Core0Mix);
    unsigned nEffectEnd = kSpu2EffectAreaTop;
    for (int nCore = 0; nCore < kSpu2CoreCount; ++nCore) {
        sceSdRemote(kSdRemoteBlocking, rSdSetCoreAttr, SD_CORE_EFFECT_ENABLE | nCore, 0);
        sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_PARAM_MVOLL | nCore, kSpu2MaxVolume);
        sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_PARAM_MVOLR | nCore, kSpu2MaxVolume);
        sceSdRemote(kSdRemoteBlocking, rSdSetAddr, SD_ADDR_EEA | nCore, nEffectEnd);
        nEffectEnd -= kSpu2EffectAreaSize;
    }
}
