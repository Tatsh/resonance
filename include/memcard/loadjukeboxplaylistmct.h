#pragma once

#include "memcard/loadfilemct.h"
#include "stream/iobpreallocmemstream.h"

class JukeboxPlayList;

/**
 * Load one jukebox playlist from a card.
 *
 * `22LoadJukeboxPlayListMCT` in the RTTI, single inheritance from `LoadFileMCT` at offset 0. An
 * instance is 0x60 bytes, from MemcardManager::CreateLoadJukeboxPlayListTask()'s allocation, and
 * the vtable is at `0x007da2b8`.
 *
 * Execute() reads `<g_jukeboxFileName><mIndex>.dat` into g_abRemixStagingBuffer, over which
 * mStream sits, through the inherited load sequence. Finish() then fills mPlayList from mStream
 * before reporting.
 */
class LoadJukeboxPlayListMCT : public LoadFileMCT {
public:
    /**
     * Construct an idle load.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param pPlayList The playlist to fill.
     * @param nIndex The playlist number the file name carries.
     * @ghidraAddress 0x001797e8
     */
    LoadJukeboxPlayListMCT(MemcardUser *pUser,
                           Memcard *pCard,
                           int nPortSlot,
                           int nCookie,
                           JukeboxPlayList *pPlayList,
                           int nIndex);

    /** @ghidraAddress 0x00185918 */
    virtual ~LoadJukeboxPlayListMCT();

    /**
     * Fill mPlayList from the buffer and report through MemcardUser::OnJukeboxPlayListLoaded().
     *
     * Any status other than kMemcardStatusOk reports without touching mPlayList.
     *
     * @ghidraAddress 0x001867a8
     */
    virtual void Finish();

    /**
     * Aim the load at the numbered playlist file and mStream's buffer, and enquire about the card.
     *
     * @ghidraAddress 0x001798e0
     */
    virtual void Execute();

private:
    // The loaded file, over g_abRemixStagingBuffer. +0x38
    IOBPreallocMemStream mStream;

    // The playlist to fill. +0x58
    JukeboxPlayList *mPlayList;

    // The playlist number the file name carries. +0x5c
    int mIndex;
};
