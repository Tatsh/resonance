#pragma once

#include "met/metscreen.h"

namespace Rnd {
class View;
} // namespace Rnd

/**
 * Logo panel drawn along the top of the front end.
 *
 * `16MetTopLogoScreen` in the RTTI descriptor at `0x008f0880`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080ff98`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x003c46f8` takes only the renderer and the load priority, and supplies `lp`
 * for the screen name, `metagame/Shared` for the directory, and `freq_logo_panel` for the
 * container. It clears MetScreen::mUnknown60 and does not write mUnknown8c. The screen loads the
 * same container as MetLogoScreen under the screen name `lp` rather than `fl`, and it is one of
 * only three classes that inherit slot 5 unchanged.
 *
 * The object is 0x90 bytes, which the allocation at `0x003c7710` fixes. That routine allocates
 * under the tag `MsgSink`, runs the constructor, and returns the object. Its one caller is the
 * routine at `0x00385180` that creates every front-end screen, and it is not declared.
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
     * @ghidraAddress 0x003c46f8
     */
    MetTopLogoScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003c7798
     */
    virtual ~MetTopLogoScreen();

    /**
     * Advance the wave animation to the current frame. Slot 26.
     *
     * @param flTime The current frame position.
     * @ghidraAddress 0x003c77f0
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Resolve the logo panel and the wave animation. Slot 38.
     *
     * The body does not run MetScreen::ResolveContainerViews(). It resolves the animation views,
     * resolves `logo_panel.view` into MetScreen::mUnknown14 and releases its animation references
     * without a null test, clears MetScreen::mUnknown48, and resolves `wave.view` into mUnknown8c.
     *
     * @ghidraAddress 0x003c4868
     */
    virtual void ResolveContainerViews();

private:
    // The wave animation slot 26 advances. Not written by the constructor. +0x8c
    Rnd::View *mUnknown8c;
};
