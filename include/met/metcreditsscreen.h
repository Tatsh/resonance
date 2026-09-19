#pragma once

#include "met/metscreen.h"

/**
 * Credits roll.
 *
 * `16MetCreditsScreen` in the RTTI descriptor at `0x008ef890`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007eb4d0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x00211970` takes only the renderer and the load priority, and supplies
 * `cred` for the screen name, `metagame/Shared` for the directory, and `credit` for the container.
 * It writes only `+0x8c`.
 *
 * The object is at least 0x90 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x00214e00`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x00214e70`, 19 `0x00214f68`, 20 `0x00214d58`, 22 `0x00214d60`, 23 `0x00214d68`, 24
 * `0x00214d70`, 26 `0x00214ee0`, 36 `0x00211e40`, 38 `0x00211ae8`.
 */
class MetCreditsScreen : public MetScreen {
public:
    /**
     * Construct the screen.
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
     * @ghidraAddress 0x00214d58
     */
    virtual void PlaySlideSound();

    /**
     * @ghidraAddress 0x00214d60
     */
    virtual void PlayHighSound();

    /**
     * @ghidraAddress 0x00214d68
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x00214d70
     */
    virtual void PlayCycleRightSound();
};
