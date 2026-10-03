#pragma once

#include "met/creditsroll.h"
#include "met/metscreen.h"

namespace Rnd {
class TransAnim;
class View;
} // namespace Rnd

/**
 * Credits roll.
 *
 * Its RTTI descriptor is at `0x008ef890`. It has MetScreen as its one public non-virtual base at
 * offset 0. New() allocates 0x9c bytes. The 39-entry primary vtable is at `0x007eb4d0`, the same
 * length as the MetScreen table, and the class declares no new virtual.
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
     * MetScreen::mShowsLoadedDrawables is cleared.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x00211970
     * @ghidraAddress PAL: 0x0021b348
     */
    MetCreditsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x00214e00
     * @ghidraAddress PAL: 0x0021e9e8
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
     * @ghidraAddress NTSC-U/C: 0x00214d78
     * @ghidraAddress PAL: 0x0021e960
     */
    static MetCreditsScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Rewind the roll, show the screen, and complete the entry at once.
     *
     * There is no enter animation. The fields MetScreen::UpdateEnterAnimation() sets on
     * completion are set directly, the renderer time is recorded as the animation start, and
     * slot 33 runs.
     *
     * @ghidraAddress NTSC-U/C: 0x00214e70
     * @ghidraAddress PAL: 0x0021ea58
     */
    virtual void EnterAndShow();

    /**
     * Begin the exit on the back command. Every other command is ignored.
     *
     * @param pCommand The command.
     * @ghidraAddress NTSC-U/C: 0x00214f68
     * @ghidraAddress PAL: 0x0021eb50
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing.
     *
     * @param nSelector Not read.
     * @ghidraAddress NTSC-U/C: 0x00214d58
     * @ghidraAddress PAL: 0x0021e940
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play nothing.
     *
     * @param nSelector Not read.
     * @ghidraAddress NTSC-U/C: 0x00214d60
     * @ghidraAddress PAL: 0x0021e948
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * Play nothing.
     *
     * @param nSelector Not read.
     * @ghidraAddress NTSC-U/C: 0x00214d68
     * @ghidraAddress PAL: 0x0021e950
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing.
     *
     * @param nSelector Not read.
     * @ghidraAddress NTSC-U/C: 0x00214d70
     * @ghidraAddress PAL: 0x0021e958
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Advance the animation and the roll.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress NTSC-U/C: 0x00214ee0
     * @ghidraAddress PAL: 0x0021eac8
     */
    virtual void UpdateIdle(float flTime);

    /**
     * Hide every credit and return to the options screens.
     *
     * @ghidraAddress NTSC-U/C: 0x00211e40
     * @ghidraAddress PAL: 0x0021b910
     */
    virtual void OnExitFinished();

    /**
     * Resolve the view, the animation, and the camera, and build the roll.
     *
     * An inline expansion of MetScreen::ResolveContainerViews() for the fixed view name
     * `credit.view`, without the null test, the diagnostic, or the call to hide the screen.
     *
     * @ghidraAddress NTSC-U/C: 0x00211ae8
     * @ghidraAddress PAL: 0x0021b520
     */
    virtual void ResolveContainerViews();

private:
#ifdef ENABLE_PATCHES
    // Open the roll with the reconstruction's credit and lengthen the scroll to match.
    void AddLeadingCredit();
#endif

    CreditsRoll *mCreditsRoll;  // +0x8c, the scroller slot 38 builds
    Rnd::TransAnim *mAnimation; // +0x90, `Group_credit.tnm`
    float mStartFrame;          // +0x94, the renderer time the screen entered at
    float mEndFrame;            // +0x98, the end frame of mAnimation
#ifdef ENABLE_PATCHES
    // mGroup is the roll's group, and mGroupStep is how far it moves per frame. Past
    // mAnimationEndFrame the group continues to move until mEndFrame. mEndFrame is later by the
    // frames the leading credit's distance takes.
    Rnd::View *mGroup = nullptr;
    float mAnimationEndFrame = 0.0f;
    float mGroupStep[3] = {};
#endif
};
