#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

/**
 * Row of buttons along the top of the jukebox.
 *
 * `26MetJukeboxTopButtonsScreen` in the RTTI descriptor at `0x008ef910`, with MetScreen as its one
 * public non-virtual base at offset 0. The object is 0x9c bytes and the 39-entry vtable is at
 * `0x007ef8b0`, the same length as the MetScreen table, so the class declares no virtual of its
 * own.
 *
 * The constructor at `0x00240c28` takes only the renderer and the load priority, and supplies
 * `jbb` for the screen name, `metagame/Shared` for the directory, and `juke_butts` for the
 * container. It zeroes the members below and then allocates a MetButtonList of 0x18 bytes tagged
 * `MetButtonList` into mUnknown94.
 *
 * The destructor at `0x00246778` restores the vptr, frees the buffer at `+0x90` through the
 * untagged path, runs the MetScreen destructor, and releases the object with the tag `MsgSink`.
 * That free is the inlined HxStr destructor of mUnknown8c rather than a statement of the
 * destructor's own, which is what identifies the eight bytes at `+0x8c` as an `HxStr`. The
 * constructor zeroing both of its words, and the destructor freeing the second word alone, are
 * the two halves of that evidence. An earlier reading recorded the four words as integers and
 * could not attribute the free.
 *
 * Nothing frees the button list, so it is never released.
 *
 * The four buttons each show one or two of five sub-screens (custom remixes, factory remixes, the
 * playlist editor, its lower-left panel, and its done panel), and mUnknown8c records the one that
 * receives the commands this screen does not consume.
 *
 * Nine slots differ from the MetScreen table.
 *
 *  - 1 `0x00246778` the destructor.
 *  - 5 `0x00241b08` EnterAndShow().
 *  - 9 `0x00242140` BeginExit().
 *  - 19 `0x002467e8` HandleCommand().
 *  - 26 `0x002468d8` OnUnknownSlot26(), overridden empty.
 *  - 33 `0x002468b8` OnUnknownSlot33().
 *  - 36 `0x00241120` OnUnknownSlot36().
 *  - 38 `0x00240e28` ResolveContainerViews().
 */
class MetJukeboxTopButtonsScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00240c28
     */
    MetJukeboxTopButtonsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00246778
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
     * @ghidraAddress 0x002466f0
     */
    static MetJukeboxTopButtonsScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Enter, wait for the five sub-screens to load, push them, and show the first button's panel.
     *
     * The title is the `met_jukebox_title` title. When MetFrontEndState::mUnknown18 is set, the
     * value moves to MetFrontEndState::mUnknown1c, the help screen takes the `standard_title`
     * layout and is pushed, and this screen becomes the renderer's active panel. The load wait
     * pumps the asynchronous loaders until every sub-screen reports its container loaded, and
     * MetRemixManager::PrunePlayList() runs before the sub-screens are pushed.
     *
     * @ghidraAddress 0x00241b08
     */
    virtual void EnterAndShow();

    /**
     * Begin the exit and exit all five sub-screens.
     *
     * @ghidraAddress 0x00242140
     */
    virtual void BeginExit();

    /**
     * Act on a command.
     *
     * Left and right step the button ring and show the new button's panel. Back records 0 in
     * MetScreen::mUnknown18 and begins the exit. Every other command goes to the sub-screen that
     * mUnknown8c records.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002467e8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Do nothing.
     *
     * @param flTime Not read.
     * @ghidraAddress 0x002468d8
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Show the selected button's panel.
     *
     * @ghidraAddress 0x002468b8
     */
    virtual void OnUnknownSlot33();

    /**
     * Return to the remix type screen after a cancel.
     *
     * When MetScreen::mUnknown18 is 0, the left gizmo, title, and remix type screens are pushed
     * and the remix type screen is activated. Any other value does nothing.
     *
     * @ghidraAddress 0x00241120
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container views and add the four buttons.
     *
     * The buttons are saved remixes, factory remixes, edit playlist, and done, in that order, and
     * the first is selected.
     *
     * @ghidraAddress 0x00240e28
     */
    virtual void ResolveContainerViews();

private:
    // Hide the five sub-screens, show the ones the selected button owns, record the sub-screen
    // that receives commands in mUnknown8c, and replace the title. 0x00241320
    void ShowSelectedPanel();

    HxStr mUnknown8c;          // +0x8c
    MetButtonList *mUnknown94; // +0x94
    int mUnknown98;            // +0x98
};
