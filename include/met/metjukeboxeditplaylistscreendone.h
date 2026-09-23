#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"

/**
 * Confirmation buttons for the jukebox playlist editor.
 *
 * `32MetJukeboxEditPlaylistScreenDone` in the RTTI descriptor at `0x008eebf8`, with MetScreen as
 * its one public non-virtual base at offset 0. The object is 0x9c bytes and the 39-entry vtable is
 * at `0x007ee0f8`, the same length as the MetScreen table, so the class declares no virtual of its
 * own.
 *
 * The constructor at `0x00231728` takes only the renderer and the load priority, and supplies
 * `jbd` for the screen name, `metagame/Shared` for the directory, and `juke_done_butts` for the
 * container. It clears the four members below and then allocates a MetButtonList of 0x18 bytes
 * tagged `MetButtonList` into mUnknown8c, so the clearing of that member is immediately
 * overwritten.
 *
 * The three buttons play the playlist in random order, play it in order, and save it. Slot 30
 * records the choice once the button has finished flashing, and slot 36 acts on it once the
 * screen has left.
 *
 * The destructor at `0x00237210` restores the vptr, runs the MetScreen destructor, and releases
 * the object with the tag `MsgSink`. It releases nothing of its own. Nothing else frees the
 * button list at `+0x8c` either, so the list is never freed.
 *
 * Fourteen slots differ from the MetScreen table. Slots 20 through 24 sit eight bytes apart at
 * `0x00237160` through `0x00237180` and are two-instruction `jr ra` stubs, so this screen plays
 * none of those five sounds. The others are 5 `0x00237330`, 17 `0x002373e0`, 19 `0x00237268`,
 * 30 `0x002321f8`, 33 `0x002373a8`, 36 `0x00231b50`, and 38 `0x00231908`.
 */
class MetJukeboxEditPlaylistScreenDone : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00231728
     */
    MetJukeboxEditPlaylistScreenDone(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00237210
     */
    virtual ~MetJukeboxEditPlaylistScreenDone();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00237188
     */
    static MetJukeboxEditPlaylistScreenDone *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Select the first button and enable the save button only when MetFrontEndState::mUnknown0c
     * is set.
     *
     * Slot 5. The MetScreen body runs first. A clear flag puts the save button in state 3, which
     * MetButtonList passes over.
     *
     * @ghidraAddress 0x00237330
     */
    virtual void EnterAndShow();

    /**
     * Show or hide the screen, posting the help text when it shows.
     *
     * Slot 17.
     *
     * @param nShowing Non-zero to draw the screen.
     * @ghidraAddress 0x002373e0
     */
    virtual void SetShowing(int nShowing);

    /**
     * Act on a command.
     *
     * Slot 19. Previous and next step the ring and post the help text. Select flashes the
     * selected button. Every other command is ignored.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x00237268
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00237160
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * @param nSelector The pad index of the command, which the body does not read.
     * @ghidraAddress 0x00237168
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00237170
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00237178
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00237180
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Record the chosen button once it has finished flashing.
     *
     * Slot 30. Either play button requires a playlist with entries, records the play request and
     * the shuffle choice, and exits the top buttons, help, and title screens. The save button
     * records the save request and exits the top buttons screen.
     *
     * @param pObject The button that finished alternating, which the body does not read.
     * @ghidraAddress 0x002321f8
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Select the first button and forget the recorded choice.
     *
     * Slot 33.
     *
     * @ghidraAddress 0x002373a8
     */
    virtual void OnUnknownSlot33();

    /**
     * Act on the recorded choice once the screen has left.
     *
     * Slot 36. A save exits the title and help screens and saves the playlist through
     * MetRemixManager, returning to the top buttons, title, and help screens. A play clears the
     * active panel, starts the playlist, and records this screen in MetFrontEndState::mUnknown24.
     * The choice is then forgotten.
     *
     * @ghidraAddress 0x00231b50
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container views and add the three buttons.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x00231908
     */
    virtual void ResolveContainerViews();

private:
    // 0x00232598
    // Posts the help text for the selected button. Either play button selects the play text and
    // the save button the save text. Any other selection posts two empty strings.
    void UpdateHelpText();

    MetButtonList *mUnknown8c; // +0x8c
    int mUnknown90;            // +0x90, set when the save button was chosen
    int mUnknown94;            // +0x94, set when the playlist plays in random order
    int mUnknown98;            // +0x98, set when a play button was chosen
};
