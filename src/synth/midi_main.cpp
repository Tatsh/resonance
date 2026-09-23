#include "synth/midi_main.h"

#include <csl.h>
#include <eekernel.h>
#include <libsdr.h>
#include <msin.h>
#include <stdint.h>
#include <string.h>

#include "app/application.h"
#include "os/async.h"
#include "os/cycles.h"
#include "os/loadfile.h"
#include "os/log.h"
#include "os/mem.h"
#include "rnd/moviestream.h"
#include "sch/tickclock.h"
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

// Stream frames in one bar, which SetSynthStreamBar() scales a bar by.
constexpr int kSynthStreamFramesPerBar = 19200;

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

// 0x006e9c58. The streamed audio, or null while none plays.
Rnd::MovieStream *g_pSynthStream;

// 0x006e9c5c. The frame PollSynthStream() passes to g_pSynthStream.
int g_nSynthStreamFrame;

// 0x006e9dc4. The song tick StartSoundBankMovie() opened the movie at. Nothing reads it.
int g_nSoundBankMovieTick;

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

// 0x00461bb8
void ReleaseAllBankSlots() {
    for (const auto &slot : g_bankSlots) {
        if (slot.mIopAddress != 0) {
            SubmitSoundDriverRequest(kSoundSelectorReleaseBank, static_cast<uintptr_t>(slot.mTag));
        }
    }
    g_bankSlots.clear();
}

// 0x00464628
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

// 0x00464b50
char *FourCcToString(const void *pFourCc) {
    *reinterpret_cast<int *>(g_szFourCc) = *static_cast<const int *>(pFourCc);
    return g_szFourCc;
}

// 0x00464ba0
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

// 0x00464ad0
// The command number stays in its second argument register from entry so that the
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

// 0x00894760
sceCslCtx g_midiInputContext;

// 0x00894778
sceCslBuffGrp g_aMidiInputGroups[kMidiInputGroupCount];

// 0x00894788
sceCslBuffCtx g_midiInputBuffer;

// 0x008947c0
MidiStreamBuffer g_midiStreamBuffer;

// 0x006e9bd8. The program each channel last received.
int g_anChannelProgram[kMidiChannelCount];

// 0x006e9c18. The bank each channel last received.
int g_anChannelBank[kMidiChannelCount];

// 0x00462290
void InitSynthStreamInput() {
    g_midiStreamBuffer.mBufferSize = kMidiStreamBufferSize;
    g_midiInputContext.buffGrpNum = kMidiInputGroupCount;
    g_aMidiInputGroups[kMidiInputGroupUnused].buffNum = 0;
    g_aMidiInputGroups[kMidiInputGroupStream].buffNum = 1;
    g_aMidiInputGroups[kMidiInputGroupStream].buffCtx = &g_midiInputBuffer;
    g_midiInputBuffer.sema = 0;
    g_midiInputBuffer.buff = &g_midiStreamBuffer;
    g_midiStreamBuffer.mValidSize = 0;
    g_midiInputContext.extmod = nullptr;
    g_midiInputContext.callBack = nullptr;
    g_midiInputContext.conf = nullptr;
    g_midiInputContext.buffGrp = g_aMidiInputGroups;
    g_aMidiInputGroups[kMidiInputGroupUnused].buffCtx = nullptr;
    if (sceMSIn_Init(&g_midiInputContext) != 0) {
        LogPrintf("sceMSIn_Init Error\n");
        return;
    }
    sceMSIn_PutMsg(&g_midiInputContext, kMidiInputPort, kMidiProgramChange);
}

// 0x00464928
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
    sceMSIn_PutMsg(&g_midiInputContext,
                   kMidiInputPort,
                   nStatus | (nData1 << kMidiData1Shift) | (nData2 << kMidiData2Shift));
}

// Selector ReleaseSoundBanks() submits through SubmitDriverSelectorC0(). Its effect is
// unrecovered.
constexpr int kSoundSelectorUnknownC0 = 0xc0;

// 0x004649d8
void SubmitDriverSelectorC0() {
    SubmitSoundDriverRequest(kSoundSelectorUnknownC0, 0);
}

// 0x00464b68
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
struct SndhChunk : Rnd::MovieStream::ChunkHeader {
    int mLength;                  // +0x10
    int mTag;                     // +0x14 becomes g_nSynthXferTag
    unsigned char mReserved18[8]; // +0x18
    unsigned char mData[1];       // +0x20
};

// Payload of an SNDB chunk, one piece of a bank at an offset into the destination.
struct SndbChunk : Rnd::MovieStream::ChunkHeader {
    int mOffset;                  // +0x10 bytes past g_nBankDestAddress
    int mLength;                  // +0x14
    unsigned char mReserved18[8]; // +0x18
    unsigned char mData[1];       // +0x20
};

// 0x004646e8
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
    XferToIop(g_chunkCommand.mStagingAddress, pData, nLength);
    SubmitSoundDriverRequest(kSoundSelectorXferChunk, reinterpret_cast<uintptr_t>(&g_chunkCommand));
    return 0;
}

// 0x00462770
// The handler reads its chunk through the header rather than the payload argument.
void OnSoundBankMovieChunk(Rnd::MovieStream::ChunkHeader *pHeader,
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
        LogPrintf("BAD CHUNK HANDED OFF TO SNDBANK MOVIE HANDLER: %s\n", FourCcToString(pHeader));
    }
}

// 0x00462908
void StartSoundBankMovie(const char *pszPath) {
    // The binary expands StopSoundBankMovie() here rather than calling it.
    if (g_pSynthStream != nullptr) {
        delete g_pSynthStream;
        g_pSynthStream = nullptr;
    }
    g_nSynthStreamFrame = 0;
    int nError;
    g_pSynthStream = new Rnd::MovieStream(pszPath, 1, &nError);
    if (nError != 0) {
        LogPrintf("Problem starting sndbank movie: %s (errcode: %d)\n", pszPath, nError);
    }
    g_pSynthStream->SetTrackHandler(kSoundBankMovieTrack, OnSoundBankMovieChunk, nullptr);
    const int nTick = Application::shared()->GetSongClock()->SongTick();
    g_nSoundBankMovieTick = nTick;
    g_pSynthStream->mLoopTicks = nTick;
}
