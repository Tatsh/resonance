#pragma once

#include <list>
#include <vector>

#include "memcard/memcardconnectstate.h"

class FreqAppearance;
class GlobalSettings;
class HxStr;
class JukeboxPlayList;
class Memcard;
class MemcardTask;
class MemcardUser;
class MetPersonaData;
struct MetRemixRecord;

/**
 * Game-wide owner of the memory-card queue and of the tasks that drive it.
 *
 * The class is not polymorphic, emits no RTTI, and has no embedded file path, so the title is
 * inferred, retained from the task factory at `0x001f37f8` that an earlier pass titled on the same
 * evidence. The one instance is the function-local static that shared() vends at `0x00891a80`.
 *
 * The fifteen routines from `0x001f2ae0` to `0x001f3b80` each build one MemcardTask subclass, stamp
 * it with mUser, mCard, and the next ticket, and append it to mTasks. Update() starts the task at
 * the front and retires it once it has finished.
 */
class MemcardManager {
public:
    /**
     * Resolve the one instance, constructing it on first use.
     *
     * The guard word is at `0x00695698`, and the destructor is registered to run at exit through
     * `0x001f6530`.
     *
     * @return The manager.
     * @ghidraAddress 0x001f61b8
     */
    static MemcardManager *shared();

    /**
     * Construct an empty manager that owns a new MemcardPS2.
     *
     * @ghidraAddress 0x001f2960
     */
    MemcardManager();

    /**
     * Delete the queue and release the task list.
     *
     * @ghidraAddress 0x001f6210
     */
    ~MemcardManager();

    /**
     * Advance the memory-card work by one frame.
     *
     * With no task queued, only the queue is updated. Otherwise a front task whose state is
     * kMemcardTaskFinished is deleted and dropped. When a task then remains, the front task is
     * started if it is still idle, and the queue is updated. Dropping the last task therefore
     * skips the queue's update for that frame. GameManagerImpl::DrawFrame() calls it once per
     * frame while the front end is active. The title is inferred.
     *
     * @ghidraAddress 0x001f3cb0
     */
    void Update();

    /**
     * Queue a GetConnectStateMCT for one slot.
     *
     * @param nPortSlot The packed port and slot.
     * @ghidraAddress 0x001f2ae0
     */
    void CreateGetConnectStateTask(int nPortSlot);

    /**
     * Queue a GetAllConnectStatesMCT.
     *
     * @param pStates Where the answers are appended. Borrowed, not owned.
     * @ghidraAddress 0x001f2c70
     */
    void CreateGetAllConnectStatesTask(std::vector<MemcardConnectState> *pStates);

    /**
     * Queue a MinimumSaveSpaceMCT for one slot.
     *
     * @param nPortSlot The packed port and slot.
     * @ghidraAddress 0x001f2d78
     */
    void CreateMinimumSaveSpaceTask(int nPortSlot);

    /**
     * Queue a FormatCardMCT that formats one slot.
     *
     * @param nPortSlot The packed port and slot.
     * @ghidraAddress 0x001f2e88
     */
    void CreateFormatTask(int nPortSlot);

    /**
     * Queue a FormatCardMCT that unformats one slot.
     *
     * Nothing in the image calls it.
     *
     * @param nPortSlot The packed port and slot.
     * @ghidraAddress 0x001f2f88
     */
    void CreateUnformatTask(int nPortSlot);

    /**
     * Queue a SavePersonasMCT, which serialises the roster at once.
     *
     * @param nPortSlot The packed port and slot.
     * @param roster The personas to save.
     * @ghidraAddress 0x001f3090
     */
    void CreateSavePersonasTask(int nPortSlot, const std::vector<MetPersonaData *> &roster);

    /**
     * Queue a SaveGlobalSettingsMCT, which serialises the settings at once.
     *
     * @param nPortSlot The packed port and slot.
     * @param pSettings The settings to save.
     * @ghidraAddress 0x001f3328
     */
    void CreateSaveGlobalSettingsTask(int nPortSlot, GlobalSettings *pSettings);

    /**
     * Queue a SaveJukeboxPlayListMCT, which serialises the playlist at once.
     *
     * @param nPortSlot The packed port and slot.
     * @param pPlayList The playlist to save.
     * @param nIndex The playlist number the file name carries.
     * @ghidraAddress 0x001f3458
     */
    void CreateSaveJukeboxPlayListTask(int nPortSlot, JukeboxPlayList *pPlayList, int nIndex);

    /**
     * Queue a LoadPersonasMCT.
     *
     * @param nPortSlot The packed port and slot.
     * @param pRoster Where the loaded personas are appended.
     * @ghidraAddress 0x001f37f8
     */
    void CreateLoadPersonasTask(int nPortSlot, std::vector<MetPersonaData *> *pRoster);

    /**
     * Queue a LoadGlobalSettingsMCT.
     *
     * @param nPortSlot The packed port and slot.
     * @param pSettings The settings to fill.
     * @ghidraAddress 0x001f3910
     */
    void CreateLoadGlobalSettingsTask(int nPortSlot, GlobalSettings *pSettings);

    /**
     * Queue a LoadJukeboxPlayListMCT.
     *
     * @param nPortSlot The packed port and slot.
     * @param pPlayList The playlist to fill.
     * @param nIndex The playlist number the file name carries.
     * @ghidraAddress 0x001f3a40
     */
    void CreateLoadJukeboxPlayListTask(int nPortSlot, JukeboxPlayList *pPlayList, int nIndex);

    /**
     * Queue a SaveRemixMCT.
     *
     * @param nPortSlot The packed port and slot.
     * @param remixName The remix's name.
     * @param appearances The players' appearances.
     * @param levelName The level the remix was built over.
     * @param nAlbumNum The album number the index entry records.
     * @ghidraAddress 0x001f31c8
     */
    void CreateSaveRemixTask(int nPortSlot,
                             const HxStr &remixName,
                             const std::vector<FreqAppearance> &appearances,
                             const HxStr &levelName,
                             int nAlbumNum);

    /**
     * Queue a ListRemixesMCT.
     *
     * @param nPortSlot The packed port and slot.
     * @param pRecords The collection the listing fills.
     * @ghidraAddress 0x001f3598
     */
    void CreateListRemixesTask(int nPortSlot, std::vector<MetRemixRecord> *pRecords);

    /**
     * Queue a LoadRemixMCT.
     *
     * @param nPortSlot The packed port and slot.
     * @param remixName The remix to read.
     * @ghidraAddress 0x001f36c8
     */
    void CreateLoadRemixTask(int nPortSlot, const HxStr &remixName);

    /**
     * Queue a DeleteRemixMCT.
     *
     * @param nPortSlot The packed port and slot.
     * @param remixName The remix to remove.
     * @ghidraAddress 0x001f3b80
     */
    void CreateDeleteRemixTask(int nPortSlot, const HxStr &remixName);

    /**
     * The receiver each new task reports to. Every task factory copies it into the task. +0x00
     *
     * Public because MetRemixManager's remix loaders at `0x00361418` and `0x00361480` store
     * themselves here directly before queueing a task, and the image has no setter for it.
     */
    MemcardUser *mUser;

private:
    // The last ticket handed out. Each task factory pre-increments it and uses the result as the
    // task's cookie. +0x04
    int mTicket;

    // Tasks in the order they were queued. Update() services the front of it. +0x08
    std::list<MemcardTask *> mTasks;

    // The queue every task submits operations to. +0x0c
    Memcard *mCard;
};
