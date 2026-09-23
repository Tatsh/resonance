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
 * `18MetRemixTypeScreen` in the RTTI descriptor at `0x00901c20`, with MetScreen as its one public
 * non-virtual base at offset 0. The object is 0xa0 bytes, which the factory at `0x00369638` fixes
 * by requesting exactly that many, and the 39-entry vtable is at
 * `0x00808760`, the same length as the MetScreen table, so the class declares no virtual of its
 * own.
 *
 * The constructor at `0x00361d18` takes only the renderer and the load priority, and supplies
 * `smrt` for the screen name, `metagame/Shared` for the directory, and `sm_remixtype` for the
 * container. It pushes three object names into the container object-name vector that MetScreen
 * owns, `smrt_new`, `smrt_load`, and `smrt_jukebox`, and then allocates a MetButtonList tagged
 * `MetButtonList` into the one member below.
 *
 * The destructor at `0x003696c0` restores the vptr, deletes mUnknown8c through slot 1 of the
 * MetButtonList table with the deleting `__in_chrg` value, runs the MetScreen destructor, and
 * releases the object with the tag `MsgSink`.
 *
 * Nine slots differ from the MetScreen table. Slots 23 and 24 sit eight bytes apart at
 * `0x00369628` and `0x00369630` and are two-instruction `jr ra` stubs. Of the rest only the
 * destructor has a recovered name, and the others are 5 `0x00362600`, 15 `0x00364478`,
 * 19 `0x00362348`, 30 `0x00362fa0`, 36 `0x00363920`, and 38 `0x00362098`.
 */
class MetRemixTypeScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00361d18
     */
    MetRemixTypeScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003696c0
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
     * @ghidraAddress 0x00369638
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
     * @ghidraAddress 0x00362600
     */
    virtual void EnterAndShow();

    /**
     * Act on one navigation command.
     *
     * Slot 19. The two ring steps move the selection and refresh the help text, select starts the
     * selection alternation, and back exits toward the title screen.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x00362348
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Leave for the chosen button once the selection alternation finishes.
     *
     * Slot 30. The help screen is exited only when a button other than the first is selected.
     *
     * @param pObject The object whose alternation finished, which is not read.
     * @ghidraAddress 0x00362fa0
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Resolve the container views and the two button-layout views and animations.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x00362098
     */
    virtual void ResolveContainerViews();

    /**
     * Silence the cycle-left sound.
     *
     * Both overrides are two-instruction stubs, so each was written inline with an empty body.
     *
     * @ghidraAddress 0x00369628
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x00369630
     */
    virtual void PlayCycleRightSound(int) {
    }

private:
    MetButtonList *mUnknown8c;        // +0x8c
    Rnd::View *mTwoButtonView;        // +0x90, `smrt_2but.view`
    Rnd::View *mThreeButtonView;      // +0x94, `smrt_3but.view`
    Rnd::TransAnim *mTwoButtonAnim;   // +0x98, `smrt_2but.tnm`
    Rnd::TransAnim *mThreeButtonAnim; // +0x9c, `smrt_3but.tnm`
};
