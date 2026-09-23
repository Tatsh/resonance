#pragma once

#include "memcard/savefilemct.h"
#include "stream/iobpreallocmemstream.h"

class CheckInfoOp;
class JukeboxPlayList;

/**
 * Save one jukebox playlist to a card.
 *
 * `22SaveJukeboxPlayListMCT` in the RTTI, single inheritance from `SaveFileMCT` at offset 0. An
 * instance is 0x430 bytes, from MemcardManager::CreateSaveJukeboxPlayListTask()'s allocation, and
 * the vtable is at `0x007da9d8`.
 *
 * The constructor serialises the playlist into g_abRemixStagingBuffer through mStream at once.
 * Execute() then points the inherited save sequence at that buffer, under a file name that carries
 * mIndex, and starts it with a card enquiry.
 */
class SaveJukeboxPlayListMCT : public SaveFileMCT {
public:
    /**
     * Construct an idle save and serialise the playlist through JukeboxPlayList::save().
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param pPlayList The playlist to save.
     * @param nIndex The playlist number the file name carries.
     * @ghidraAddress 0x00178d58
     */
    SaveJukeboxPlayListMCT(MemcardUser *pUser,
                           Memcard *pCard,
                           int nPortSlot,
                           int nCookie,
                           JukeboxPlayList *pPlayList,
                           int nIndex);

    /** @ghidraAddress 0x00184ee8 */
    virtual ~SaveJukeboxPlayListMCT();

    /**
     * Start the save sequence once the card enquiry succeeds.
     *
     * The body is instruction for instruction SavePersonasMCT::OnCheckInfo().
     *
     * @param pOp The finished enquiry.
     * @ghidraAddress 0x00186650
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /**
     * Report the result through MemcardUser::OnJukeboxPlayListSaved().
     *
     * @ghidraAddress 0x001866e8
     */
    virtual void Finish();

    /**
     * Aim the save at the playlist directory and at `<g_jukeboxFileName><mIndex>.dat`, and enquire
     * about the card.
     *
     * @ghidraAddress 0x00178ec0
     */
    virtual void Execute();

private:
    // The serialised playlist, over g_abRemixStagingBuffer. +0x40c
    IOBPreallocMemStream mStream;

    // The playlist number the file name carries. +0x42c
    int mIndex;
};
