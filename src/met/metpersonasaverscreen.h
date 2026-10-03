#pragma once

#include <vector>

#include "memcard/memcardconnectstate.h"
#include "memcard/memcarduser.h"
#include "met/metkbuser.h"
#include "met/metpersonadata.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

/**
 * Dialogue that writes a persona to a memory card, copies it to another card, or deletes it.
 *
 * Its RTTI descriptor is at `0x008f0060`. It has three public non-virtual bases at fixed offsets:
 * MetScreen at `+0x00`, MemcardUser at `+140`, and MetKBUser at `+144`. New() allocates 0xd8 bytes.
 *
 * The 39-entry primary vtable at `0x00805600` is the same length as the MetScreen table, and the
 * class declares no new virtual. The twenty-one-entry MemcardUser table at `0x00805550`
 * adjusts `this` by `-140` in every entry, and the three-entry MetKBUser table at `0x00805530` by
 * `-144`.
 *
 * The constructor at `0x0032ece0` takes only the renderer and the load priority, and supplies
 * `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for the
 * container. It zeroes mIsCopy, mConfirmReplace, and mRefreshingSettingsCard, and
 * default-constructs the vectors and mTargetSlot.
 *
 * A request runs as a chain of memory-card tasks. OnPanelActivated() starts CommitSave(), and
 * CommitSave() requests the target card's state. OnConnectState() checks the card and loads its
 * roster, OnPersonasLoaded() merges the persona into the roster and saves it, and OnPersonasSaved()
 * refreshes the load list. Every failure raises a MetMsgScreen dialogue that
 * OnMsgScreenDismissed() acts on.
 */
