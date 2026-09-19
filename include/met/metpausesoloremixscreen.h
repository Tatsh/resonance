#pragma once

#include "met/metpausebasescreen.h"

/**
 * Pause screen.
 *
 * `23MetPauseSoloRemixScreen` in the RTTI descriptor at `0x008ef550`, with MetPauseBaseScreen as
 * its one public non-virtual base at offset 0. The 40-entry vtable at `0x00803fc0` is the same
 * length as the MetPauseBaseScreen table, so the class declares no virtual of its own.
 *
 * The constructor at `0x00323db8` takes only the renderer and the load priority. It runs the
 * MetPauseBaseScreen constructor at `0x00317d40` with `psr` for the screen name,
 * `metagame/Transition` for the directory, `pause_soloremix` for the container, and the screen
 * registry key `MetPauseSoloRemixScreen`, which is its own class name verbatim. It also zeroes the
 * word at `+0xb0`, which two of the four pause screens write and the other two do not, so whether
 * it belongs to this class or to MetPauseBaseScreen is undetermined.
 *
 * The destructor is at `0x00327ac0`.
 *
 * Apart from the type function and the destructor, the slots that differ from the
 * MetPauseBaseScreen table are 5 `0x00324260`, 19 `0x003240a8`, 36 `0x00324580`, and 38
 * `0x00323f58`.
 */
class MetPauseSoloRemixScreen : public MetPauseBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00323db8
     */
    MetPauseSoloRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00327ac0
     */
    virtual ~MetPauseSoloRemixScreen();
};
