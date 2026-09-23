#pragma once

#include "met/metscreen.h"

class MetButtonList;

/**
 * End-of-game button row shown after a solo loss.
 *
 * `17MetSoloLoseScreen` in the RTTI descriptor at `0x008ef180`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080cc90`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x00399be8` takes only the renderer and the load priority, and supplies
 * `egb` for the screen name, `metagame/_Solo` for the directory, and `end_game_butts` for the
 * container. It writes only `+0x8c`, into which it allocates a MetButtonList.
 *
 * It loads the same container as MetMultiEndScreen under the same screen name `egb`.
 *
 * The object is at least 0x90 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 19, 21, 23, 24, 30, and 36, all declared below.
 */
class MetSoloLoseScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00399be8
     */
    MetSoloLoseScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the button list.
     *
     * @ghidraAddress 0x0039e010
     */
    virtual ~MetSoloLoseScreen();

    /**
     * Allocate and construct the screen.
     *
     * The 0x90-byte allocation is billed to the tag `MsgSink`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x0039df88
     */
    static MetSoloLoseScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Rebuild the two buttons and show the screen. Slot 5.
     *
     * Makes this screen the renderer's active panel, hides it, and rebuilds the button list with
     * the retry button `egb_01.but` and the levels button `egb_02.but`, recording their two prompt
     * keys. When MetFrontEndState's `+0x0c` and `+0x10` flags are both 1, `+0x10` is cleared,
     * MetGlobalSettingsSaverScreen::StartSave() runs with this screen as the one to return to, and
     * mUnknown50 is cleared. Otherwise ShowButtons() runs.
     *
     * @ghidraAddress 0x00399f88
     */
    virtual void EnterAndShow();

    /**
     * Move along the button ring, or act on the selection. Slot 19.
     *
     * A previous or next command steps the button list and shows the selected button's prompt. A
     * select command clears the active panel, records 2 in mUnknown18, alternates the selected
     * button's state twice at 30-frame intervals, and clears the prompt.
     *
     * @param pCommand The command to handle.
     * @ghidraAddress 0x00399db8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing. Slot 21.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x0039df80
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * Play nothing. Slot 23.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x0039df70
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing. Slot 24.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x0039df78
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Exit the stats, title, and help screens and start this screen's exit. Slot 30.
     *
     * @param pButton Not read.
     * @ghidraAddress 0x0039a6e8
     */
    virtual void OnUnknownSlot30(Rnd::Button *pButton);

    /**
     * Leave for the load screen or the stage select once the exit has finished. Slot 36.
     *
     * With the retry button selected and mUnknown18 set by the select command, MetFrontEndState's
     * return screen becomes this screen and `MetLoadGameScreen` is pushed and activated. Otherwise
     * the renderer resolves its arena view, runs its two empty hooks, and `MetSoloStagesScreen` is
     * pushed and activated. The selection is then cleared.
     *
     * @ghidraAddress 0x0039a880
     */
    virtual void OnUnknownSlot36();

private:
    // 0x0039a4e8
    // Sets the `solo_lose` caption and the `no_back_title` help layout, pushes `MetHelpScreen` and
    // `MetSoloStatsScreen`, makes this screen the active panel, selects the first button, shows its
    // prompt, and runs MetScreen::EnterAndShow(). EnterAndShow() is its one caller, and the title
    // is inferred.
    void ShowButtons();

    // The retry and levels buttons. +0x8c
    MetButtonList *mUnknown8c;
};
