#include "synth/callbackxferbdtoiop.h"

#include <string.h>

#include "os/async.h"
#include "os/log.h"
#include "os/mem.h"
#include "synth/midi_main.h"

// The tag the release below bills to. It is the module's original file rather than this one,
// because the whole of midi_main compiled as a single translation unit.
constexpr char kMidiMainFileName[] = "midi_main.cpp";

// Line 305 of midi_main.cpp, which the release of the read buffer passes to the tagged free.
constexpr int kFreeReadBufferLine = 0x131;

// Largest read a single chunk is asked for.
constexpr int kBankChunkSize = 0x2000;

// Selector that hands one chunk to the driver.
constexpr int kSoundSelectorXferChunk = 0x1070;

// Selector that reports a bank complete. The block is the one the transfer was started from.
constexpr int kSoundSelectorBankComplete = 0x1050;

// 0x00464430
void CallbackXferBdToIop::Done(int nHandle, int nFile, void *pBuffer, int nLength, int nStatus) {
    if (nStatus > 0) {
        LogPrintf("BD bank loading returned async error %d\n", nStatus);
    }
    mBusy = 1;
    if (g_nHdXferInFlight != 0) {
        // The transfer stays marked busy. Resume() is what continues it.
        return;
    }
    XferChunk();
}

// 0x00464da8
void CallbackXferBdToIop::Resume() {
    if (g_nHdXferInFlight != 0) {
        return;
    }
    if (mBusy == 0) {
        return;
    }
    XferChunk();
}

inline void CallbackXferBdToIop::XferChunk() {
    g_chunkCommand.mBankAddress = 0;
    g_chunkCommand.mStagingAddress = g_anIopStagingAddress[g_nIopStagingIndex];
    g_nIopStagingIndex = (g_nIopStagingIndex + 1) & (kIopStagingBufferCount - 1);
    g_chunkCommand.mLength = mChunkLength;
    g_chunkCommand.mDest = mDest;
    g_chunkCommand.mTag = g_nSynthXferTag;
    memset(g_chunkCommand.mPayload, 0, kSoundDriverCommandPayloadSize);
    XferToIop(g_chunkCommand.mStagingAddress, mpReadBuffer, mChunkLength);
    SubmitSoundDriverRequest(kSoundSelectorXferChunk, &g_chunkCommand);
    if (g_pfnBankLoadProgress != nullptr) {
        g_pfnBankLoadProgress();
    }
    mRemaining -= mChunkLength;
    if (mRemaining != 0) {
        mBusy = 0;
        mDest += mChunkLength;
        mChunkLength = (mRemaining <= kBankChunkSize) ? mRemaining : kBankChunkSize;
        mRequestId = AsyncSubmitRequest(mFile, mpReadBuffer, mChunkLength, 0, this, 0);
        return;
    }
    // Yes, the binary finishes with the transfer still marked busy.
    MemFreeTagged(g_pBdXferBuffer, kMidiMainFileName, kFreeReadBufferLine);
    FileClose(mFile);
    SubmitSoundDriverRequest(kSoundSelectorBankComplete, &g_bankCommand);
    if (g_pfnBankLoadProgress != nullptr) {
        g_pfnBankLoadProgress();
    }
    mRequestId = 0;
}
