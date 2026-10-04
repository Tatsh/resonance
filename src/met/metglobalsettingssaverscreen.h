#pragma once

#include <vector>

#include "memcard/memcarduser.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

/**
 * Dialogue screen that writes the global settings to a memory card.
 *
 * Its RTTI descriptor is at `0x008eedc8`. It has two public non-virtual bases at fixed offsets,
 * MetScreen at `+0x00` and MemcardUser at `+140`. The object is 0x9c bytes. Its primary 39-entry
 * vtable is at `0x007f4b08`, the same length as the MetScreen table, and the class declares no new
 * virtual. The 21-entry MemcardUser table at `0x007f4a58` adjusts `this` by `-140` in every entry
 * it overrides.
 *
 * Entering the screen requests the state of the card GlobalSettings records from the memory card
 * manager. The card replies through the three MemcardUser overrides. Each raises a MetMsgScreen
 * dialogue that this screen receives the choice of, and OnMsgScreenDismissed() retries, formats,
 * or exits. Exiting brings back the screens StartSave() recorded.
 *
 * The directory uses a capital S in the image, unlike the lowercase `metagame/shared` that the
 * configuration screens use, and it is reproduced verbatim.
 *
 * The translation unit spans `0x0027c2e0` to `0x00282468`. Besides the members below, it has the
 * type function at `0x00281f30`, per-unit copies of MsgSink and MemcardUser routines, and template
 * library emissions.
 *
 * The primary slots that differ from the MetScreen table are 5, 9, 15, 23, 24, and 38. The
 * MemcardUser slots that differ are 2, 5, and 9.
 */
class MetGlobalSettingsSaverScreen : public MetScreen, public MemcardUser {
public:
    /**
     * Construct the settings-saver dialogue.
     *
     * Supplies `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for
     * the container.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x0027c4f8
     * @ghidraAddress PAL: 0x00295210
     */
    MetGlobalSettingsSaverScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Release the screen. The body is empty.
     *
     * @ghidraAddress NTSC-U/C: 0x0027c680
     * @ghidraAddress PAL: 0x00295400
     */
    virtual ~MetGlobalSettingsSaverScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x00281fc0
     * @ghidraAddress PAL: 0x0029bd18
     */
    static MetGlobalSettingsSaverScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Request the state of the recorded card. The base routine does not run.
     *
     * Slot 5.
     *
     * @ghidraAddress NTSC-U/C: 0x00282088
     * @ghidraAddress PAL: 0x0029bde0
     */
    virtual void EnterAndShow();

    /**
     * Remove the screen from the renderer, push every recorded return screen, and activate the
     * first. The base routine does not run.
     *
     * Slot 9.
     *
     * @ghidraAddress NTSC-U/C: 0x002820f8
     * @ghidraAddress PAL: 0x0029be00
     */
    virtual void BeginExit();

    /**
     * Act on the choice made in one of this screen's dialogues.
     *
     * Slot 15. The second button of `mem_check` and every button of an unrecognised dialogue
     * exit. The second button of `mem_format_check` formats the first card and raises
     * `mem_format_go`, and its first button retries. `mem_format_done` retries, and the first
     * button of `format_fail` retries while the second exits. In the European release the first
     * button of `mem_format_check` raises `save_fail_no_format` with RETRY and CONTINUE instead,
     * whose buttons act as those of `format_fail` do.
     *
     * @param name The dialogue name.
     * @param nChoice The index of the button chosen.
     * @ghidraAddress NTSC-U/C: 0x0027dc70
     * @ghidraAddress PAL: 0x002973a8
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Play nothing.
     *
     * Slot 23. Both overrides are two-instruction stubs.
     *
     * @ghidraAddress NTSC-U/C: 0x00281fb0
     * @ghidraAddress PAL: 0x0029bbf8
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 24.
     *
     * @ghidraAddress NTSC-U/C: 0x00281fb8
     * @ghidraAddress PAL: 0x0029bc00
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Resolve the base views.
     *
     * Slot 38. The body only forwards to MetScreen::ResolveContainerViews().
     *
     * @ghidraAddress NTSC-U/C: 0x00282068
     * @ghidraAddress PAL: 0x0029bdc0
     */
    virtual void ResolveContainerViews();

