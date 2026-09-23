#pragma once

namespace Rnd {
class Mesh;
class Text;
} // namespace Rnd

/**
 * Mix-in for an object that supplies the rows of a scrolling list to the screen that draws it.
 *
 * `16ListDataProvider` in the RTTI descriptor at `0x0086f760`, a leaf class with no base. The
 * class declares no data member, so the subobject is the four bytes of the compiler-generated vptr
 * at offset 0. MetMCFreqDelScreen proves the width directly, listing MemcardUser at `+140`,
 * ListDataProvider at `+144`, and MetMemCardPickerUser at `+148`.
 *
 * Five classes derive from the class. MetFreqMakerDirectionsScreen, MetJukeboxBaseScreen, and
 * MetRemixLoadScreen place the subobject at `+140`, MetMCFreqDelScreen at `+144`, and
 * MetRemixDelScreen at `+232`.
 *
 * The four-entry vtable at `0x007ec830` runs GetTypeInfo, the destructor, then two entries that
 * both store the `__pure_virtual` handler at `0x005381a8`, which is why ProvideText() and
 * ProvideMesh() are pure. ScrollingList is the one caller of both. It walks the cells of each
 * visible row and passes a Text cell to ProvideText() and a Mesh cell to ProvideMesh(), and
 * discards the result of both. The two method names are inferred from that use.
 *
 * The destructor at `0x002247f0` restores the vptr and releases the object through the scalar
 * release path at `0x004a9230` when its `__in_chrg` argument is odd, which is the whole of its
 * body.
 */
class ListDataProvider {
public:
    /**
     * @ghidraAddress 0x002247f0
     */
    virtual ~ListDataProvider();

    /**
     * Fill one Text cell of a visible row. Slot 2.
     *
     * @param nItem The index of the item the row shows.
     * @param nColumn The index of the cell within its row.
     * @param pText The cell.
     * @param nContext The value the ScrollingList was constructed with.
     * @return A status, which ScrollingList discards. Every override returns 1.
     */
    virtual int ProvideText(int nItem, int nColumn, Rnd::Text *pText, int nContext) = 0;

    /**
     * Fill one Mesh cell of a visible row. Slot 3.
     *
     * Every override in the image is empty and returns either 0 or 1.
     *
     * @param nItem The index of the item the row shows.
     * @param nColumn The index of the cell within its row.
     * @param pMesh The cell.
     * @param nContext The value the ScrollingList was constructed with.
     * @return A status, which ScrollingList discards.
     */
    virtual int ProvideMesh(int nItem, int nColumn, Rnd::Mesh *pMesh, int nContext) = 0;
};
