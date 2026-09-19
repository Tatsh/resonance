#pragma once

#include "met/metscreen.h"

/**
 * On-screen keyboard.
 *
 * `17MetKeyboardScreen` in the RTTI descriptor at `0x008f29f0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007f5390`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x00282660` takes only the renderer and the load priority, and supplies `kb`
 * for the screen name, `metagame/Shared` for the directory, and `keyboard` for the container. It
 * writes the run from `+0x8c` to `+0xb0` and the run from `+0xbc` to `+0xd4`.
 *
 * It is the only class in the subsystem that overrides slot 28, the repeating-sound starter.
 *
 * The object is at least 0xd8 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x00282e30`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x00283868`, 9 `0x0028c550`, 19 `0x00283268`, 20 `0x0028c518`, 22 `0x0028c4e0`, 23
 * `0x0028c470`, 24 `0x0028c4a8`, 26 `0x0028c708`, 28 `0x0028c5e0`, 30 `0x00283968`, 33
 * `0x0028c808`, 36 `0x00283aa0`, 38 `0x00282948`.
 */
class MetKeyboardScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00282660
     */
    MetKeyboardScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00282e30
     */
    virtual ~MetKeyboardScreen();
};
