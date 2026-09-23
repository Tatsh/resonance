#pragma once

#include <vector>

#include "game/freqappearance.h"
#include "memcard/loadfilemct.h"
#include "memcard/memcardtask.h"
#include "memcard/memcarduser.h"
#include "os/hxstr.h"
#include "stream/iobpreallocmemstream.h"

/** Free clusters a remix save needs, which the card enquiry reports through CheckInfoOp::mFree. */
constexpr int kRemixSaveMinimumFreeClusters = 60;

/**
 * Write one remix to a card and add it to the index of the directory it lands in.
 *
 * `12SaveRemixMCT` in the RTTI descriptor at `0x008ef5e0`, with two public non-virtual bases,
 * `MemcardTask` at offset 0 and `MemcardUser` at offset 28. Two vtables belong to the class, the
 * 19-entry primary at `0x007da938` and the 21-entry `MemcardUser` table at `0x007da888`, whose
 * first two entries and whose slots 19 and 20 adjust `this` back by 28. An instance is 0xa4 bytes,
 * from the constructor's own highest store at `+0xa0`. Nothing derives from the class, so that is a
 * lower bound rather than a settled size.
 *
 * The task enquires about the card, rejects a card with fewer than kRemixSaveMinimumFreeClusters
 * free clusters, lists `/BASCUS-97125r*`, and then reads the index out of each remix save directory
 * to find one with room. It owns a `LoadFileMCT` at `+0x9c` and a `SaveFileMCT` at `+0xa0`, and
 * receives both reports as a `MemcardUser`, which is why it derives from both interfaces.
 *
 * A directory with room is one whose index holds fewer than thirteen entries. When no directory has
 * room, the task builds a fresh directory name by formatting `%02d` from one past the highest
 * directory number the listing found and appending it to g_remixDirBase.
 *
 * Six routines are recovered and not written, and the same one obstacle blocks all six. The index
 * record that the string `********** RemixIndex element **********` at `0x007d2b18` titles, with
 * the six fields `LevelName`, `RemixName`, `FileName`, `GameOK`, `Version`, and `AlbumNum`, is a
 * class the image does not name. Its descriptor is absent from the 574 in the RTTI harvest, no
 * `__FILE__` path survives for the translation unit, and no method name for it survives either, so
 * titling it would be invention. The six are the constructor at `0x00179ec0`, `OnListDir()` at
 * `0x0017a430`, the two step bodies at `0x0017a928` and `0x0017ab50`, `OnFileLoaded()` at
 * `0x0017ad70`, and `WriteIndex()` at `0x0017b318`.
 *
 * The method titles ListRemixDir(), WriteIndex(), Execute(), and Finish() are inferred. No string
 * in the image identifies any of them.
 */
class SaveRemixMCT : public MemcardTask, public MemcardUser {
public:
    /**
     * Construct an idle remix save.
     *
     * MemcardManager::CreateSaveRemixTask() at `0x001f31c8` is the one caller. Not written, for the
     * reason recorded in the class documentation.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param unknown60 Copied into mUnknown60.
     * @param appearances The players' appearances, copied into the vector at `+0x68`.
     * @param unknown74 Copied into mUnknown74.
     * @param nUnknown58 Stored in mUnknown58.
     * @ghidraAddress 0x00179ec0
     */
    SaveRemixMCT(MemcardUser *pUser,
                 Memcard *pCard,
                 int nPortSlot,
                 int nCookie,
                 const HxStr &unknown60,
                 const std::vector<FreqAppearance> &appearances,
                 const HxStr &unknown74,
                 int nUnknown58);

    /** @ghidraAddress 0x001850a8 */
    virtual ~SaveRemixMCT();

    /**
     * List every remix save directory on the card.
     *
     * Unlike the other three remix tasks, this one does not clear mStep, because Execute() has
     * already cleared it.
     *
     * @ghidraAddress 0x0017a778
     */
    void ListRemixDir();

    /** @ghidraAddress 0x0017b318 */
    void WriteIndex();

    /**
     * Start the listing unless the card cannot be read or has too little room.
     *
     * A card with fewer than kRemixSaveMinimumFreeClusters free clusters reports
     * kMemcardStatusCardFull, which abandons the task rather than proceeding to the listing.
     *
     * @param pOp The finished enquiry.
     * @ghidraAddress 0x00186a40
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /**
     * Report the finished save through MemcardUser::OnRemixSaved().
     *
     * @ghidraAddress 0x00186b60
     */
    virtual void Finish();

    /**
     * Enquire about the card and start the save.
     *
     * @ghidraAddress 0x00186a08
     */
    virtual void Execute();

    /** @ghidraAddress 0x0017ad70 */
    virtual void OnFileLoaded(int nStatus);

    /**
     * Advance past a finished write, or rewrite the index once the payload has landed.
     *
     * The status is recorded whether the write succeeded or not. A successful write at step 3 moves
     * to step 4 and rewrites the index, and every other successful write reports the task
     * finished.
     *
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x00186ad8
     */
    virtual void OnFileSaved(int nStatus);

private:
    // 0x0017a928
    // Moves the first entry of mDirNames into mCurrentDir, erases it, rewinds mStream,
    // and reads `<dir>/index` into the stream buffer through a fresh inner LoadFileMCT. Not
    // written, for the reason recorded in the class documentation. The title is inferred.
    void ReadNextIndex();
    // 0x0017ab50
    // Writes the remix payload into the chosen directory through a fresh inner
    // SaveFileMCT, and advances mStep to 2. Not written, for the same reason. The title is
    // inferred.
    void WritePayload();

    // Selects the step the four step bodies run next. Execute() clears it. +0x20
    int mStep;

    // +0x24, not written by the constructor
    int mUnknown24;

    // The directory the index is being read out of, or written into. +0x28
    HxStr mCurrentDir;

    // Every remix save directory the listing found, consumed one per step. +0x30
    std::vector<HxStr> mDirNames;

    // The parsed index of every directory the listing found, as a `std::vector` of the 20-byte
    // record `{ HxStr; int; int; int }` whose class cannot be titled. Recorded as a reserved span
    // rather than declared, for that reason. +0x3c
    unsigned char mReserved3c[0xc];

    // The directory the remix lands in, chosen by the index walk. +0x48
    HxStr mTargetDir;

    // +0x50
    HxStr mUnknown50;

    // +0x58, from the constructor's one stack argument
    int mUnknown58;

    // +0x5c
    int mUnknown5c;

    // +0x60, from the constructor's fifth register argument
    HxStr mUnknown60;

    // A copy of the constructor's sixth argument. The 20-byte element with its vptr at +0x10 is
    // FreqAppearance, the element of MetRemixRecord::appearances. +0x68
    std::vector<FreqAppearance> mAppearances;

    // +0x74, from the constructor's seventh register argument
    HxStr mUnknown74;

    // One index or payload file passes through this stream's buffer at a time. +0x7c
    IOBPreallocMemStream mStream;

    // The inner read task. Recorded as a reserved word rather than as a pointer, because the
    // destructor releases it through a virtual slot and no step body that would fix its class is
    // written. +0x9c
    unsigned char mReserved9c[4];

    // The inner save task, recorded for the same reason. +0xa0
    unsigned char mReserveda0[4];
};
