#pragma once

#include "met/metscreen.h"

/**
 * End-of-game statistics for a solo session.
 *
 * `18MetSoloStatsScreen` in the RTTI descriptor at `0x008f0080`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080e218`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x003af2e0` takes only the renderer and the load priority, and supplies
 * `egs` for the screen name, `metagame/_Solo` for the directory, and `end_game_stats` for the
 * container. It writes nothing beyond its own vptr.
 *
 * The class declares no data member, and with three differing slots it is the most nearly
 * unmodified MetScreen child in the subsystem.
 *
 * The object is at least 0x8c bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x003b5218`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003b0378`, 38 `0x003af450`.
 */
class MetSoloStatsScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003af2e0
     */
    MetSoloStatsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003b5218
     */
    virtual ~MetSoloStatsScreen();
};
