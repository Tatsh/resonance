#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"

/**
 * Row of buttons along the network options screen.
 *
 * `29MetConfigOptionsButtonsScreen` in the RTTI descriptor at `0x008eeda8`, with MetScreen as its
 * one public non-virtual base at offset 0. The object is 0x94 bytes and the 39-entry vtable is at
 * `0x007ea688`, the same length as the MetScreen table, so the class declares no virtual of its
 * own.
 *
 * The constructor at `0x002071f0` takes only the renderer and the load priority. It supplies the
 * three names itself as literals, `nob` for the screen, `metagame/shared` for the directory, and
 * `net_options_butts` for the container, so the screen loads
 * `metagame/shared/net_options_butts.rnd` and resolves `net_options_butts.view` along with the
 * `nob_EE.anim` and `nob_BF.anim` animations. It then allocates a MetButtonList of 0x18 bytes
 * tagged `MetButtonList` and zeroes mUnknown90.
 *
 * The destructor at `0x0020c088` releases the button list through slot 1 of its own vtable with a
 * deleting `__in_chrg`, runs the MetScreen destructor, and releases the object with the tag
 * `MsgSink`.
 *
 * Eight slots differ from the MetScreen table. Slots 23 and 24 are two-instruction `jr ra` stubs,
 * so this screen plays neither cycle sound. Of the rest only the destructor has a recovered name.
 *
 *  - 1 `0x0020c088` the destructor.
 *  - 5 `0x00207660` replaces MetScreen::EnterAndShow at `0x003900a8`.
 *  - 19 `0x002073c8` replaces an empty MetScreen slot.
 *  - 23 `0x0020bff0` PlayCycleLeftSound(), overridden empty.
 *  - 24 `0x0020bff8` PlayCycleRightSound(), overridden empty.
 *  - 30 `0x00207fc0` replaces an empty MetScreen slot. Copies the HxStr at `+0x04` of its
 *    argument and compares it against `nob_controller.but` and `nob_memory.but`, which fixes the
 *    argument as a pointer to a record whose name sits at `+0x04`.
 *  - 36 `0x00208270` replaces an empty MetScreen slot.
 */
class MetConfigOptionsButtonsScreen : public MetScreen {
public:
    /**
     * Construct the button row.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002071f0
     */
    MetConfigOptionsButtonsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0020c088
     */
    virtual ~MetConfigOptionsButtonsScreen();

    /**
     * @ghidraAddress 0x0020bff0
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x0020bff8
     */
    virtual void PlayCycleRightSound();

private:
    MetButtonList *mUnknown8c; // +0x8c
    int mUnknown90;            // +0x90
};
