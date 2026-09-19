#pragma once

#include "met/metscreen.h"

/**
 * Title bar shown above another screen.
 *
 * `20MetScreenTitleScreen` in the RTTI descriptor at `0x008ef8d0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080bc40`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x00390e10` takes only the renderer and the load priority, and supplies
 * `fst` for the screen name, `metagame/shared` for the directory, and `screen_title` for the
 * container. It writes nothing beyond its own vptr.
 *
 * The class declares no data member.
 *
 * The object is at least 0x8c bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x00393fa0`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003940b8`, 9 `0x00394108`, 33 `0x00394100`, 36 `0x00394128`, 38 `0x00390f88`.
 */
class MetScreenTitleScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00390e10
     */
    MetScreenTitleScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00393fa0
     */
    virtual ~MetScreenTitleScreen();
};
