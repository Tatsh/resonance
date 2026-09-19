#pragma once

#include "met/metpausebasescreen.h"

/**
 * Pause screen.
 *
 * `24MetPauseMultiRemixScreen` in the RTTI descriptor at `0x008ef480`, with MetPauseBaseScreen as
 * its one public non-virtual base at offset 0. The 40-entry vtable at `0x00804600` is the same
 * length as the MetPauseBaseScreen table, so the class declares no virtual of its own.
 *
 * The constructor at `0x00327d90` takes only the renderer and the load priority. It runs the
 * MetPauseBaseScreen constructor at `0x00317d40` with `pngr` for the screen name,
 * `metagame/Transition` for the directory, `pause_net` for the container, and the screen
 * registry key `MetPauseMultiRemixScreen`, which is its own class name verbatim.
 *
 * The destructor is at `0x0032b450`.
 *
 * Apart from the type function and the destructor, the slots that differ from the
 * MetPauseBaseScreen table are 5 `0x00328080`, 19 `0x0032b4a8`, and 38 `0x00327f30`.
 */
class MetPauseMultiRemixScreen : public MetPauseBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00327d90
     */
    MetPauseMultiRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0032b450
     */
    virtual ~MetPauseMultiRemixScreen();
};
