#pragma once

#include "memcard/memcarduser.h"
#include "met/metscreen.h"

class MetButtonList;

/**
 * End-of-game button row shown after a solo win.
 *
 * `16MetSoloWinScreen` in the RTTI descriptor at `0x008ef190`, with two public non-virtual bases at
 * fixed offsets, MetScreen at `+0x00`, and MemcardUser at `+140`.
 *
 * The 39-entry primary vtable is at `0x0080ea80`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The twenty-one-entry MemcardUser table at `0x0080e9d0` adjusts `this` by `-140` in every entry.
 * A diff against MemcardUser's own table at `0x007daf78` reads only the type function and the
 * destructor as different, so the class overrides no memory-card report at all. It derives from
 * the interface and implements none of it, which is faithful.
 *
 * The constructor at `0x003b55a8` takes only the renderer and the load priority, and supplies
 * `egwb` for the screen name, `metagame/_Solo` for the directory, and `end_win_butts` for the
 * container.
 *
 * The object is 0x98 bytes, which the allocation at `0x003b9bb4` fixes rather than the
 * constructor's highest store.
 *
 * The destructor is at `0x003b9ce8`.
 *
 * Nine entries of the primary table differ from the MetScreen table, which a diff of the two
 * settles rather than the title each routine carries. Apart from the type function they are 1
 * `0x003b9ce8` the destructor, 5 `0x003b5968`, 19 `0x003b57a0`, 21 `0x003b9b90`, 23 `0x003b9b80`,
 * 24 `0x003b9b88`, 30 `0x003b5f88`, 33 `0x003b9d80`, and 36 `0x003b6188`. All eight behaviour
 * slots are declared below.
 *
 * Slots 21, 23, and 24 are two-instruction empty bodies at distinct addresses. Each is a genuine
 * override rather than an inherited empty body, because MetScreen's own slot 21, 23, and 24 all
 * play a named sound. The class therefore silences the leave sound and both cycle sounds.
 *
 * One routine in the translation unit is not declared. `0x003b9b98` allocates 0x98 bytes under the
 * tag `MetSoloWinScreen`, runs the constructor, and returns the object, which is what a `new`
 * expression compiles to. It has exactly one caller, the routine at `0x00385180` that creates
 * every front-end screen, and whether the original wrote the expression at that call site or wrote
 * a factory on this class is not settled.
 *
 * OnUnknownSlot36() is the one body that is not written. It needs MetRenderer::ResolveArenaView()
 * and the two empty MetRenderer routines at `0x00390088` and `0x00390090`, which this tree does not
 * declare yet.
 */
class MetSoloWinScreen : public MetScreen, public MemcardUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003b55a8
     */
    MetSoloWinScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003b9ce8
     */
    virtual ~MetSoloWinScreen();

    /**
     * Build the three buttons, title the screen, and push the three companion panels. Slot 5.
     *
     * The prompt keys the buttons are labelled from are the same three strings the class pushes
     * into MetScreen::mUnknown38, so the prompt the help screen displays for a button is the key
     * its label was looked up under rather than the label itself.
     *
     * @ghidraAddress 0x003b5968
     */
    virtual void EnterAndShow();

    /**
     * Route one command to the button ring or to the selection. Slot 19.
     *
     * Commands 1 and 2 step the ring and repost the prompt. Command 5 clears the active panel,
     * starts the selection alternating, and clears the prompt. Every other command is discarded,
     * including command 6, so the screen cannot be departed with the back button.
     *
     * @param pCommand The command to route.
     * @ghidraAddress 0x003b57a0
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play no leave sound. Slot 21.
     *
     * @param nSelector The pad index of the command, which the body does not read.
     * @ghidraAddress 0x003b9b90
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * Play no left cycle sound. Slot 23.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x003b9b80
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play no right cycle sound. Slot 24.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x003b9b88
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Exit the four companion screens and start this screen's exit animation. Slot 30.
     *
     * MetScreen slot 29 runs the slot once the selection has finished alternating, which is what
     * pairs the button press with the departure. The Rnd::Object the slot receives is not read.
     *
     * @param pObject The object slot 29 finished with, which the body does not read.
     * @ghidraAddress 0x003b5f88
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Select the prompt layout with no back button and post the selected button's prompt. Slot 33.
     *
     * MetScreen slot 32 runs the slot once the enter animation has finished.
     *
     * @ghidraAddress 0x003b9d80
     */
    virtual void OnUnknownSlot33();

    /**
     * Commit the session result and go on to whichever screen the selection chose. Slot 36.
     *
     * The body is not written, for the reason the class documentation records. MetScreen slot 35
     * runs the slot once the exit animation has finished. Selection 0 continues to
     * `MetSoloStagesScreen` with the game manager's stage index advanced by one, selection 1
     * returns to `MetMainScreen`, and any other selection goes to `MetLoadGameScreen` after
     * recording this screen's own name in MetFrontEndState::mUnknown24. Every path then clears the
     * selection.
     *
     * @ghidraAddress 0x003b6188
     */
    virtual void OnUnknownSlot36();

    /**
     * Record on the registered solo win screen whether the finished stage unlocked a difficulty.
     *
     * The screen is resolved under the registry key `MetSoloWinScreen` and narrowed with
     * dynamic_cast. The result is written through without a null test. MetStageFinishScreen slot
     * 36 is the one caller.
     *
     * @param nUnlocked Non-zero when a difficulty was unlocked.
     * @ghidraAddress 0x003b9c20
     */
    static void SetDifficultyUnlocked(int nUnlocked);

private:
    // The three-button ring. The constructor allocates it and the destructor releases it. +0x90
    MetButtonList *mUnknown90;
    // Set while a difficulty unlocked by the finished stage still needs committing.
    // SetDifficultyUnlocked() writes it from MetStageFinishScreen slot 36, the constructor clears
    // it, and slot 36 clears it again after committing. +0x94
    int mDifficultyUnlocked;
};
