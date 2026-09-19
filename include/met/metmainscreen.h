#pragma once

#include "met/metscreen.h"

/**
 * Main menu.
 *
 * `13MetMainScreen` in the RTTI descriptor at `0x008f00e0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007fb4f8`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002c60d0` takes only the renderer and the load priority, and supplies `fm`
 * for the screen name, `metagame/_Solo` for the directory, and `2_main` for the container. It
 * writes only `+0x8c`, into which it allocates a MetButtonList.
 *
 * The object is at least 0x90 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x002cb570`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x002c6dc0`, 19 `0x002c6820`, 23 `0x002cb4d8`, 24 `0x002cb4e0`, 30 `0x002c6c20`, 33
 * `0x002c72a0`, 36 `0x002c73e0`, 38 `0x002c62a0`.
 */
class MetMainScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002c60d0
     */
    MetMainScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002cb570
     */
    virtual ~MetMainScreen();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002cb4d8
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002cb4e0
     */
    virtual void PlayCycleRightSound(int nSelector);
};
