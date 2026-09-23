#pragma once

#include "met/creditsroll.h"
#include "met/metscreen.h"

namespace Rnd {
class TransAnim;
} // namespace Rnd

/**
 * Credits roll.
 *
 * `16MetCreditsScreen` in the RTTI descriptor at `0x008ef890`, with MetScreen as its one public
 * non-virtual base at offset 0. New() allocates 0x9c bytes. The 39-entry primary vtable is at
 * `0x007eb4d0`, the same length as the MetScreen table, so the class declares no virtual of its
 * own.
 *
 * The screen plays the `Group_credit.tnm` animation from the moment it enters and drives the
 * CreditsRoll along with it. The screen exits 100 frames after the animation's end, or when the
 * back command arrives.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x00214e70`, 19 `0x00214f68`, 20 `0x00214d58`, 22 `0x00214d60`, 23 `0x00214d68`, 24
 * `0x00214d70`, 26 `0x00214ee0`, 36 `0x00211e40`, and 38 `0x00211ae8`.
 */
class MetCreditsScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * The screen name is `cred`, the directory `metagame/Shared`, and the container `credit`.
     * MetScreen::mUnknown60 is cleared.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00211970
     */
    MetCreditsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00214e00
     */
    virtual ~MetCreditsScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00214d78
     */
    static MetCreditsScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Rewind the roll, show the screen, and complete the entry at once.
     *
     * There is no enter animation. The fields MetScreen::UpdateEnterAnimation() sets on
     * completion are set directly, the renderer time is recorded as the animation start, and
     * slot 33 runs.
     *
     * @ghidraAddress 0x00214e70
     */
    virtual void EnterAndShow();

    /**
     * Begin the exit on the back command. Every other command is ignored.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x00214f68
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x00214d58
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play nothing.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x00214d60
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * Play nothing.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x00214d68
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x00214d70
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Advance the animation and the roll.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x00214ee0
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Hide every credit and return to the options screens.
     *
     * @ghidraAddress 0x00211e40
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the view, the animation, and the camera, and build the roll.
     *
     * An inline expansion of MetScreen::ResolveContainerViews() for the fixed view name
     * `credit.view`, without the null test, the diagnostic, or the call to hide the screen.
     *
     * @ghidraAddress 0x00211ae8
     */
    virtual void ResolveContainerViews();

private:
    CreditsRoll *mUnknown8c;    // +0x8c, the scroller slot 38 builds
    Rnd::TransAnim *mUnknown90; // +0x90, `Group_credit.tnm`
    float mUnknown94;           // +0x94, the renderer time the screen entered at
    float mUnknown98;           // +0x98, the end frame of mUnknown90
};
