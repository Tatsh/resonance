#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

/**
 * Row of buttons along the top of the jukebox.
 *
 * Its RTTI descriptor is at `0x008ef910`. It has MetScreen as its one public non-virtual base at
 * offset 0. The object is 0x9c bytes and the 39-entry vtable is at `0x007ef8b0`, the same length as
 * the MetScreen table, and the class declares no new virtual.
 *
 * The constructor at `0x00240c28` takes only the renderer and the load priority, and supplies
 * `jbb` for the screen name, `metagame/Shared` for the directory, and `juke_butts` for the
 * container. It zeroes the members below and then allocates a MetButtonList of 0x18 bytes tagged
 * `MetButtonList` into mButtons.
 *
 * The destructor at `0x00246778` restores the vptr, frees the buffer at `+0x90` through the
 * untagged path, runs the MetScreen destructor, and releases the object with the tag `MsgSink`.
 * That free is the inlined HxStr destructor of mCommandTargetScreen rather than a statement of the
 * destructor's own, which is what identifies the eight bytes at `+0x8c` as an `HxStr`. The
 * constructor zeroing both of its words, and the destructor freeing the second word alone, are
 * the two halves of that evidence.
 *
 * Nothing frees the button list, so it is never released.
 *
 * The four buttons each show one or two of five sub-screens (custom remixes, factory remixes, the
 * playlist editor, its lower-left panel, and its done panel), and mCommandTargetScreen records the
 * one that receives the commands this screen does not consume.
 *
 * Nine slots differ from the MetScreen table.
 *
 *  - 1 `0x00246778` the destructor.
 *  - 5 `0x00241b08` EnterAndShow().
 *  - 9 `0x00242140` BeginExit().
 *  - 19 `0x002467e8` HandleCommand().
 *  - 26 `0x002468d8` UpdateIdle(), overridden empty.
 *  - 33 `0x002468b8` OnEnterFinished().
 *  - 36 `0x00241120` OnExitFinished().
 *  - 38 `0x00240e28` ResolveContainerViews().
 */
class MetJukeboxTopButtonsScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x00240c28
     * @ghidraAddress PAL: 0x002556d8
     */
    MetJukeboxTopButtonsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x00246778
     * @ghidraAddress PAL: 0x0025b890
     */
    virtual ~MetJukeboxTopButtonsScreen();

    /**
     * Build the screen on the heap.
     *
     * The object is allocated with the tag `MsgSink`.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x002466f0
     * @ghidraAddress PAL: 0x0025b808
     */
    static MetJukeboxTopButtonsScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Enter, wait for the five sub-screens to load, push them, and show the first button's panel.
     *
     * The title is the `met_jukebox_title` title. When MetFrontEndState::mPendingTransition is set,
     * the value moves to MetFrontEndState::mLastTransition, the help screen takes the
     * `standard_title` layout and is pushed, and this screen becomes the renderer's active panel.
     * The load wait pumps the asynchronous loaders until every sub-screen reports its container
     * loaded, and MetRemixManager::PrunePlayList() runs before the sub-screens are pushed.
     *
     * @ghidraAddress NTSC-U/C: 0x00241b08
     * @ghidraAddress PAL: 0x00256940
     */
    virtual void EnterAndShow();

    /**
     * Begin the exit and exit all five sub-screens.
     *
     * @ghidraAddress NTSC-U/C: 0x00242140
     * @ghidraAddress PAL: 0x002570e8
     */
    virtual void BeginExit();

    /**
     * Act on a command.
     *
     * Left and right step the button ring and show the new button's panel. Back records 0 in
     * MetScreen::mExitChoice and begins the exit. Every other command goes to the sub-screen that
     * mCommandTargetScreen records.
     *
     * @param pCommand The command.
     * @ghidraAddress NTSC-U/C: 0x002467e8
     * @ghidraAddress PAL: 0x0025b910
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Do nothing.
     *
     * @param flTime Not read.
     * @ghidraAddress NTSC-U/C: 0x002468d8
     * @ghidraAddress PAL: 0x0025ba00
     */
    virtual void UpdateIdle(float flTime);

    /**
     * Show the selected button's panel.
     *
     * @ghidraAddress NTSC-U/C: 0x002468b8
     * @ghidraAddress PAL: 0x0025b9e0
     */
    virtual void OnEnterFinished();

    /**
     * Return to the remix type screen after a cancel.
     *
     * When MetScreen::mExitChoice is 0, the left gizmo, title, and remix type screens are pushed
     * and the remix type screen is activated. Any other value does nothing.
     *
     * @ghidraAddress NTSC-U/C: 0x00241120
     * @ghidraAddress PAL: 0x00255d20
     */
    virtual void OnExitFinished();

    /**
     * Resolve the container views and add the four buttons.
     *
     * The buttons are saved remixes, factory remixes, edit playlist, and done, in that order, and
     * the first is selected.
     *
     * @ghidraAddress NTSC-U/C: 0x00240e28
     * @ghidraAddress PAL: 0x00255950
     */
    virtual void ResolveContainerViews();

private:
    // Hide the five sub-screens, show the ones the selected button owns, record the sub-screen
    // that receives commands in mCommandTargetScreen, and replace the title.
    // NTSC-U/C: 0x00241320, PAL: 0x00255f98
    void ShowSelectedPanel();

    HxStr mCommandTargetScreen; // +0x8c
    MetButtonList *mButtons;    // +0x94
    // Zeroed by the constructor and never read or written again.
    int mUnused; // +0x98
};
