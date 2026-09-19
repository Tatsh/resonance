#pragma once

#include "met/metscreen.h"

/**
 * Tutorial screen.
 *
 * `17MetTutorialScreen` in the RTTI descriptor at `0x008eee38`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x00810578`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x003c7a88` takes only the renderer and the load priority, and supplies
 * `tut` for the screen name, `metagame/Shared` for the directory, and `tutorial` for the
 * container. It writes only `+0x8c`, into which it allocates a MetButtonList.
 *
 * It pushes the object names `tut_g` and `tut_r` into the container object-name vector that
 * MetScreen owns.
 *
 * The object is at least 0x90 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x003cc230`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003c82a0`, 19 `0x003c7f10`, 23 `0x003cc198`, 24 `0x003cc1a0`, 30 `0x003c84d8`, 36
 * `0x003c8678`, 38 `0x003c7d78`.
 */
class MetTutorialScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003c7a88
     */
    MetTutorialScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003cc230
     */
    virtual ~MetTutorialScreen();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003cc198
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003cc1a0
     */
    virtual void PlayCycleRightSound(int nSelector);
};
