#pragma once

#include "met/metscreen.h"

/**
 * Screen that picks the difficulty.
 *
 * `18MetGameSkillScreen` in the RTTI descriptor at `0x008ef8b0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007f38b0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x00273140` takes only the renderer and the load priority, and supplies
 * `smgs` for the screen name, `metagame/_Solo` for the directory, and `gameskill` for the
 * container. It writes only `+0x8c`, into which it allocates a MetButtonList.
 *
 * The object is at least 0x90 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x00276ad0`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x00273800`, 19 `0x00273558`, 23 `0x00276a38`, 24 `0x00276a40`, 30 `0x00273f28`, 36
 * `0x00274058`, 38 `0x00273310`.
 */
class MetGameSkillScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00273140
     */
    MetGameSkillScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00276ad0
     */
    virtual ~MetGameSkillScreen();

    /**
     * @ghidraAddress 0x00276a38
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x00276a40
     */
    virtual void PlayCycleRightSound();
};
