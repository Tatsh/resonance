#pragma once

#include "met/metscreen.h"

/**
 * Inventory of the FreQ maker, which owns seven named sub-views.
 *
 * `27MetFreqMakerInventoryScreen` in the RTTI descriptor at `0x008ef8a0`, with MetScreen as its one
 * public non-virtual base at offset 0.
 *
 * The 44-entry primary vtable is at `0x007f3308`, five entries longer than the MetScreen table, so
 * the class declares five virtuals of its own, at slots 39 through 43.
 *
 * The constructor at `0x0026a498` takes only the renderer and the load priority, and supplies
 * `fm_inventory` for the screen name, `metagame/persona` for the directory, and
 * `freq_maker_inventory` for the container. It writes seven view pointers from `+0x8c` to `+0xa4`,
 * `+0xa8`, the run from `+0xb0` to `+0xc8`, the run from `+0xcc` to `+0xd8`, a vector, and `+0xe4`
 * through `+0xfc`.
 *
 * The seven view pointers are resolved by name and the names are recovered: `MainInventoryView` at
 * `+0x8c`, `BodyView` at `+0x90`, `HeadView` at `+0x94`, `FaceView` at `+0x98`, `DetailsView` at
 * `+0x9c`, `LogosView` at `+0xa0`, and `EditView` at `+0xa4`. This is the only leaf that extends
 * the interface, with five virtuals of its own at slots 39 through 43, and none of the five has a
 * recovered name.
 *
 * The object is at least 0x100 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x0026c230`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x00272528`, 7 `0x00272600`, 14 `0x0026a3b0`, 19 `0x0026c928`, 20 `0x00272b78`, 21
 * `0x00272bb8`, 22 `0x00272c38`, 23 `0x00272c68`, 24 `0x00272c98`, 30 `0x00272560`, 33
 * `0x00272a98`, 38 `0x0026b518`, 39 `0x00272cc8`, 40 `0x00272bf8`, 41 `0x00272c18`, 42
 * `0x00272b38`, 43 `0x00272b58`.
 */
class MetFreqMakerInventoryScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0026a498
     */
    MetFreqMakerInventoryScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0026c230
     */
    virtual ~MetFreqMakerInventoryScreen();
};
