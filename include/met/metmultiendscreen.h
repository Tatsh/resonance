#pragma once

#include "met/metscreen.h"

/**
 * End-of-game button row for a multiplayer session.
 *
 * `17MetMultiEndScreen` in the RTTI descriptor at `0x008ef170`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007ff4c8`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002f58d0` takes only the renderer and the load priority, and supplies
 * `egb` for the screen name, `metagame/_Solo` for the directory, and `end_game_butts` for the
 * container. It writes `+0x8c` and the five vectors at `+0x94`, `+0xa0`, `+0xac`, `+0xb8`, and
 * `+0xc4`.
 *
 * It loads the same container as MetSoloLoseScreen, `metagame/_Solo/end_game_butts`, under a
 * different screen name.
 *
 * The object is at least 0xd0 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x002f5d20`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x002f6158`, 19 `0x002f5f90`, 21 `0x002f9da8`, 23 `0x002f9d98`, 24 `0x002f9da0`, 30
 * `0x002f6640`, 36 `0x002f67d8`.
 */
class MetMultiEndScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002f58d0
     */
    MetMultiEndScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002f5d20
     */
    virtual ~MetMultiEndScreen();
};