class MetPersonaSaverScreen : public MetScreen, public MemcardUser, public MetKBUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0032ece0
     */
    MetPersonaSaverScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the loaded roster.
     *
     * @ghidraAddress 0x0032f020
     */
    virtual ~MetPersonaSaverScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00338f98
     */
    static MetPersonaSaverScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Hand a save request to the registered saver screen and show it over MetLoadGameScreen.
     *
     * The saver is resolved under `MetPersonaSaverScreen` and narrowed with dynamic_cast, and the
     * result is used without a null test. After SetSaveRequest(), mIsCopy takes nIsCopy,
     * mConfirmReplace takes nConfirmReplace, and mIsDelete is cleared. The screen registered under
     * `MetLoadGameScreen` then pushes the saver and activates it. MetStageFinishScreen, the FreQ
     * maker, and the memory-card screens call it.
     *
     * @param screens The registry keys of the screens to return to.
     * @param pPersona The persona to save.
     * @param slot The card location.
     * @param nConfirmReplace Non-zero to prompt before saving over a persona of the same name.
     * @param nIsCopy Non-zero for a copy to another card.
     * @ghidraAddress 0x0032e858
     */
    static void StartSave(const std::vector<HxStr> &screens,
                          MetPersonaData *pPersona,
                          const MemcardConnectState &slot,
                          int nConfirmReplace,
                          int nIsCopy);

    /**
     * Hand a delete request to the registered saver screen and show it over MetLoadGameScreen.
     *
     * The same sequence as StartSave(), except that mIsDelete is set to 1 and mIsCopy and
     * mConfirmReplace are cleared. MetMCFreqDelScreen's slot 15 is the caller. The title is
     * inferred.
     *
     * @param screens The registry keys of the screens to return to.
     * @param pPersona The persona to delete.
     * @param slot The card location.
     * @ghidraAddress 0x0032eaa8
     */
    static void StartDelete(const std::vector<HxStr> &screens,
                            MetPersonaData *pPersona,
                            const MemcardConnectState &slot);

    /**
     * Hide the screen. Slot 5.
     *
     * The body is SetShowing(0) alone, and MetScreen::EnterAndShow() does not run.
     *
     * @ghidraAddress 0x003390c0
     */
    virtual void EnterAndShow();

    /**
     * Start the request. Slot 7.
     *
     * Clears mRefreshingSettingsCard and runs CommitSave().
     *
     * @ghidraAddress 0x003390f0
     */
    virtual void OnPanelActivated();

    /**
     * Exit to the screens the request listed. Slot 9.
     *
     * The screen is removed from the renderer, each screen in mReturnScreens is pushed, and the
     * first is made the active panel.
     *
     * @ghidraAddress 0x00339110
     */
    virtual void BeginExit();

    /**
     * Act on a dismissed dialogue. Slot 15.
     *
     * `mem_check` exits on its second button and retries otherwise. `mem_format_check` formats
     * the card on its second button and retries otherwise. `mem_format_done` retries.
     * `format_fail` retries on its first button and raises `no_save_warn` otherwise. The two
     * no-space dialogues retry on their first button and exit otherwise. `freq_replace` saves
     * over the old persona on its second button, and both it and `new_name_required` otherwise
     * open the keyboard for a new FreQ name. `freq_limit` and any other dialogue exit.
     *
     * In the European release the second button of `mem_check` and of the no-space dialogues runs
     * LeaveWithoutSaving(), as does `no_save_warn`. Declining `mem_format_check` raises
     * `save_fail_no_format` with the `Save aborted.` text. That dialogue acts like the no-space
     * dialogues. The second button of `freq_replace_catastrophe` copies the persona over the
     * load-list entry with the same name and runs BeginExit(), and the first opens the keyboard.
     *
     * @param name The dialogue that was dismissed.
     * @param nChoice The button chosen.
     * @ghidraAddress NTSC-U/C: 0x003346c8
     * @ghidraAddress PAL: 0x0035dbd8
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Resolve the container views. Slot 38, the MetScreen body alone.
     *
     * @ghidraAddress 0x003390a0
     */
    virtual void ResolveContainerViews();

    /**
     * Check the target card, then load its roster. MemcardUser slot 2.
     *
     * After a save to the settings card (mRefreshingSettingsCard set), the state is recorded as the
     * first GlobalSettings::mCardSlots entry and MetMsgScreen exits. A save or copy to a card with
     * less free space than GlobalSettings::mPersonaMinimumFreeClusters raises a no-space dialogue.
     * Otherwise the roster is loaded into mPersonas and the progress dialogue for the save, the
     * delete, or the copy is raised. An unformatted card raises `mem_format_check`, and a failed
     * enquiry `mem_check`.
     *
     * The European release omits the free-space test, treats a status of
     * kMemcardStatusNotFormatted as an unformatted card, and raises the deleting dialogue for a
     * delete. The North American release raises the save dialogue for a delete. A save that finds
     * no card identifies the first card slot through FirstCardSlotName().
     *
     * @param state The card's state, passed by value.
     * @param nStatus The enquiry status.
     * @ghidraAddress NTSC-U/C: 0x0032f5a0
     * @ghidraAddress PAL: 0x00357ca8
     */
    virtual void OnConnectState(MemcardConnectState state, int nStatus);

    /**
     * Report a finished format. MemcardUser slot 5.
     *
     * Success and status 13 both raise `mem_format_done` as the active panel, with the
     * `format_success` or `format_already` text. Any other status raises `format_fail`.
     *
     * @param nPortSlot The packed port and slot. The body does not read it.
     * @param nStatus The format status.
     * @ghidraAddress NTSC-U/C: 0x00331358
     * @ghidraAddress PAL: 0x00359ed0
     */
    virtual void OnCardFormatted(int nPortSlot, int nStatus);

    /**
     * Report a finished roster save. MemcardUser slot 7.
     *
     * Success on the settings card copies the roster into MetPersonaData::loadList(), sets
     * mRefreshingSettingsCard, and requests the card's state again. Success elsewhere exits
     * MetMsgScreen. A full card raises a no-space dialogue, status 15 the no-card dialogue, and any
     * other status `save_fail_no_space` with the `save_fail_general` text. In the European release
     * both no-space texts receive nKilobytes.
     *
     * @param nPortSlot The packed port and slot the roster went to.
     * @param nStatus The save status.
     * @param nKilobytes The kilobytes the card lacked. European release only.
     * @ghidraAddress NTSC-U/C: 0x00333210
     * @ghidraAddress PAL: 0x0035c120
     */
#ifdef VIDEO_STANDARD_PAL
    virtual void OnPersonasSaved(int nPortSlot, int nStatus, int nKilobytes);
#else
    virtual void OnPersonasSaved(int nPortSlot, int nStatus);
#endif

    /**
     * Merge the persona into the loaded roster and save it. MemcardUser slot 13.
     *
     * A persona without a name raises `new_name_required`. The roster is searched by the name
     * the persona was last loaded under and by its present name. A delete removes the old entry,
     * or raises `mem_check` when there is none. A renamed persona whose new name is taken raises
     * `new_name_required`. A persona whose name is taken replaces that entry, after prompting when
     * mConfirmReplace is set. Any other persona is appended, unless CheckPersonaLimit() refuses.
     * Neither argument is read.
     *
     * @param nPortSlot The packed port and slot.
     * @param nStatus The load status.
     * @ghidraAddress NTSC-U/C: 0x00332428
     * @ghidraAddress PAL: 0x0035b1a8
     */
    virtual void OnPersonasLoaded(int nPortSlot, int nStatus);

    /**
     * Take the name the keyboard committed as the persona's FreQ name.
     *
     * MetKBUser slot 2, in the secondary table at `0x00805530` with a `-144` adjustment. The body
     * is MetPersonaData::SetName() expanded in place.
     *
     * @param text The text the user entered.
     * @ghidraAddress 0x003391a8
     */
    virtual void OnKeyboardTextEntered(const HxStr &text);

    /**
     * Silence the cycle-left sound.
     *
     * Both overrides are two-instruction stubs, written inline with an empty body.
     *
     * @ghidraAddress 0x00338f88
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x00338f90
     */
    virtual void PlayCycleRightSound(int) {
    }

