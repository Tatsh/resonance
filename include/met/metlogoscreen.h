#pragma once

#include "met/metscreen.h"

/**
 * Panel that draws the game logo.
 *
 * `13MetLogoScreen` in the RTTI descriptor at `0x008efe00`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007fa208`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002ba4a0` takes only the renderer and the load priority, and supplies `fl`
 * for the screen name, `metagame/Shared` for the directory, and `freq_logo_panel` for the
 * container. It writes `+0x90`, a vector at `+0x98`, then `+0xa8`, `+0xac`, and `+0xb0`.
 *
 * It and MetMsgScreen are the only two classes that override slot 3, MsgSink::HandleMessage, with
 * a body rather than inheriting the empty MetScreen override. It is also the only class that
 * overrides slot 27.
 *
 * The object is 0xc0 bytes, the size New() allocates. The constructor's highest store is at
 * `+0xb0`.
 *
 * The destructor is at `0x002be418`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 3 `0x002be6a0`, 5 `0x002be540`, 19 `0x002be4d8`, 20 `0x002be368`, 21 `0x002be370`, 22
 * `0x002be378`, 23 `0x002be380`, 24 `0x002be388`, 26 `0x002bac40`, 27 `0x002be5a8`, 33
 * `0x002bae40`, 36 `0x002baf20`, 38 `0x002ba6d0`.
 */
class MetLogoScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002ba4a0
     */
    MetLogoScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002be418
     */
    virtual ~MetLogoScreen();

    /**
     * Build the screen on the heap.
     *
     * MetScreen::CreateStartupScreens() registers this factory.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002be390
     */
    static MetLogoScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002be368
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * @param nSelector The pad index of the command.
     * @ghidraAddress 0x002be370
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002be378
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002be380
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002be388
     */
    virtual void PlayCycleRightSound(int nSelector);
};
