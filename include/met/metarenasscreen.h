#pragma once

#include "met/metscreen.h"

/**
 * Screen that picks an arena.
 *
 * `15MetArenasScreen` in the RTTI descriptor at `0x00901e80`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007e8ba0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x001f65a0` takes only the renderer and the load priority, and supplies `as`
 * for the screen name, `metagame/_Solo` for the directory, and `arena_sel` for the container. It
 * writes `+0x8c`, `+0x90`, and the four vectors at `+0xa4`, `+0xb0`, `+0xbc`, and `+0xc8`.
 *
 * It pushes the object name `arenas` into the container object-name vector that MetScreen owns,
 * and allocates a MetButtonList into `+0x8c`.
 *
 * The object is at least 0xd4 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x001f70f0`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x001f7750`, 19 `0x001f7310`, 20 `0x001fc718`, 23 `0x001fc680`, 24 `0x001fc688`, 26
 * `0x001f8af0`, 30 `0x001f8378`, 33 `0x001fc758`, 36 `0x001f8658`, 38 `0x001f6a08`.
 */
class MetArenasScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x001f65a0
     */
    MetArenasScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x001f70f0
     */
    virtual ~MetArenasScreen();
};
