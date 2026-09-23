#pragma once

#include <list>
#include <vector>

#include "met/metscreen.h"
#include "os/hxstr.h"

class FreqPart;
class MetFreqMakerCanvasScreen;
struct Color;
struct Vector2;
namespace Rnd {
class Mesh;
class Object;
class Text;
class View;
} // namespace Rnd

/**
 * Inventory of the FreQ maker, the part browser and colour palette beside the canvas.
 *
 * `27MetFreqMakerInventoryScreen` in the RTTI descriptor at `0x008ef8a0`, with MetScreen as its one
 * public non-virtual base at offset 0.
 *
 * The 44-entry primary vtable is at `0x007f3308`, five entries longer than the MetScreen table, so
 * the class declares five virtuals, at slots 39 through 43. The titles of those five are inferred
 * from the sounds they play.
 *
 * New() allocates 0x140 bytes. The constructor builds seven views, a main view and six pages hung
 * from it. Slot 38 lists the part templates on the pages as meshes in a grid of eight columns, and
 * the screen drives MetFreqMakerCanvasScreen through mCanvas.
 *
 * The translation unit spans `0x0026a3b0` to `0x00273140`. Besides the members below, it has the
 * static initialiser at `0x00272030` for four grid cells, and template library emissions. It also
 * has unreferenced out-of-line copies of inline routines. These are HxStr::operator+=(const char *)
 * at `0x002720b8`, Rnd::Mesh::MirrorX() at `0x00272328`, Rnd::Mesh::ScaleUniform() at
 * `0x002723a0`, and two file-local float sign helpers at `0x00272268` and `0x002722c8`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 7, 14, 19 through 24, 30, 33, 38, and 39 through 43.
 */
class MetFreqMakerInventoryScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * Supplies `fm_inventory` for the screen name, `metagame/persona` for the directory, and
     * `freq_maker_inventory` for the container. The seven views are created here rather than
     * resolved from the container, the six pages are hidden and hung from the main view, and the
     * palette cursor starts at column 8 and row 4.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0026a498
     */
    MetFreqMakerInventoryScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the seven views and every part mesh, and empty the five name lists.
     *
     * @ghidraAddress 0x0026c230
     */
    virtual ~MetFreqMakerInventoryScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00272440
     */
    static MetFreqMakerInventoryScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Show the screen with no editing mode active.
     *
     * Slot 5. MetScreen::mUnknown58 becomes 1.0f.
     *
     * @ghidraAddress 0x00272528
     */
    virtual void EnterAndShow();

    /**
     * Start browsing the inventory.
     *
     * Slot 7. The grid cursor returns to the first cell, the inventory panel is highlighted, and
     * the directions return to the select page. On the edit page the palette cursor follows the
     * part under the grid cursor, and on every other page the palette colour is applied and the
     * template under the grid cursor is previewed.
     *
     * @ghidraAddress 0x00272600
     */
    virtual void OnUnknownSlot7();

    /**
     * Report the container load finished once the FreQ maker assets and the directions screen's
     * container are both resident.
     *
     * Slot 14. The directions screen is polled whether or not the assets are resident.
     *
     * @return Non-zero once all three loads have finished.
     * @ghidraAddress 0x0026a3b0
     */
    virtual int PollContainerLoad();

    /**
     * Apply one command to the grid, the palette, or the part under edit, by editing mode.
     *
     * Slot 19. Commands 1 through 4 move the grid cursor while no part is placed. Select picks the
     * template or part under the grid cursor, or places the part under edit and returns to the
     * buttons screen. Back abandons the edit, or returns to the buttons screen. Command 7 switches
     * between the colour and the part modes. Commands 16 through 19 move the palette cursor in the
     * colour mode and the part in the part mode. Command 8 deletes, 12 recentres, 13 and 14
     * reorder, and 21 mirrors the part under edit. The command codes above 6 are named nowhere in
     * the image.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x0026c928
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the base slide sound while an editing mode is active.
     *
     * Slot 20.
     *
     * @param nSelector Passed through to MetScreen::PlaySlideSound().
     * @ghidraAddress 0x00272b78
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play the base leave sound while an editing mode is active.
     *
     * Slot 21.
     *
     * @param nSelector Passed through to MetScreen::PlayLeaveSound().
     * @ghidraAddress 0x00272bb8
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * Play `SND_MET_FM_PART_SELECT` while the inventory is browsed.
     *
     * Slot 22.
     *
     * @param nSelector The selector, which the body does not read.
     * @ghidraAddress 0x00272c38
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * Play `SND_MET_FM_PART_SELECT` while the inventory is browsed.
     *
     * Slot 23.
     *
     * @param nSelector The selector, which the body does not read.
     * @ghidraAddress 0x00272c68
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play `SND_MET_FM_PART_SELECT` while the inventory is browsed.
     *
     * Slot 24.
     *
     * @param nSelector The selector, which the body does not read.
     * @ghidraAddress 0x00272c98
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Make this screen the active panel again, through ActivateNamedPanel() with
     * `MetFreqMakerInventoryScreen`.
     *
     * Slot 30.
     *
     * @param pObject The object slot 29 finished with, which the body does not read.
     * @ghidraAddress 0x00272560
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Resolve the base views, list every part template on its page, and resolve the screen's
     * objects.
     *
     * Slot 38. Each template is cloned as `<name>.mesh` into mPartMeshes, given the template's
     * material and scale, and placed on the page of its category at the next cell of an
     * eight-column grid, and its name is appended to that page's name list. The main view is hung
     * from `fm_grid.view` and shown, mCanvas is resolved, the palette, the inventory decorations,
     * and both panel highlights are turned off, and the palette cursor moves to column 0 and row 0.
     *
     * @ghidraAddress 0x0026b518
     */
    virtual void ResolveContainerViews();

    /**
     * Hide the palette and the inventory, and return the palette cursor to its starting cell and
     * colour.
     *
     * Slot 33.
     *
     * @ghidraAddress 0x00272a98
     */
    virtual void OnUnknownSlot33();

    /**
     * Show the head page. MetFreqMakerButtonsScreen's routine at `0x0025a3e0` calls it. The title
     * is inferred.
     *
     * @ghidraAddress 0x0026d318
     */
    void ShowHeadPage();

    /**
     * Show the face page. MetFreqMakerButtonsScreen's routine at `0x0025a3e0` calls it. The title
     * is inferred.
     *
     * @ghidraAddress 0x0026d4c8
     */
    void ShowFacePage();

    /**
     * Show the body page. MetFreqMakerButtonsScreen's routine at `0x0025a3e0` calls it. The title
     * is inferred.
     *
     * @ghidraAddress 0x0026d678
     */
    void ShowBodyPage();

    /**
     * Show the details page. MetFreqMakerButtonsScreen's routine at `0x0025a3e0` calls it. The
     * title is inferred.
     *
     * @ghidraAddress 0x0026d828
     */
    void ShowDetailsPage();

    /**
     * Show the logos page. MetFreqMakerButtonsScreen's routine at `0x0025a3e0` calls it. The title
     * is inferred.
     *
     * @ghidraAddress 0x0026d9d8
     */
    void ShowLogosPage();

    /**
     * Hide every page, the palette, and the inventory decorations, and clear the heading.
     *
     * mCurrentView becomes null and mCurrentRowCount -1. MetFreqMakerButtonsScreen's routine at
     * `0x0025a3e0` and its slot 36 call it. The title is inferred.
     *
     * @ghidraAddress 0x0026db88
     */
    void HidePages();

    /**
     * Rebuild the edit page from the canvas's parts and show it.
     *
     * The previous edit meshes are deleted, and each part is copied into a new mesh named
     * `editableMesh<n>`, scaled and mirrored like the part, and placed at cell n. The heading
     * becomes `FreQ`. MetFreqMakerButtonsScreen's routine at `0x0025a3e0` and slot 19 call it. The
     * title is inferred.
     *
     * @ghidraAddress 0x0026ddc8
     */
    void ShowEditPage();

    /**
     * Play `SND_MET_FM_COLOR_MOVE` in the colour mode or `SND_MET_FM_PART_MOVE` in the part mode.
     *
     * Slot 39. The title is inferred.
     *
     * @ghidraAddress 0x00272cc8
     */
    virtual void PlayMoveSound();

    /**
     * Play `SND_MET_FM_FLIP_BUBBLE`.
     *
     * Slot 40. The title is inferred.
     *
     * @ghidraAddress 0x00272bf8
     */
    virtual void PlayFlipSound();

    /**
     * Play `SND_MET_FM_TOGGLE`. Slot 19 plays it after changing the editing mode.
     *
     * Slot 41. The title is inferred.
     *
     * @ghidraAddress 0x00272c18
     */
    virtual void PlayModeToggleSound();

    /**
     * Play `SND_MET_FM_TOGGLE`.
     *
     * Slot 42. The title is inferred.
     *
     * @ghidraAddress 0x00272b38
     */
    virtual void PlayToggleSound();

    /**
     * Play `SND_MET_FM_DELETE`.
     *
     * Slot 43. The title is inferred.
     *
     * @ghidraAddress 0x00272b58
     */
    virtual void PlayDeleteSound();

