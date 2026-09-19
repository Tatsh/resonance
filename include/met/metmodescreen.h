#pragma once

#include "met/metscreen.h"

/**
 * Screen that picks the game mode.
 *
 * `13MetModeScreen` in the RTTI descriptor at `0x008f00f0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007fe050`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002e72c0` takes only the renderer and the load priority, and supplies
 * `smm` for the screen name, `metagame/Shared` for the directory, and `sm_mode` for the container.
 * It writes only `+0x8c`, into which it allocates a MetButtonList.
 *
 * The object is at least 0x90 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x002eb8a0`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x002e79b8`, 19 `0x002e7628`, 23 `0x002eb808`, 24 `0x002eb810`, 30 `0x002eb958`, 33
 * `0x002eb920`, 36 `0x002e8008`, 38 `0x002e7490`.
 */
class MetModeScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002e72c0
     */
    MetModeScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002eb8a0
     */
    virtual ~MetModeScreen();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002eb808
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002eb810
     */
    virtual void PlayCycleRightSound(int nSelector);
};
