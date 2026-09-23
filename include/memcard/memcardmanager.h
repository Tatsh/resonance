#pragma once

#include <list>
#include <vector>

#include "memcard/memcardconnectstate.h"

class HxStr;
class Memcard;
class MemcardTask;
class MemcardUser;

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

private:
    // The receiver each new task reports to. Every task factory copies it into the task. +0x00
    MemcardUser *mUser;

    // The last ticket handed out. Each task factory pre-increments it and uses the result as the
    // task's cookie. +0x04
    int mTicket;

    // Tasks in the order they were queued. Update() services the front of it. +0x08
    std::list<MemcardTask *> mTasks;

    // The queue every task submits operations to. +0x0c
    Memcard *mCard;
};
