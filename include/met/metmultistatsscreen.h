#pragma once

#include "met/metscreen.h"

/**
 * End-of-game statistics for a multiplayer session.
 *
 * `19MetMultiStatsScreen` in the RTTI descriptor at `0x008eedd8`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x008003c0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002ff228` takes only the renderer and the load priority, and supplies
 * `ems` for the screen name, `metagame/_Local` for the directory, and `end_multi_stats` for the
 * container. It writes the six vectors at `+0x94`, `+0xa0`, `+0xac`, `+0xb8`, `+0xc4`, and
 * `+0xd0`.
 *
 * The object is at least 0xdc bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x002fff90`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x00300268`, 36 `0x00306998`, 38 `0x002ff3f8`.
 */
class MetMultiStatsScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002ff228
     */
    MetMultiStatsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002fff90
     */
    virtual ~MetMultiStatsScreen();
};
