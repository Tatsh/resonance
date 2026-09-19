#pragma once

#include <vector>

#include "memcard/memcarduser.h"
#include "met/metbuttonlist.h"
#include "met/metscreenmultisoundbank.h"
#include "os/hxstr.h"
#include "rnd/object.h"

/**
 * Screen that assigns the controller buttons.
 *
 * `25MetConfigControllerScreen` in the RTTI descriptor at `0x008efbf0`, with two public
 * non-virtual bases at fixed offsets, MetScreenMultiSoundBank at `+0x00` and MemcardUser at
 * `+140`. The object is 0xcc bytes. Its primary 39-entry vtable is at `0x007e9cd8`, the same
 * length as the MetScreen table, so the class declares no virtual of its own, and the 21-entry
 * MemcardUser table at `0x007e9c28` adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x001ff1e0` takes only the renderer and the load priority, and supplies
 * `psx` for the screen name, `metagame/shared` for the directory, and `psx_config` for the
 * container. It writes the MetScreenMultiSoundBank vtable at `0x0080b560` before its own, empties
 * the four vectors below, sets mUnknownc4 to `a` and mUnknownc5 to `h`, allocates a MetButtonList
 * tagged `MetButtonList`, and pushes nine object names into the container object-name vector that
 * MetScreen owns. Those nine are `controller_config_left_note_1` and `_2`,
 * `controller_config_center_note_1` and `_2`, `controller_config_right_note_1` and `_2`,
 * `controller_config_erase_and_power`, `controller_config_expression`, and
 * `controller_config_remix_fx`. It then resolves a run of button meshes named
 * `psx_square_hi.mesh` through `psx_analogr_hi.mesh` into its own vectors.
 *
 * The destructor at `0x002008e8` releases the button list, then the four vectors in reverse
 * declaration order, restores the MemcardUser vptr to `0x007daf78`, runs the MetScreen destructor,
 * and releases the object with the tag `MsgSink`.
 *
 * Fourteen slots differ from the MetScreenMultiSoundBank table. All five multiplayer sounds are
 * overridden again, and only the destructor has a recovered name.
 *
 *  - 1 `0x002008e8` the destructor.
 *  - 5 `0x00201478` replaces MetScreen::EnterAndShow at `0x003900a8`.
 *  - 9 `0x002069a0` replaces MetScreen::BeginExit at `0x00390100`, and calls the base after a
 *    routine of its own at `0x00206a08`.
 *  - 15 `0x00206bb0` replaces an empty MetScreen slot. Compares its HxStr argument against
 *    `missingconfigvals` and, on a match, hands the literal `MetConfigControllerScreen` to
 *    MetScreen::ActivateNamedPanel.
 *  - 19 `0x00200ba8` replaces an empty MetScreen slot. Tests the word at `+0x04` of its argument
 *    against mUnknownc8 plus one and then jumps through a seven-entry table at `0x007e9ac0` on the
 *    word at `+0x00`.
 *  - 20 through 24 at `0x002068b0`, `0x00206970`, `0x00206940`, `0x002068e0`, and `0x00206910`,
 *    which replace the five MetScreenMultiSoundBank sounds.
 *  - 33 `0x002069d0` replaces an empty MetScreen slot. Passes the selected index of the button
 *    list to `0x00201eb8` and then runs `0x00202070` on this screen.
 *  - 36 `0x00201790` replaces an empty MetScreen slot.
 *  - 38 `0x001fff40` replaces MetScreen::ResolveContainerViews at `0x0038b1b0`.
 */
class MetConfigControllerScreen : public MetScreenMultiSoundBank, public MemcardUser {
public:
    /**
     * Construct the controller configuration screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x001ff1e0
     */
    MetConfigControllerScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002008e8
     */
    virtual ~MetConfigControllerScreen();

    /**
     * @ghidraAddress 0x002068b0
     */
    virtual void PlaySlideSound();

    /**
     * @ghidraAddress 0x00206970
     */
    virtual void PlayLeaveSound();

    /**
     * @ghidraAddress 0x00206940
     */
    virtual void PlayHighSound();

    /**
     * @ghidraAddress 0x002068e0
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x00206910
     */
    virtual void PlayCycleRightSound();

private:
    MetButtonList *mUnknown90;             // +0x90
    std::vector<Rnd::Object *> mUnknown94; // +0x94
    std::vector<HxStr> mUnknowna0;         // +0xa0
    std::vector<Rnd::Object *> mUnknownac; // +0xac
    std::vector<HxStr> mUnknownb8;         // +0xb8
    char mUnknownc4;                       // +0xc4, starts at `a`
    char mUnknownc5;                       // +0xc5, starts at `h`
    // Compared against the word at +0x04 of the argument of vtable slot 19.
    int mUnknownc8; // +0xc8
};
