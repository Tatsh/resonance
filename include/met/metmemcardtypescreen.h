#pragma once

#include "met/metscreen.h"

/**
 * Screen that picks what kind of saved data to load.
 *
 * `20MetMemCardTypeScreen` in the RTTI descriptor at `0x00901f30`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007fc488`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002d23b8` takes only the renderer and the load priority, and supplies
 * `mcrf` for the screen name, `metagame/Shared` for the directory, and `mcrf_load` for the
 * container. It writes `+0x8c` and `+0x90`.
 *
 * It pushes the object names `mcrf_remix` and `mcrf_freq` into the container object-name vector
 * that MetScreen owns.
 *
 * The object is at least 0x94 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x002d84d0`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x002d2b50`, 19 `0x002d2898`, 23 `0x002d8438`, 24 `0x002d8440`, 30 `0x002d2cf0`, 36
 * `0x002d2e90`, 38 `0x002d26b0`.
 */
class MetMemCardTypeScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002d23b8
     */
    MetMemCardTypeScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002d84d0
     */
    virtual ~MetMemCardTypeScreen();

    /**
     * @ghidraAddress 0x002d8438
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x002d8440
     */
    virtual void PlayCycleRightSound();
};
