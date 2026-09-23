#pragma once

#include "met/listdataprovider.h"
#include "met/metscreen.h"

/**
 * Instructions panel of the FreQ maker.
 *
 * `28MetFreqMakerDirectionsScreen` in the RTTI descriptor at `0x00901e90`, with two public
 * non-virtual bases at fixed offsets, MetScreen at `+0x00`, and ListDataProvider at `+140`.
 *
 * The 39-entry primary vtable is at `0x007f2500`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The four-entry ListDataProvider table at `0x007f24d8` adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x00262810` takes only the renderer and the load priority, and supplies
 * `fm_directions` for the screen name, `metagame/persona` for the directory, and
 * `freq_maker_directions` for the container. It writes `+0x8c`, which is the ListDataProvider
 * vptr, and `+0x98`.
 *
 * The object is at least 0x9c bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x00269e68`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x00269ed0`, 14 `0x00269fc0`, 23 `0x00269dd0`, 24 `0x00269dd8`, 30 `0x00269f00`, 36
 * `0x00269fa0`, 38 `0x00262998`.
 */
class MetFreqMakerDirectionsScreen : public MetScreen, public ListDataProvider {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00262810
     */
    MetFreqMakerDirectionsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00269e68
     */
    virtual ~MetFreqMakerDirectionsScreen();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00269dd0
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00269dd8
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Show one cell of the directions page `+0x98` selects.
     *
     * The body switches on the page, 0 through 16, and shows the string at row nItem and column
     * nColumn of that page's table. The tables are static arrays of two HxStr per row at
     * `0x006a3a50` through `0x006a40e0`, indexed through the helper at `0x0026a008`. On the page
     * whose table is at `0x006a4070`, row 1 is a format that receives FirstCardSlotName(). A page
     * outside the range leaves the cell as it is. The body is not written, because the tables and
     * the static initialiser that builds them are not recovered.
     *
     * @param nItem The row.
     * @param nColumn The column.
     * @param pText The cell.
     * @param nContext The list context, which the body does not read.
     * @return Always 1.
     * @ghidraAddress 0x00262c28
     */
    virtual int ProvideText(int nItem, int nColumn, Rnd::Text *pText, int nContext);

    /**
     * Leave the cell as it is.
     *
     * @param nItem The row, which the body does not read.
     * @param nColumn The cell index, which the body does not read.
     * @param pMesh The cell, which the body does not read.
     * @param nContext The list context, which the body does not read.
     * @return Always 0.
     * @ghidraAddress 0x0026a020
     */
    virtual int ProvideMesh(int nItem, int nColumn, Rnd::Mesh *pMesh, int nContext);
};
