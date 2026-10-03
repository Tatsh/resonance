#pragma once

#include "memcard/memcardconnectstate.h"

/**
 * Receiver notified once a memory-card task has finished.
 *
 * Its RTTI descriptor is at `0x0086f618`. It has no base class and no data members. An instance is
 * the four-byte vptr alone, and the vtable is at `0x007daf78`.
 *
 * The interface declares one method per `MemcardTask` subclass, and every body is a single
 * `jr ra`. An implementation overrides only the tasks it starts. Each method is pinned to its
 * task by the task's reporting virtual. The reporting virtual reads one fixed vtable slot of the
 * user the task was constructed with.
 *
 * Ten front-end screens derive from this interface alongside `MetScreen`, at offset 140 or 164,
 * and four of the remix tasks derive from it as well, at offset 28. Each of those four runs an
 * inner `LoadFileMCT` or `SaveFileMCT` and receives the inner task's report here.
 *
 * Every method title is inferred from the task that reports through the slot. No string in the
 * image identifies any of them. No task reports through two of the slots, and both are recorded
 * below as unrecovered.
 */
class MemcardUser {
public:
    /**
     * Release the receiver.
     *
     * Occupies vtable slot 1. The body is empty.
     *
     * @ghidraAddress 0x00184448
     */
    virtual ~MemcardUser();

    /**
     * Report the state of one slot. Slot 2, from `GetConnectStateMCT`.
     *
     * The state arrives by value. The slot's compiled body therefore destroys the argument's
     * string rather than being empty like the rest.
     *
     * @param state What the slot reported.
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x00184478
     */
    virtual void OnConnectState(MemcardConnectState state, int nStatus);

    /**
     * Report that every slot has been enquired about. Slot 3, from `GetAllConnectStatesMCT`.
     *
     * The task reports each slot through OnConnectState() as its enquiry finishes, and reports
     * here once with no argument when the last of them is done.
     *
     * @ghidraAddress 0x001844a0
     */
    virtual void OnAllConnectStates();

    /**
     * Report the space a save would need against the space a card has. Slot 4, from
     * `MinimumSaveSpaceMCT`.
     *
     * The second argument is the task's measurement rather than its status. This slot is the only
     * one that does not receive a MemcardStatus.
     *
     * The European release adds nSkipWarning and nCampaign. `MinimumSaveSpaceMCT` passes two
     * members that only its constructor writes, with zero. Their titles are inferred from
     * MetMemDetectScreen::OnMinimumSaveSpace(), the one override that reads them.
     *
     * @param nPortSlot The packed port and slot.
     * @param nSpace The measurement.
     * @param nSkipWarning Non-zero to show no space warning at all. European release only.
     * @param nCampaign 1 to warn against the smaller campaign requirement. European release only.
     * @ghidraAddress NTSC-U/C: 0x001844a8
     * @ghidraAddress PAL: 0x001897a0
     */
#ifdef VIDEO_STANDARD_PAL
    virtual void OnMinimumSaveSpace(int nPortSlot, int nSpace, int nSkipWarning, int nCampaign);
#else
    virtual void OnMinimumSaveSpace(int nPortSlot, int nSpace);
#endif

    /**
     * Report a finished format. Slot 5, from `FormatCardMCT` when it was requested to format.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x001844b0
     */
    virtual void OnCardFormatted(int nPortSlot, int nStatus);

    /**
     * Report a finished unformat. Slot 6, from `FormatCardMCT` when it was requested to unformat.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x001844b8
     */
    virtual void OnCardUnformatted(int nPortSlot, int nStatus);

    /**
     * Report a finished save of the FreQ roster. Slot 7, from `SavePersonasMCT`.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @param nKilobytes SaveFileMCT::mKilobytes, the kilobytes the card lacked when nStatus is
     *                   kMemcardStatusCardFull. European release only.
     * @ghidraAddress NTSC-U/C: 0x001844c0
     * @ghidraAddress PAL: 0x001897b8
     */
#ifdef VIDEO_STANDARD_PAL
    virtual void OnPersonasSaved(int nPortSlot, int nStatus, int nKilobytes);
#else
    virtual void OnPersonasSaved(int nPortSlot, int nStatus);
#endif

