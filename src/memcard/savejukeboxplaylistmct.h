#pragma once

#include "memcard/savefilemct.h"
#include "stream/iobpreallocmemstream.h"

class CheckInfoOp;
class JukeboxPlayList;

/**
 * Save one jukebox playlist to a card.
 *
 * It has single inheritance from `SaveFileMCT` at offset 0. An instance is 0x430 bytes, from
 * MemcardManager::CreateSaveJukeboxPlayListTask()'s allocation, and the vtable is at `0x007da9d8`.
 *
 * The constructor serialises the playlist into g_abRemixStagingBuffer through mStream at once.
 * Execute() then points the inherited save sequence at that buffer, under a file name that includes
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
     * @param nIndex The playlist number in the file name.
     * @ghidraAddress NTSC-U/C: 0x00178d58
     * @ghidraAddress PAL: 0x0017c7c8
     */
    SaveJukeboxPlayListMCT(MemcardUser *pUser,
                           Memcard *pCard,
                           int nPortSlot,
                           int nCookie,
                           JukeboxPlayList *pPlayList,
                           int nIndex);

    /**
     * @ghidraAddress NTSC-U/C: 0x00184ee8
     * @ghidraAddress PAL: 0x0018a5e8
     */
    virtual ~SaveJukeboxPlayListMCT();

#ifndef VIDEO_STANDARD_PAL
    /**
     * Start the save sequence once the card enquiry succeeds.
     *
     * The body is instruction for instruction SavePersonasMCT::OnCheckInfo(). The European release
     * inherits SaveFileMCT::OnCheckInfo() instead.
     *
     * @param pOp The finished enquiry.
     * @ghidraAddress NTSC-U/C: 0x00186650
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);
#endif

    /**
     * Report the result through MemcardUser::OnJukeboxPlayListSaved().
     *
     * @ghidraAddress NTSC-U/C: 0x001866e8
     * @ghidraAddress PAL: 0x0018bfe0
     */
    virtual void Finish();

    /**
     * Aim the save at the playlist directory and at `<g_jukeboxFileName><mIndex>.dat`, and enquire
     * about the card.
     *
     * The European release adds the playlist to SaveFileMCT::mFiles and runs
     * SaveFileMCT::Execute().
     *
     * @ghidraAddress NTSC-U/C: 0x00178ec0
     * @ghidraAddress PAL: 0x0017ca28
     */
    virtual void Execute();

private:
    // The serialised playlist, over g_abRemixStagingBuffer. +0x40c in the North American release
    // and +0x41c in the European release.
    IOBPreallocMemStream mStream;

    // The playlist number in the file name. +0x42c in the North American release and +0x43c
    // in the European release.
    int mIndex;
};