    /**
     * Start the save on a formatted card, or raise the dialogue for an unformatted or absent one.
     *
     * MemcardUser slot 2. A formatted card receives the save task, and `mem_save` is raised with
     * no buttons. An unformatted card raises `mem_format_check` with NO and YES, and a failed
     * enquiry raises `mem_check` with RETRY and CONTINUE. The European release also raises
     * `mem_format_check` for an enquiry that reports kMemcardStatusNotFormatted.
     *
     * @param state The card's state.
     * @param nStatus The enquiry's MemcardStatus.
     * @ghidraAddress NTSC-U/C: 0x0027c7a8
     * @ghidraAddress PAL: 0x002956b8
     */
    virtual void OnConnectState(MemcardConnectState state, int nStatus);

    /**
     * Report the format result.
     *
     * MemcardUser slot 5. Success and an already formatted card raise `mem_format_done` through
     * MetMsgScreen::ShowActive() with CONTINUE. Every other result raises `format_fail` with RETRY
     * and CONTINUE.
     *
     * @param nPortSlot The packed port and slot. The body does not read it.
     * @param nStatus The format task's MemcardStatus.
     * @ghidraAddress NTSC-U/C: 0x0027d258
     * @ghidraAddress PAL: 0x00296800
     */
    virtual void OnCardFormatted(int nPortSlot, int nStatus);

    /**
     * Report the save result.
     *
     * MemcardUser slot 9. Success exits the dialogue screen. A full card, an unknown failure, and
     * every other result raise `mem_check` with RETRY and CONTINUE and a matching text. In the
     * European release the full-card text receives nKilobytes.
     *
     * @param nPortSlot The packed port and slot. The body does not read it.
     * @param nStatus The save task's MemcardStatus.
     * @param nKilobytes The kilobytes the card lacked. European release only.
     * @ghidraAddress NTSC-U/C: 0x0027e080
     * @ghidraAddress PAL: 0x00297bc8
     */
#ifdef VIDEO_STANDARD_PAL
    virtual void OnGlobalSettingsSaved(int nPortSlot, int nStatus, int nKilobytes);
#else
    virtual void OnGlobalSettingsSaved(int nPortSlot, int nStatus);
#endif

    /**
     * Hand the return screens to the registered saver and show it, or the screens themselves.
     *
     * The saver is resolved under `MetGlobalSettingsSaverScreen` and narrowed with dynamic_cast,
     * and the result is used without a null test. When MetFrontEndState::mUsingMemcard is set, the
     * screen registered under `MetSonyScreen` pushes the saver. Otherwise it pushes every screen
     * of the list and activates the first. Seven front-end screens call it, MetStageFinishScreen
     * among them. The European release always pushes the saver.
     *
     * @param screens The registry keys of the screens to return to.
     * @ghidraAddress NTSC-U/C: 0x0027c2e0
     * @ghidraAddress PAL: 0x00295010
     */
    static void StartSave(const std::vector<HxStr> &screens);

private:
    /**
     * Replace mReturnScreens with the screens to return to.
     *
     * @ghidraAddress NTSC-U/C: 0x00282048
     * @ghidraAddress PAL: 0x0029bda0
     */
    void SetReturnScreens(const std::vector<HxStr> &screens);

    /**
     * Become the memory card manager's user and request the state of the card GlobalSettings
     * records.
     *
     * The North American OnMsgScreenDismissed() expands it. The European release enquires about
     * port 1 whatever card GlobalSettings records.
     *
     * @ghidraAddress NTSC-U/C: 0x002820a8
     * @ghidraAddress PAL: 0x00295540
     */
    void RequestConnectState();

    // Raise `mem_format_check` with NO and YES for the card the state describes. OnConnectState()
    // expands the routine.
    void ShowFormatCheck(const MemcardConnectState &state);

    // The registry keys of the screens to return to after the save. +0x90
    std::vector<HxStr> mReturnScreens;
};