    /**
     * Report a finished save of one remix. Slot 8, from `SaveRemixMCT`.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @param nKilobytes SaveFileMCT::mKilobytes, the kilobytes the card lacked when nStatus is
     *                   kMemcardStatusCardFull. European release only.
     * @ghidraAddress NTSC-U/C: 0x001844c8
     * @ghidraAddress PAL: 0x001897c0
     */
#ifdef VIDEO_STANDARD_PAL
    virtual void OnRemixSaved(int nPortSlot, int nStatus, int nKilobytes);
#else
    virtual void OnRemixSaved(int nPortSlot, int nStatus);
#endif

    /**
     * Report a finished save of the global settings. Slot 9, from `SaveGlobalSettingsMCT`.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @param nKilobytes SaveFileMCT::mKilobytes, the kilobytes the card lacked when nStatus is
     *                   kMemcardStatusCardFull. European release only.
     * @ghidraAddress NTSC-U/C: 0x001844d0
     * @ghidraAddress PAL: 0x001897c8
     */
#ifdef VIDEO_STANDARD_PAL
    virtual void OnGlobalSettingsSaved(int nPortSlot, int nStatus, int nKilobytes);
#else
    virtual void OnGlobalSettingsSaved(int nPortSlot, int nStatus);
#endif

    /**
     * Report a finished save of a jukebox playlist. Slot 10, from `SaveJukeboxPlayListMCT`.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @param nKilobytes SaveFileMCT::mKilobytes, the kilobytes the card lacked when nStatus is
     *                   kMemcardStatusCardFull. European release only.
     * @ghidraAddress NTSC-U/C: 0x001844d8
     * @ghidraAddress PAL: 0x001897d0
     */
#ifdef VIDEO_STANDARD_PAL
    virtual void OnJukeboxPlayListSaved(int nPortSlot, int nStatus, int nKilobytes);
#else
    virtual void OnJukeboxPlayListSaved(int nPortSlot, int nStatus);
#endif

    /**
     * Report a finished listing of the saved remixes. Slot 11, from `ListRemixesMCT`.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x001844e0
     */
    virtual void OnRemixesListed(int nPortSlot, int nStatus);

    /**
     * Report a finished load of one remix. Slot 12, from `LoadRemixMCT`.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x001844e8
     */
    virtual void OnRemixLoaded(int nPortSlot, int nStatus);

    /**
     * Report a finished load of the FreQ roster. Slot 13, from `LoadPersonasMCT`.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x001844f0
     */
    virtual void OnPersonasLoaded(int nPortSlot, int nStatus);

    /**
     * Report a finished load of the global settings. Slot 14, from `LoadGlobalSettingsMCT`.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x001844f8
     */
    virtual void OnGlobalSettingsLoaded(int nPortSlot, int nStatus);

    /**
     * Report a finished load of a jukebox playlist. Slot 15, from `LoadJukeboxPlayListMCT`.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x00184500
     */
    virtual void OnJukeboxPlayListLoaded(int nPortSlot, int nStatus);

    /**
     * Report a finished deletion of one remix. Slot 16, from `DeleteRemixMCT`.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x00184508
     */
    virtual void OnRemixDeleted(int nPortSlot, int nStatus);

    /**
     * Do nothing.
     *
     * Slot 17. No task in the image reports through this slot and no subclass overrides it. Its
     * purpose and argument list cannot be established. The two arguments are declared to match
     * every neighbouring slot.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x00184510
     */
    virtual void UnusedFirstReport(int nPortSlot, int nStatus);

    /**
     * Do nothing.
     *
     * Slot 18. Recorded on the same evidence as UnusedFirstReport().
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x00184518
     */
    virtual void UnusedSecondReport(int nPortSlot, int nStatus);

    /**
     * Report a finished load of one file. Slot 19, from `LoadFileMCT`.
     *
     * The slot receives the status alone, without a port and slot. Only the two inner file tasks
     * report this way.
     *
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x00184520
     */
    virtual void OnFileLoaded(int nStatus);

    /**
     * Report a finished save of one file. Slot 20, from `SaveFileMCT`.
     *
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x00184528
     */
    virtual void OnFileSaved(int nStatus);
};