private:
    /**
     * Send the request to the card, or retain the persona in memory.
     *
     * A copy, a non-zero mTargetSlot.mPortSlot, a non-zero MetFrontEndState::mUsingMemcard, or a
     * missing persona queries MemcardManager for the target's state. Otherwise the persona replaces
     * the pre-fab identity or the saved persona of the same name, or is appended to
     * MetPersonaData::savedList() as a copy, and the screen exits. The name is inferred.
     *
     * @ghidraAddress 0x0032f1e0
     */
    void CommitSave();

    /**
     * Delete every persona the screen built and empty mPersonas.
     *
     * Each element is released through slot 1 of the table its vptr at `+0x168` addresses, with
     * the deleting `__in_chrg` value. The title is inferred.
     *
     * @ghidraAddress 0x0032f4d0
     */
    void ClearPersonas();

    /**
     * Report whether the roster has room for one more persona.
     *
     * A roster of eight raises `freq_limit`. The name is inferred.
     *
     * @return 1 when there is room, 0 when the dialogue was raised.
     * @ghidraAddress NTSC-U/C: 0x00331d48
     * @ghidraAddress PAL: 0x0035aa48
     */
    int CheckPersonaLimit();

    /**
     * Copy the persona over the first persona of the game manager's roster, when there is one.
     *
     * The name is inferred.
     *
     * @ghidraAddress 0x00332130
     */
    void SyncActivePersona();

    /**
     * Request confirmation through `freq_replace` before saving over a persona of the same name.
     *
     * The European release takes a flag that raises the dialogue as `freq_replace_catastrophe`
     * instead, with the same text and buttons. The name is inferred.
     *
     * @param bInLoadList Non-zero when the persona of the same name is in
     *                    MetPersonaData::loadList() rather than on the card. European release only.
     * @ghidraAddress NTSC-U/C: 0x003350f8
     * @ghidraAddress PAL: 0x0035ee80
     */
#ifdef VIDEO_STANDARD_PAL
    void AskToReplace(int bInLoadList);

    /**
     * Retain the persona in MetPersonaData::loadList() without saving it, and exit.
     *
     * A persona of the same name already in the list raises AskToReplace() with the flag set
     * instead. Otherwise a copy of the persona is appended and BeginExit() runs. The European
     * release added the routine. The name is inferred.
     *
     * @ghidraAddress PAL: 0x0035da70
     */
    void KeepInLoadList();

    /**
     * Exit without saving.
     *
     * KeepInLoadList() runs when mConfirmReplace is set for a save that is not a copy, and
     * BeginExit() otherwise. OnMsgScreenDismissed() expands this test at three sites, and no
     * address of the routine remains.
     */
    void LeaveWithoutSaving();
#else
    void AskToReplace();
#endif

    /**
     * Take the screens to return to, the persona to save, and the card location.
     *
     * The screen list replaces mReturnScreens, the persona goes to mPersona, and the location is
     * copied into mTargetSlot. StartSave() and StartDelete() are the callers.
     *
     * @param screens The registry keys of the screens to return to.
     * @param pPersona The persona to save.
     * @param slot The card location.
     * @ghidraAddress 0x00339020
     */
    void SetSaveRequest(const std::vector<HxStr> &screens,
                        MetPersonaData *pPersona,
                        const MemcardConnectState &slot);

    // 1 for a delete request from StartDelete(), cleared by StartSave(). The constructor does not
    // write it. +0x94
    int mIsDelete;
    // Non-zero for a copy to another card. Written by StartSave() from its last argument. +0x98
    int mIsCopy;
    // Non-zero to prompt before saving over a persona of the same name. Written by StartSave() from
    // its fourth argument. +0x9c
    int mConfirmReplace;
    // The roster loaded from the target card. ClearPersonas() deletes every element. +0xa0
    std::vector<MetPersonaData *> mPersonas;
    // The registry keys of the screens to return to after the request. +0xac
    std::vector<HxStr> mReturnScreens;
    // The persona to save, recorded by SetSaveRequest(). Not written by the constructor. +0xb8
    MetPersonaData *mPersona;
    // Set while the settings card's state is being read again after a save. +0xbc
    int mRefreshingSettingsCard;
    // The card location to save to. The constructor's inline MemcardConnectState construction
    // stores mType, mFormatted, and mFree out of offset order. +0xc0
    MemcardConnectState mTargetSlot;
};
