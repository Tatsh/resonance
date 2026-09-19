#pragma once

#include "os/asynccallback.h"

/** Largest read a single chunk of a BD bank transfer is asked for. */
constexpr int kBankChunkSize = 0x2000;

/**
 * Receiver that streams a BD sound bank into the sound driver one chunk at a time.
 *
 * `19CallbackXferBdToIop` in the RTTI descriptor at `0x008eeed8`, with AsyncCallback as its one
 * public base at offset 0. The vtable at `0x0081cf10` has the base's three slots and no more, so
 * the class adds no virtual of its own and declares no destructor; slot 1 addresses a copy of the
 * base destructor. An instance is 0x20 bytes, which the allocation at `0x00462018` measures rather
 * than the field range implying.
 *
 * The transfer is a loop of asynchronous reads of at most 0x2000 bytes. Each completion moves the
 * chunk just read into a staging buffer on the IOP, submits it to the driver, and queues the read
 * for the next chunk. The last chunk releases the read buffer, closes the file, and tells the
 * driver the bank is complete.
 *
 * The object registers itself with the one static CallbackXferHdToIop so that an HD transfer
 * finishing can resume a BD transfer that deferred to it.
 */
class CallbackXferBdToIop : public AsyncCallback {
public:
    /**
     * Prepare a transfer of a bank already measured.
     *
     * The transfer starts idle. StartBdBankXfer() queues the first read straight afterwards, with
     * the same chunk length it passes here, which is why the length is a parameter rather than
     * being derived from nLength inside.
     *
     * The constructor is inlined into its one caller, which is why the vptr store and all seven
     * field stores appear there as one run rather than as a call. It has no address of its own.
     *
     * @param nFile The bank, positioned at its start.
     * @param pReadBuffer The staging buffer every chunk is read into, aligned by the caller.
     * @param nDest Where the driver writes the bank.
     * @param nChunkLength The first chunk, which is the whole bank when it fits within
     *                     kBankChunkSize.
     * @param nLength The bank's length.
     */
    CallbackXferBdToIop(int nFile, char *pReadBuffer, int nDest, int nChunkLength, int nLength);

    /**
     * Move the chunk that has just been read and queue the next one.
     *
     * Reports `BD bank loading returned async error %d` for a positive status and then continues
     * regardless. Marks the transfer busy before testing whether an HD transfer holds the shared
     * command block, and returns with the transfer still marked busy when one does. Resume() is
     * what picks the transfer back up.
     *
     * @param nHandle The identifier the read was queued under.
     * @param nFile The file the read was issued against.
     * @param pBuffer The destination the read filled.
     * @param nLength The number of bytes the read requested.
     * @param nStatus Zero once the data is in place, or a positive failure code.
     * @ghidraAddress 0x00464430
     */
    virtual void Done(int nHandle, int nFile, void *pBuffer, int nLength, int nStatus);

    /**
     * Pick a deferred transfer back up.
     *
     * CallbackXferHdToIop::Done() is the only caller. Performs no work while an HD transfer still
     * holds the shared command block, or while this transfer is not marked busy.
     *
     * @ghidraAddress 0x00464da8
     */
    void Resume();

    /*!< Read the transfer is waiting on, or zero when none is outstanding. Public because
         CallbackXferHdToIop::Done() tests it through a pointer before resuming the transfer, which
         is access from outside the hierarchy, and the image has no accessor for it. +0x04 */
    int mRequestId;

private:
    // The body Done() and Resume() share. The compiler emitted it into both rather than calling it,
    // which is what an inline definition produces.
    inline void XferChunk();

    int mFile;          // +0x08
    char *mpReadBuffer; // +0x0c
    int mChunkLength;   // +0x10
    int mDest;          // +0x14
    int mRemaining;     // +0x18
    int mBusy;          // +0x1c
};
