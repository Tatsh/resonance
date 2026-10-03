#pragma once

#include "met/metscreen.h"

namespace Rnd {
class View;
} // namespace Rnd

/**
 * Logo panel drawn along the top of the front end.
 *
 * Its RTTI descriptor is at `0x008f0880`. It has MetScreen as its one public non-virtual base at
 * offset 0.
 *
 * The 39-entry primary vtable is at `0x0080ff98`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x003c46f8` takes only the renderer and the load priority, and supplies `lp`
 * for the screen name, `metagame/Shared` for the directory, and `freq_logo_panel` for the
 * container. It clears MetScreen::mShowsLoadedDrawables and does not write mWaveView. The screen
 * loads the same container as MetLogoScreen under the screen name `lp` rather than `fl`, and it is
 * one of only three classes that inherit slot 5 unchanged.
 *
 * The object is 0x90 bytes, which the allocation in New() fixes.
 *
 * The destructor is at `0x003c7798`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 26 `0x003c77f0` and 38 `0x003c4868`.
 */
class MetTopLogoScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x003c46f8
     * @ghidraAddress PAL: 0x003fb828
     */
    MetTopLogoScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Build the screen on the heap.
     *
     * The object is allocated with the tag `MsgSink`. MetScreen::CreateMainMenuScreens() is the
     * one caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x003c7710
     * @ghidraAddress PAL: 0x003fe980
     */
    static MetTopLogoScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x003c7798
     * @ghidraAddress PAL: 0x003fea08
     */
    virtual ~MetTopLogoScreen();

    /**
     * Advance the wave animation to the current frame. Slot 26.
     *
     * @param flTime The current frame position.
     * @ghidraAddress NTSC-U/C: 0x003c77f0
     * @ghidraAddress PAL: 0x003fea60
     */
    virtual void UpdateIdle(float flTime);

    /**
     * Resolve the logo panel and the wave animation. Slot 38.
     *
     * The body does not run MetScreen::ResolveContainerViews(). It resolves the animation views,
     * resolves `logo_panel.view` into MetScreen::mView and releases its animation references
     * without a null test, clears MetScreen::mViewsUnresolved, and resolves `wave.view` into
     * mWaveView.
     *
     * @ghidraAddress NTSC-U/C: 0x003c4868
     * @ghidraAddress PAL: 0x003fba00
     */
    virtual void ResolveContainerViews();

private:
    // The wave animation slot 26 advances. Not written by the constructor. +0x8c
    Rnd::View *mWaveView;
};
