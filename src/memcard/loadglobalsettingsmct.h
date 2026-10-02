#pragma once

#include "memcard/loadfilemct.h"
#include "stream/iobpreallocmemstream.h"

class GlobalSettings;

/**
 * Load the global settings from a card.
 *
 * `21LoadGlobalSettingsMCT` in the RTTI, single inheritance from `LoadFileMCT` at offset 0. An
 * instance is 0x5c bytes, from MemcardManager::CreateLoadGlobalSettingsTask()'s allocation, and
 * the vtable is at `0x007da358`.
 *
 * Execute() reads the settings file into g_abRemixStagingBuffer, over which mStream sits, through
 * the inherited load sequence. Finish() then fills mSettings from mStream before reporting.
 */
class LoadGlobalSettingsMCT : public LoadFileMCT {
public:
    /**
     * Construct an idle load.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param pSettings The settings to fill.
     * @ghidraAddress 0x00179518
     */
    LoadGlobalSettingsMCT(
        MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, GlobalSettings *pSettings);

    /** @ghidraAddress 0x00185898 */
    virtual ~LoadGlobalSettingsMCT();

    /**
     * Fill mSettings from the buffer and report through MemcardUser::OnGlobalSettingsLoaded().
     *
     * Any status other than kMemcardStatusOk reports without touching mSettings.
     *
     * @ghidraAddress 0x00186728
     */
    virtual void Finish();

    /**
     * Aim the load at the settings file and mStream's buffer, and enquire about the card.
     *
     * @ghidraAddress 0x00179600
     */
    virtual void Execute();

private:
    // The loaded file, over g_abRemixStagingBuffer. +0x38
    IOBPreallocMemStream mStream;

    // The settings to fill. +0x58
    GlobalSettings *mSettings;
};
