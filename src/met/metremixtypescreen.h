#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"

namespace Rnd {
class TransAnim;
class View;
} // namespace Rnd

/**
 * Screen that chooses what kind of remix to work on.
 *
 * Its RTTI descriptor is at `0x00901c20`. It has MetScreen as its one public non-virtual base at
 * offset 0. The factory at `0x00369638` fixes the object at 0xa0 bytes by requesting exactly that
 * many. The 39-entry vtable is at `0x00808760`, the same length as the MetScreen table, and the
 * class declares no new virtual.
 *
 * The constructor at `0x00361d18` takes only the renderer and the load priority, and supplies
 * `smrt` for the screen name, `metagame/Shared` for the directory, and `sm_remixtype` for the
 * container. It pushes three object names into the container object-name vector that MetScreen
 * owns, `smrt_new`, `smrt_load`, and `smrt_jukebox`, and then allocates a MetButtonList tagged
 * `MetButtonList` into the one member below.
 *
 * The destructor at `0x003696c0` restores the vptr, deletes mButtons through slot 1 of the
 * MetButtonList table with the deleting `__in_chrg` value, runs the MetScreen destructor, and
 * releases the object with the tag `MsgSink`.
 *
 * Nine slots differ from the MetScreen table. Slots 23 and 24 sit eight bytes apart at
 * `0x00369628` and `0x00369630` and are two-instruction `jr ra` stubs. The others are the
 * destructor, 5 `0x00362600`, 15 `0x00364478`, 19 `0x00362348`, 30 `0x00362fa0`, 36
 * `0x00363920`, and 38 `0x00362098`.
 */
class MetRemixTypeScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x00361d18
     * @ghidraAddress PAL: 0x0038f220
     */
    MetRemixTypeScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x003696c0
     * @ghidraAddress PAL: 0x00397740
     */
    virtual ~MetRemixTypeScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x00369638
     * @ghidraAddress PAL: 0x003976b8
     */
    static MetRemixTypeScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Show the screen with the buttons the game mode offers.
     *
     * Slot 5. A solo game offers the new, load, and jukebox buttons over the three-button view,
     * and any other mode offers the first two over the two-button view. A pending front-end
     * transition first brings up the gizmo and help screens and clears the loading flag in the
     * game parameters.
     *
     * @ghidraAddress NTSC-U/C: 0x00362600
     * @ghidraAddress PAL: 0x0038fcb0
     */
    virtual void EnterAndShow();

    /**
     * Act on the answer to the low-space warning.
     *
     * Slot 15. Only the `warn_remix_no_space` dialogue is handled. Its first button, `BACK`,
     * brings the help, gizmo, and remix-type screens back and makes this screen the active panel,
     * and any other choice proceeds with the selected button.
     *
     * @param name The dialogue name.
     * @param nChoice The index of the button chosen.
     * @ghidraAddress NTSC-U/C: 0x00364478
     * @ghidraAddress PAL: 0x003922e8
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Act on one navigation command.
     *
     * Slot 19. The two ring steps move the selection and refresh the help text, select starts the
     * selection alternation, and back exits toward the title screen.
     *
     * @param pCommand The command.
     * @ghidraAddress NTSC-U/C: 0x00362348
     * @ghidraAddress PAL: 0x0038f978
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Leave for the chosen button once the selection alternation finishes.
     *
     * Slot 30. The help screen is exited only when a button other than the first is selected. The
     * European release always exits it.
     *
     * @param pButton The button whose alternation finished, which is not read.
     * @ghidraAddress NTSC-U/C: 0x00362fa0
     * @ghidraAddress PAL: 0x00390848
     */
    virtual void OnRepeatingSoundFinished(Rnd::Button *pButton);

    /**
     * Move on once the screen has exited.
     *
     * Slot 36. After a back command the mode screen is brought up. The new and load buttons raise
     * the `warn_remix_no_space` dialogue when a memory card is in use in a solo game and the first
     * card slot's GlobalSettings::mCardSlots entry reports less than
     * GlobalSettings::mMinimumFreeClusters, and otherwise proceed with the selected button. The
     * jukebox button lists the remixes on the card and the disc with MetJukeboxTopButtonsScreen
     * and MetHelpScreen as the screens to return to, and also loads the playlist.
     *
     * The European release raises the dialogue only when GlobalSettings::mCardSlots is not empty,
     * and formats GlobalSettings::mMinimumFreeClusters into the dialogue text. The German text
     * also receives FirstCardSlotName() ahead of it. After a back command it also pushes the help
     * screen.
     *
     * @ghidraAddress NTSC-U/C: 0x00363920
     * @ghidraAddress PAL: 0x00391470
     */
    virtual void OnExitFinished();

    /**
     * Resolve the container views and the two button-layout views and animations.
     *
     * Slot 38.
     *
     * @ghidraAddress NTSC-U/C: 0x00362098
     * @ghidraAddress PAL: 0x0038f648
     */
    virtual void ResolveContainerViews();

    /**
     * Silence the cycle-left sound.
     *
     * Both overrides are two-instruction stubs, so each was written inline with an empty body.
     *
     * @ghidraAddress NTSC-U/C: 0x00369628
     * @ghidraAddress PAL: 0x003976a8
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress NTSC-U/C: 0x00369630
     * @ghidraAddress PAL: 0x003976b0
     */
    virtual void PlayCycleRightSound(int) {
    }

private:
    // NTSC-U/C: 0x00363148, PAL: 0x00390a48
    // Brings up the solo stages screen for the new button, or lists the remixes on
    // the card and the disc for the load button. Other selections do nothing. The European release
    // also pushes the help screen for the new button, and lists the first slot of port 0, as
    // recorded or by the name `1`, whether or not the front end uses the card.
    void OpenSelectedButton();

    MetButtonList *mButtons;          // +0x8c
    Rnd::View *mTwoButtonView;        // +0x90, `smrt_2but.view`
    Rnd::View *mThreeButtonView;      // +0x94, `smrt_3but.view`
    Rnd::TransAnim *mTwoButtonAnim;   // +0x98, `smrt_2but.tnm`
    Rnd::TransAnim *mThreeButtonAnim; // +0x9c, `smrt_3but.tnm`
};
