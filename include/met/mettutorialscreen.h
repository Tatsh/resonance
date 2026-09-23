#pragma once

#include "met/metscreen.h"

class MetButtonList;

namespace Rnd {
class Object;
} // namespace Rnd

/**
 * Tutorial screen.
 *
 * `17MetTutorialScreen` in the RTTI descriptor at `0x008eee38`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x00810578`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x003c7a88` takes only the renderer and the load priority, and supplies
 * `tut` for the screen name, `metagame/Shared` for the directory, and `tutorial` for the
 * container. It allocates the two-button ring and pushes the prompts `tut_g` and `tut_r` into
 * MetScreen::mUnknown38, one per button.
 *
 * The object is 0x90 bytes, which the allocation at `0x003cc1a8` fixes. That routine allocates
 * under the tag `MsgSink`, runs the constructor, and returns the object. Its one caller is the
 * routine at `0x00385180` that creates every front-end screen, and it is not declared.
 *
 * The destructor is at `0x003cc230`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003c82a0`, 19 `0x003c7f10`, 23 `0x003cc198`, 24 `0x003cc1a0`, 30 `0x003c84d8`, 36
 * `0x003c8678`, 38 `0x003c7d78`.
 *
 * Slots 5 and 36 are not written. Both read the object the unrecovered singleton getter at
 * `0x00217f30` vends, which this tree does not declare.
 */
class MetTutorialScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003c7a88
     */
    MetTutorialScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003cc230
     */
    virtual ~MetTutorialScreen();

    /**
     * Route one command to the button ring, the selection, or the exit. Slot 19.
     *
     * Commands 1 and 2 step the ring and post the selected button's prompt. Command 5 clears the
     * prompt and the active panel and starts the selection alternating. Command 6 clears the
     * prompt and the active panel, records 0 in MetScreen::mUnknown18, exits
     * `MetScreenTitleScreen` and `MetLeftGizmoScreen`, and starts this screen's exit animation.
     * Every other command is discarded.
     *
     * @param pCommand The command to route.
     * @ghidraAddress 0x003c7f10
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play no left cycle sound. Slot 23.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x003cc198
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play no right cycle sound. Slot 24.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x003cc1a0
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Record 2 in MetScreen::mUnknown18, exit the three companion screens, and start this
     * screen's exit animation. Slot 30.
     *
     * MetScreen slot 29 runs the slot once the selection has finished alternating. The companion
     * screens are `MetLeftGizmoScreen`, `MetScreenTitleScreen`, and `MetHelpScreen`.
     *
     * @param pObject The object slot 29 finished with, which the body does not read.
     * @ghidraAddress 0x003c84d8
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Resolve the base views and add the two buttons. Slot 38.
     *
     * The buttons are `tut_01.but` and `tut_02.but`, labelled from configuration code 0x258 under
     * `tut_g` and `tut_r`.
     *
     * @ghidraAddress 0x003c7d78
     */
    virtual void ResolveContainerViews();

private:
    // The two-button ring. The constructor allocates it and the destructor releases it. +0x8c
    MetButtonList *mUnknown8c;
};
