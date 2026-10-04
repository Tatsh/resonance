#pragma once

#include "met/listdataprovider.h"
#include "met/metscreen.h"

class ScrollingList;
namespace Rnd {
class View;
}

/**
 * Instructions panel of the FreQ maker.
 *
 * Its RTTI descriptor is at `0x00901e90`. It has two public non-virtual bases at fixed offsets,
 * MetScreen at `+0x00`, and ListDataProvider at `+140`.
 *
 * The 39-entry primary vtable is at `0x007f2500`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The four-entry ListDataProvider table at `0x007f24d8` adjusts `this` by `-140` in every entry.
 *
 * The object is 0x9c bytes, which the allocation in New() fixes.
 *
 * The translation unit spans `0x00262810` to `0x0026a498`. Besides the members below, it has the
 * static initialiser at `0x00265f50`, which builds the page tables and the mode names, per-unit
 * copies of MsgSink and ListDataProvider routines, and template library emissions.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 14, 23, 24, 30, 36, and 38.
 */
class MetFreqMakerDirectionsScreen : public MetScreen, public ListDataProvider {
public:
    /**
     * Construct the screen.
     *
     * Supplies `fm_directions` for the screen name, `metagame/persona` for the directory, and
     * `freq_maker_directions` for the container, clears MetScreen::mShowsLoadedDrawables, and
     * starts at the blank page.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x00262810
     * @ghidraAddress PAL: 0x00279790
     */
    MetFreqMakerDirectionsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Release the screen. The scrolling list is not deleted.
     *
     * @ghidraAddress NTSC-U/C: 0x00269e68
     * @ghidraAddress PAL: 0x00281770
     */
    virtual ~MetFreqMakerDirectionsScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x00269de0
     * @ghidraAddress PAL: 0x002816e8
     */
    static MetFreqMakerDirectionsScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Show the screen and its list entries.
     *
     * Slot 5.
     *
     * @ghidraAddress NTSC-U/C: 0x00269ed0
     * @ghidraAddress PAL: 0x002817d8
     */
    virtual void EnterAndShow();

    /**
     * Report the container load finished once the FreQ maker assets are resident as well.
     *
     * Slot 14.
     *
     * @return Non-zero once both loads have finished.
     * @ghidraAddress NTSC-U/C: 0x00269fc0
     * @ghidraAddress PAL: 0x002818e8
     */
    virtual int PollContainerLoad();

    /**
     * Play nothing.
     *
     * Slot 23. The body is empty.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress NTSC-U/C: 0x00269dd0
     * @ghidraAddress PAL: 0x002816d8
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing.
     *
     * Slot 24. The body is empty.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress NTSC-U/C: 0x00269dd8
     * @ghidraAddress PAL: 0x002816e0
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Make this screen the active panel again, through ActivateNamedPanel() with
     * `MetFreqMakerDirectionsScreen`.
     *
     * Slot 30.
     *
     * @param pButton The button slot 29 finished with, which the body does not read.
     * @ghidraAddress NTSC-U/C: 0x00269f00
     * @ghidraAddress PAL: 0x00281808
     */
    virtual void OnRepeatingSoundFinished(Rnd::Button *pButton);

    /**
     * Hide the list entries.
     *
     * Slot 36.
     *
     * @ghidraAddress NTSC-U/C: 0x00269fa0
     * @ghidraAddress PAL: 0x002818c8
     */
    virtual void OnExitFinished();

    /**
     * Resolve the base views and build the seven-row directions list.
     *
     * Slot 38. The rows are cloned from `fmd_help_line_prototype.view` with a pitch of 22 and no
     * highlight or arrows, and the list is given seven items. The European release first fills the
     * page texts with LoadPageTexts() and labels `HELP.txt` with the directions title.
     *
     * @ghidraAddress NTSC-U/C: 0x00262998
     * @ghidraAddress PAL: 0x00279978
     */
    virtual void ResolveContainerViews();

    /**
     * Show one directions page.
     *
     * The list is refreshed, the page's mode name is posted to the help screen, and the help
     * preset becomes `freq_maker_save_button` on the save page, `only_back_title` on the full
     * page, and `standard_title` otherwise. MetFreqMakerButtonsScreen's routine at `0x0025a5e0`
     * calls it. The title is inferred. The European release looks the mode text and the preset up
     * in the current language.
     *
     * @param nPage The page, 0 through 16.
     * @ghidraAddress NTSC-U/C: 0x00262ad8
     * @ghidraAddress PAL: 0x00279c40
     */
    void ShowPage(int nPage);

    /**
     * Show one cell of the directions page mPage selects.
     *
     * Each of pages 0 through 15 shows the string at row nItem and column nColumn of its table,
     * and page 16 shows an empty string. The tables are file-scope arrays of seven rows of two
     * HxStr each at `0x006a3a50` through `0x006a414f`, built by the unit's static initialiser. On
     * the save page, row 1 is a format that receives FirstCardSlotName(). A page outside the range
     * leaves the cell as it is.
     *
     * @param nItem The row.
     * @param nColumn The column.
     * @param pText The cell.
     * @param nContext The list context, which the body does not read.
     * @return Always 1.
     * @ghidraAddress NTSC-U/C: 0x00262c28
     * @ghidraAddress PAL: 0x00279e28
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
     * @ghidraAddress NTSC-U/C: 0x0026a020
     * @ghidraAddress PAL: 0x00281948
     */
    virtual int ProvideMesh(int nItem, int nColumn, Rnd::Mesh *pMesh, int nContext);

private:
#ifdef VIDEO_STANDARD_PAL
    /**
     * Fill the text column of every page from the current language.
     *
     * The title is inferred.
     *
     * @ghidraAddress PAL: 0x00278e00
     */
    void LoadPageTexts();
#endif

    /**
     * The cell at one row and column of a page table.
     *
     * The body does not read this object.
     *
     * @ghidraAddress NTSC-U/C: 0x0026a008
     * @ghidraAddress PAL: 0x00281930
     */
    const HxStr &PageCell(int nRow, int nColumn, const HxStr (*pTable)[2]);

    Rnd::View *mRowTemplate; // +0x90, resolved by slot 38
    ScrollingList *mList;    // +0x94, built by slot 38 and never deleted
    // The directions page ProvideText() shows. The constructor starts it at the blank page. +0x98
    int mPage;
};
