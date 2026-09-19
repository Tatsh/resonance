#pragma once

#include "met/metscreen.h"

/**
 * Help and options screen.
 *
 * `13MetHelpScreen` in the RTTI descriptor at `0x009021e0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x00802420`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x003125b0` takes only the renderer and the load priority, and supplies `so`
 * for the screen name, `metagame/shared` for the directory, and `options_sl` for the container. It
 * writes the seven words from `+0x8c` to `+0xa0`, two vectors at `+0xa4` and `+0xc0`, and `+0xe0`
 * with `+0xe4`.
 *
 * It is one of only three classes that inherit slot 5 unchanged rather than overriding it.
 *
 * The object is at least 0xe8 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x00312770`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 19 `0x00317448`, 26 `0x00312df0`, 36 `0x00317480`, 38 `0x003128d0`.
 */
class MetHelpScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003125b0
     */
    MetHelpScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00312770
     */
    virtual ~MetHelpScreen();
};
