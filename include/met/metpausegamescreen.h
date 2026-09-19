#pragma once

#include "met/metpausebasescreen.h"

/**
 * Pause screen.
 *
 * `18MetPauseGameScreen` in the RTTI descriptor at `0x00902310`, with MetPauseBaseScreen as its one
 * public non-virtual base at offset 0. The 40-entry vtable at `0x00803280` is the same length as
 * the MetPauseBaseScreen table, so the class declares no virtual of its own.
 *
 * The constructor at `0x0031c368` takes only the renderer and the load priority. It runs the
 * MetPauseBaseScreen constructor at `0x00317d40` with `pmgr` for the screen name,
 * `metagame/Transition` for the directory, `pause_multi` for the container, and the screen
 * registry key `MetPauseGameScreen`, which is its own class name verbatim.
 *
 * The destructor is at `0x0031fac8`.
 *
 * Apart from the type function and the destructor, the slots that differ from the
 * MetPauseBaseScreen table are 5 `0x0031c658` and 38 `0x0031c508`.
 */
class MetPauseGameScreen : public MetPauseBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0031c368
     */
    MetPauseGameScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0031fac8
     */
    virtual ~MetPauseGameScreen();
};
