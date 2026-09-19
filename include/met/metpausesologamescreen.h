#pragma once

#include "met/metpausebasescreen.h"

/**
 * Pause screen.
 *
 * `22MetPauseSoloGameScreen` in the RTTI descriptor at `0x008ef540`, with MetPauseBaseScreen as its
 * one public non-virtual base at offset 0. The 40-entry vtable at `0x00803918` is the same length
 * as the MetPauseBaseScreen table, so the class declares no virtual of its own.
 *
 * The constructor at `0x0031fd98` takes only the renderer and the load priority. It runs the
 * MetPauseBaseScreen constructor at `0x00317d40` with `psgr` for the screen name,
 * `metagame/Transition` for the directory, `pause_solo` for the container, and the screen
 * registry key `MetPauseSoloGameScreen`, which is its own class name verbatim. It also zeroes the
 * word at `+0xb0`, which two of the four pause screens write and the other two do not, so whether
 * it belongs to this class or to MetPauseBaseScreen is undetermined.
 *
 * The destructor is at `0x00323ae8`.
 *
 * Apart from the type function and the destructor, the slots that differ from the
 * MetPauseBaseScreen table are 5 `0x00320230`, 19 `0x00320088`, 36 `0x003205a8`, and 38
 * `0x0031ff38`.
 */
class MetPauseSoloGameScreen : public MetPauseBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0031fd98
     */
    MetPauseSoloGameScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00323ae8
     */
    virtual ~MetPauseSoloGameScreen();
};
