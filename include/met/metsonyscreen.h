#pragma once

#include "met/fadeuser.h"
#include "met/metscreen.h"

/**
 * Publisher presentation screen shown at start-up.
 *
 * `13MetSonyScreen` in the RTTI descriptor at `0x008ef560`, with two public non-virtual bases at
 * fixed offsets, MetScreen at `+0x00`, and FadeUser at `+140`.
 *
 * The 39-entry primary vtable is at `0x0080f1e8`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The four-entry FadeUser table at `0x0080f1c0` adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x003ba0f8` takes only the renderer and the load priority, and supplies
 * `sony` for the screen name, `metagame/Shared` for the directory, and `sony_pres` for the
 * container. It writes `+0x8c`, which is the FadeUser vptr, then `+0x90`, `+0x94`, `+0x98`, and
 * `+0x9c`.
 *
 * The object is 0xa0 bytes, the size New() allocates.
 *
 * The destructor is at `0x003bd7a8`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003bd868`, 20 `0x003bd6f8`, 21 `0x003bd700`, 22 `0x003bd708`, 23 `0x003bd710`, 24
 * `0x003bd718`, 26 `0x003ba2f0`, 36 `0x003ba4c8`, 38 `0x003bd828`.
 */
class MetSonyScreen : public MetScreen, public FadeUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003ba0f8
     */
    MetSonyScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003bd7a8
     */
    virtual ~MetSonyScreen();

    /**
     * Build the screen on the heap.
     *
     * MetScreen::CreateStartupScreens() registers this factory.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x003bd720
     */
    static MetSonyScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Respond to a MetFade fade out having finished.
     *
     * FadeUser slot 2, through the FadeUser table entry that adjusts `this` by `-140`.
     *
     * @ghidraAddress 0x003bd908
     */
    virtual void OnFadeOutDone();

    /**
     * Respond to a MetFade fade in having finished.
     *
     * FadeUser slot 3, through the FadeUser table entry that adjusts `this` by `-140`.
     *
     * @ghidraAddress 0x003bd8b8
     */
    virtual void OnFadeInDone();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003bd6f8
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * @param nSelector The pad index of the command.
     * @ghidraAddress 0x003bd700
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003bd708
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003bd710
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003bd718
     */
    virtual void PlayCycleRightSound(int nSelector);
};
