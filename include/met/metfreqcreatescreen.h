#pragma once

#include "met/metscreen.h"

/**
 * Screen that creates a new FreQ.
 *
 * `19MetFreqCreateScreen` in the RTTI descriptor at `0x00901aa0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007f7808`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x0029c130` takes only the renderer and the load priority, and supplies `cf`
 * for the screen name, `metagame/_Solo` for the directory, and `create_freq` for the container. It
 * writes `+0x90`, `+0x94`, and `+0xa0`.
 *
 * The object is at least 0xa4 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x002a0b08`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x0029ccb8`, 19 `0x0029c7a0`, 23 `0x002a0b88`, 24 `0x002a0bb8`, 30 `0x0029ce58`, 36
 * `0x0029cfa0`, 38 `0x0029c330`.
 */
class MetFreqCreateScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0029c130
     */
    MetFreqCreateScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002a0b08
     */
    virtual ~MetFreqCreateScreen();
};
