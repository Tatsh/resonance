#pragma once

#include <vector>

#include "memcard/loadfilemct.h"

class MetPersonaData;

/**
 * Load the FreQ roster from a card.
 *
 * `15LoadPersonasMCT` in the RTTI, single inheritance from `LoadFileMCT` at offset 0. An instance
 * is 0x3c bytes and the vtable is at `0x007da3f8`.
 *
 * Execute() reads the roster file into g_abRemixStagingBuffer through the inherited load sequence.
 * Finish() then rebuilds each persona from the buffer and appends it to mRoster before reporting.
 */
class LoadPersonasMCT : public LoadFileMCT {
public:
    /**
     * Construct an idle load.
     *
     * The constructor is inlined into MemcardManager::CreateLoadPersonasTask() at `0x001f37f8`,
     * its one site, and no address of its own survives.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param pRoster Where the loaded personas are appended. Borrowed, not owned.
     */
    LoadPersonasMCT(MemcardUser *pUser,
                    Memcard *pCard,
                    int nPortSlot,
                    int nCookie,
                    std::vector<MetPersonaData *> *pRoster);

    /** @ghidraAddress 0x00185828 */
    virtual ~LoadPersonasMCT();

    /**
     * Rebuild the roster from the buffer and report through MemcardUser::OnPersonasLoaded().
     *
     * On kMemcardStatusOk the buffer is read back as a persona count followed by that many
     * personas, each allocated and filled through MetPersonaData::Load() and appended to mRoster.
     * Any other status reports without touching mRoster.
     *
     * @ghidraAddress 0x00179360
     */
    virtual void Finish();

    /**
     * Aim the load at the roster file and g_abRemixStagingBuffer, and enquire about the card.
     *
     * @ghidraAddress 0x00179178
     */
    virtual void Execute();

private:
    // Where the loaded personas are appended. Borrowed, not owned. +0x38
    std::vector<MetPersonaData *> *mRoster;
};