private:
    // The editing mode selects the sounds the slots play. The names are inferred.
    enum Mode { kModeNone = 0, kModeColor = 1, kModePart = 2, kModeInventory = 3 };

    // The panel SetHighlight() draws with its highlight material. The names are inferred.
    enum Highlight { kHighlightCanvas = 0, kHighlightInventory = 1, kHighlightNone = 2 };

    // 0x0026e9e0. Show or hide `fm_spectrum.view`, mCrossOrigin, and `COLOR.txt`.
    void ShowPalette(int nShowing);

    // 0x0026e690. Show or hide the inventory decorations and the main view. The wires and the
    // limit text are hidden first, and when shown, the edit page shows mLimitText and mWire16 and
    // every other page shows mWire30.
    void ShowInventory(int nShowing);

    // 0x0026ebb8. Choose the materials of `canvas_2.mesh` and `fm_inventory.mesh`. A value outside
    // Highlight clears both materials.
    void SetHighlight(int nHighlight);

    // Show one part page with its row count and heading. The five page routines expand it.
    void ShowPartPage(Rnd::View *pView, int nRowCount, const char *pszHeading);

    // Apply the palette colour on the canvas, move the cross to it, and on the edit page colour the
    // mesh under the grid cursor. Slot 19 expands it for each palette move.
    void ApplyPaletteToCurrentMesh();

    // 0x0026e448. Hide `fm_inventory_hisquare.mesh`.
    void HideGridCursor();

    // 0x0026e528. Move `fm_inventory_hisquare.mesh` to the grid cursor and show it.
    void UpdateGridCursor();

    // 0x0026eff0. Show one directions page, or the full page for the select page once the canvas
    // has its maximum of parts and the edit page is not shown.
    void ShowDirections(int nPage);

    // 0x0026f100. Report whether the grid cursor is on a template, or on the edit page on a part.
    bool IsCurrentCellFilled();

    // 0x0026f1b0. The name list of the page shown, or null when the grid cursor is past its end.
    std::vector<HxStr> *GetCurrentPageNames();

    // 0x002726e8. Apply the palette colour and preview the template under the grid cursor.
    void PreviewCurrentTemplate();

    // 0x00272868. The palette position at the centre of the palette cursor's cell.
    void GetPalettePosition(Vector2 &position);

    // 0x00272918. Move `fm_cross_origin.view` to one palette position.
    void MoveCrossOrigin(const Vector2 &position);

    // 0x00272800. Move `fm_cross_origin.view` to one palette position. The routine is never called.
    void MoveCrossOrigin(float flX, float flY);

    // 0x00272828. Move `fm_cross_origin.view` to the palette cursor.
    void UpdateCrossOrigin();

    // 0x002727d0. The colour at one palette position.
    Color *PaletteColorAt(const Vector2 &position);

    // 0x002724c8. Report the colour under the palette cursor and apply it on the canvas.
    void ApplyPaletteColor(Color &color);

    // 0x00272988. Put the palette cursor on the cell a part's colour came from, or on the centre
    // for a part with no palette position.
    void SetPaletteFromPart(FreqPart *pPart);

    // 0x00272a68. Select the part under the grid cursor on the canvas.
    FreqPart *SelectCurrentPart();

    // 0x00272790. Delete the part under the grid cursor from the canvas.
    void DeleteCurrentPart();

    // 0x002727c0. Empty. Slot 19 runs it when the grid cursor cannot move above row 0.
    void OnGridTopReached();

    // 0x002727c8. Empty. Slot 19 runs it when the grid cursor cannot move below the last row.
    void OnGridBottomReached();

    // 0x00272d20. The scale a part's template category is drawn at on the grid.
    float PartScale(FreqPart *pPart);

    Rnd::View *mMainInventoryView;        // +0x8c
    Rnd::View *mBodyView;                 // +0x90
    Rnd::View *mHeadView;                 // +0x94
    Rnd::View *mFaceView;                 // +0x98
    Rnd::View *mDetailsView;              // +0x9c
    Rnd::View *mLogosView;                // +0xa0
    Rnd::View *mEditView;                 // +0xa4
    Rnd::View *mCurrentView;              // +0xa8, the page shown
    std::list<Rnd::Object *> mPartMeshes; // +0xac, one clone for each part template
    int mBodyRowCount;                    // +0xb0
    int mFaceRowCount;                    // +0xb4
    int mHeadRowCount;                    // +0xb8
    int mDetailsRowCount;                 // +0xbc
    int mLogosRowCount;                   // +0xc0
    int mCurrentRowCount;                 // +0xc4, the bound slot 19 moves mGridRow within
    int mEditRowCount;                    // +0xc8
    Rnd::Mesh *mWire16;                   // +0xcc, `fm_wire_16.mesh`
    Rnd::Mesh *mWire30;                   // +0xd0, `fm_wire_30.mesh`
    Rnd::Text *mLimitText;                // +0xd4, `fminv_limit.txt`
    std::vector<Rnd::Mesh *> mEditMeshes; // +0xd8, the part meshes of the edit page
    int mPaletteColumn;                   // +0xe4, 0 through 15
    int mPaletteRow;                      // +0xe8, 0 through 7
    int mViewsResolved;                   // +0xec, set by slot 38
    MetFreqMakerCanvasScreen *mCanvas;    // +0xf0
    Rnd::View *mCrossOrigin;              // +0xf4, `fm_cross_origin.view`
    int mGridColumn;                      // +0xf8, 0 through 7
    int mGridRow;                         // +0xfc
    Mode mMode;                           // +0x100
    std::vector<HxStr> mFaceNames;        // +0x104
    std::vector<HxStr> mHeadNames;        // +0x110
    std::vector<HxStr> mBodyNames;        // +0x11c
    std::vector<HxStr> mDetailsNames;     // +0x128
    std::vector<HxStr> mLogosNames;       // +0x134
};
