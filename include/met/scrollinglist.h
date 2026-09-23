#pragma once

#include <list>
#include <vector>

#include "met/listdataprovider.h"
#include "rnd/transformable.h"

namespace Rnd {
class Drawable;
class Mesh;
class Object;
class Text;
class View;
} // namespace Rnd

/**
 * Scrolling list of rows driven by a ListDataProvider.
 *
 * `13ScrollingList` in the RTTI descriptor at `0x0086f6a0`, a leaf class with no base. The class
 * name is the RTTI spelling verbatim. No method name and no member name is attested anywhere in
 * the image, so every name below is inferred from the routine's behaviour.
 *
 * Following the g++ 2.x layout for a class with no base, the vptr sits after the data members at
 * `+0x94`, and the object is 0xa0 bytes, which the `operator new` argument at each call site
 * confirms. The two-entry vtable at `0x00815a20` runs the compiler-generated GetTypeInfo at
 * `0x00400e88` and the destructor at `0x003fd380`, so the destructor is the one virtual the class
 * declares.
 *
 * The list clones a template row view once per visible row, spacing the clones mRowPitch apart
 * along z, and names each clone `v_NN_` followed by the template's name. Each clone's Text and Mesh
 * children become its cells. refresh() hands every cell of a visible row to the provider, a
 * highlight mesh follows the cursor row, and two arrow meshes show whether items lie above or
 * below the visible page.
 *
 * MetFreqMakerDirectionsScreen, MetJukeboxBaseScreen, MetMCFreqDelScreen, MetRemixDelScreen, and
 * MetRemixLoadScreen all drive one.
 *
 * The rest of the translation unit is library code and is not declared. `0x004009b8` is the
 * vector insert that appends a Cell, `0x00400500` and `0x00400928` are std::list<Rnd::Object *>
 * sort and unique (shared with Rnd::Manager), `0x004003a0` is the merge that sort uses,
 * `0x00400060` and `0x00400c60` are unreferenced vector insert emissions, and `0x004013d0` is the
 * allocator's out-of-memory retry loop.
 */
class ScrollingList {
public:
    /**
     * Build the rows from the template view.
     *
     * The body runs with no memory zone selected and restores the caller's zone at the end. It
     * shows every child of the template, clones one row per visible row, hides the template, and
     * records the highlight mesh's local transform as the position of the first row. The item
     * count starts at zero.
     *
     * @param pProvider The provider the cells are filled from.
     * @param nRowPitch The distance between two rows along z.
     * @param nRowCount The number of visible rows.
     * @param pTemplate The row view each row is cloned from.
     * @param pHighlight The mesh that marks the cursor row, or null.
     * @param pUpArrow The mesh shown while items lie above the page, or null.
     * @param pDownArrow The mesh shown while items lie below the page, or null.
     * @param nContext The value passed through to the provider. It is the one argument on the
     * stack.
     * @ghidraAddress 0x003fcb00
     */
    ScrollingList(ListDataProvider *pProvider,
                  int nRowPitch,
                  int nRowCount,
                  Rnd::View *pTemplate,
                  Rnd::Mesh *pHighlight,
                  Rnd::Mesh *pUpArrow,
                  Rnd::Mesh *pDownArrow,
                  int nContext);

    /**
     * Delete every row clone together with every object it references.
     *
     * The highlight first returns to the first row. For each row the body resolves the clone by
     * name, gathers its animation, collision, draw, and transform descendants, removes duplicates,
     * and deletes each one and then the clone itself.
     *
     * @ghidraAddress 0x003fd380
     */
    virtual ~ScrollingList();

    /**
     * Show the rows that have an item and fill their cells, and hide the rest.
     *
     * The first visible item is the selection less the cursor row. Each cell goes to
     * ListDataProvider::ProvideMesh() when it is a Mesh and to ListDataProvider::ProvideText()
     * otherwise. The arrows are updated last.
     *
     * @ghidraAddress 0x003fdd18
     */
    void refresh();

    /**
     * Move the selection up by one item.
     *
     * An empty list hides the highlight and does nothing else. Otherwise the highlight takes the
     * low bit of mShowing, the selection and the cursor row each step up when they can, and the
     * highlight and the rows are refreshed.
     *
     * @ghidraAddress 0x00400ec8
     */
    void scrollUp();

    /**
     * Move the selection down by one item.
     *
     * The body mirrors scrollUp() with two differences. The highlight is not tested for null, and
     * it is shown unconditionally rather than with the low bit of mShowing.
     *
     * @ghidraAddress 0x00400f78
     */
    void scrollDown();

    /**
     * Record the number of items and clamp the selection to it.
     *
     * A count that is not positive hides the highlight and does nothing else. Otherwise the
     * highlight takes the low bit of mShowing, and the highlight is moved. The rows are not
     * refreshed.
     *
     * @param nItemCount The number of items.
     * @ghidraAddress 0x00401088
     */
    void setItemCount(int nItemCount);

    /**
     * Report the selected item.
     *
     * @return The index of the selected item.
     * @ghidraAddress 0x00401160
     */
    int getSelected();

    /**
     * Select one item and place the cursor row so the page ends at the last item where it can.
     *
     * An empty list resets the selection and the cursor row to zero instead. The highlight is
     * moved in both cases. The rows are not refreshed.
     *
     * @param nSelected The index of the item to select.
     * @ghidraAddress 0x00401168
     */
    void setSelected(int nSelected);

    /**
     * Record the showing flag and pass it to the highlight mesh.
     *
     * The highlight receives zero while the list is empty, and the low bit of nShowing otherwise.
     * A null highlight is skipped.
     *
     * @param nShowing Non-zero to draw the list.
     * @ghidraAddress 0x00401300
     */
    void setShowing(int nShowing);

    /**
     * Pass a showing flag to every Text cell.
     *
     * The flag is forwarded unchanged and, unlike setShowing(), neither masked nor tested against
     * the item count.
     *
     * @param nShowing Non-zero to draw each cell.
     * @ghidraAddress 0x00401360
     */
    void setEntriesShowing(int nShowing);

private:
    // One cell of a row. Exactly one of the two pointers is set.
    struct Cell {
        Rnd::Text *mText;
        Rnd::Mesh *mMesh;
    };

    // 0x003fdec0
    // Clone the template into the row view for one index.
    Rnd::View *makeRow(int nIndex);

    // 0x003fd858
    // Record the Text and Mesh children of one row view as its cells, and every Text
    // child in mTextCells as well.
    void buildRowCells(Rnd::View *pRow);

    // 0x00401030
    // Place the highlight mesh on the cursor row.
    void updateHighlight();

    // 0x00401270
    // Show each arrow when items lie beyond that end of the page.
    void updateArrows();

    ListDataProvider *mProvider;
    int mUnknown04; // +0x04, cleared by the constructor and read nowhere
    int mRowCount;
    int mItemCount;
    int mCursorRow;
    int mSelected;
    int mRowPitch;
    // The number of the template's children. Written by the constructor and read nowhere.
    int mTemplateDrawCount;
    Rnd::View *mTemplate;
    Rnd::Mesh *mHighlight;
    Rnd::Mesh *mUpArrow;
    Rnd::Mesh *mDownArrow;
    // The highlight mesh's local transform at construction, which places it on the first row.
    float mHighlightXfm[Rnd::kXfmRowCount][Rnd::kXfmRowFloatCount];
    std::list<std::vector<Cell>> mRowCells;
    std::vector<Rnd::View *> mRows;
    std::vector<Rnd::Drawable *> mTextCells;
    int mShowing;
    int mContext;
};
