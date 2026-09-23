#pragma once

#include <vector>

#include "met/listdataprovider.h"
#include "met/metscreen.h"

class MetButtonList;
class ScrollingList;
struct MetRemixRecord;

namespace Rnd {
class Font;
}

/**
 * Screen that lists the remixes on a memory card or the factory remixes for loading.
 *
 * `18MetRemixLoadScreen` in the RTTI descriptor at `0x00901c10`, with two public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00` and ListDataProvider at `+140`. The object is 0xac bytes,
 * which New() confirms by requesting exactly that many with the tag `MsgSink`.
 *
 * The 39-entry primary vtable is at `0x00807300`, the same length as the MetScreen table, so the
 * class declares no virtual of its own, and the four-entry ListDataProvider table at `0x008072d8`
 * adjusts `this` by `-140` in every entry.
 *
 * Two buttons choose the catalogue, `mcrl_SAVED.but` for the remixes on the first card slot and
 * `mcrl_FACTORY.but` for the factory remixes, and a ScrollingList shows the chosen catalogue. The
 * selected row is shown on MetRemixDataScreen.
 */
class MetRemixLoadScreen : public MetScreen, public ListDataProvider {
public:
    /**
     * Construct the screen.
     *
     * The screen name is `mcrl`, the directory `metagame/Shared`, and the container
     * `memcard_remix_load`. MetScreen::mUnknown60 is cleared, and a MetButtonList is allocated
     * into mUnknowna8. The words at `+0x90`, `+0x98`, and `+0x9c` are not written.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00349dc0
     */
    MetRemixLoadScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the list and the button list.
     *
     * @ghidraAddress 0x00352618
     */
    virtual ~MetRemixLoadScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00352590
     */
    static MetRemixLoadScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Choose the catalogue, set the title, build or show the list, then enter.
     *
     * Slot 5. The help line becomes `mem_load_remix` in jam mode and `mem_load_custom` otherwise,
     * and both buttons are shown. With a card present (MetFrontEndState::mUnknown0c) the card
     * catalogue is chosen, unless it is empty. The factory catalogue is chosen otherwise. The
     * title is the configuration code 0x269 text `mem_load_remix` or `mem_load_custom` formatted
     * with the first card slot's name for the card, and `fact_load_remix` or `fact_load_custom`
     * for the factory. The list is built on the first entry from `mcrl_line.view`,
     * `mcrl_hilite.mesh`, `mcrl_up.mesh`, and `mcrl_down.mesh`, and shown again on later entries.
     *
     * @ghidraAddress 0x0034b568
     */
    virtual void EnterAndShow();

    /**
     * Act on a navigation command.
     *
     * Slot 19, through a six-entry jump table at `0x00807130`. Previous and next move the list
     * within the catalogue and show the new row. Left and right walk the button ring and then run
     * OnButtonRingMoved(). A select on a row made on this disc empties the help text, records 2
     * in MetScreen::mUnknown18, exits the title, help, and data screens, and begins the exit, and
     * a select on any other row plays the error sound. A back empties the help text, clears
     * MetScreen::mUnknown18, exits the title and data screens, and begins the exit.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x0034a2a8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the slide sound only while the catalogue has rows. Slot 20.
     *
     * A null mUnknown90 and an empty catalogue both silence the sound. The element count is
     * computed by dividing the vector's byte span by 0x38, which is what settles the row type as
     * MetRemixRecord independently of the list provider.
     *
     * @param nSelector The controller index, which the base body ignores.
     * @ghidraAddress 0x003526d0
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play the emphasised-selection sound only while the catalogue has rows. Slot 22.
     *
     * The same guard as slot 20.
     *
     * @param nSelector The controller index, which the base body ignores.
     * @ghidraAddress 0x00352720
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * Show the first row on the data screen and the load options on the help screen.
     *
     * Slot 33. The help layout is `remix_load_opt`, and the help text is the first line of
     * MetScreen::mUnknown38 at the renderer time.
     *
     * @ghidraAddress 0x00352770
     */
    virtual void OnUnknownSlot33();

    /**
     * Act on the command once the exit has finished.
     *
     * Slot 36. Both buttons are hidden. After a back, jam mode returns through
     * `MetLeftGizmoScreen` to `MetRemixTypeScreen`, and any other mode to `MetSoloStagesScreen`.
     * After a select, the selected record becomes the loading game's level, script template 0x267
     * runs with `1`, and the record goes to MetRemixManager. With three or more personas the last
     * arena is taken and the load continues on `MetLoadGameScreen`. Otherwise
     * MetFrontEndState::mUnknown24 records this screen and the load continues on
     * `MetArenasScreen` with `MetHelpScreen`. MetRemixManager::BeginRemixLoad() then runs with
     * this screen, the title, data, and help screens to restore, and whether the factory
     * catalogue was chosen. The list entries are hidden last.
     *
     * @ghidraAddress 0x0034cbd0
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the two buttons and the two row fonts.
     *
     * Slot 38. The button list is emptied, and `mcrl_SAVED.but` and `mcrl_FACTORY.but` are added
     * with the configuration code 0x258 labels `rl_saved` and `rl_factory`. `font1_pink_2` and
     * `font1_pinkgrey_2` are resolved into mUnknowna0 and mUnknowna4.
     *
     * @ghidraAddress 0x00349fc8
     */
    virtual void ResolveContainerViews();

    /**
     * Show the second string of one row of mUnknown90, in mUnknowna0 when the row's unknown34_
     * matches GetAlbumJukeboxValue() and in mUnknowna4 otherwise.
     *
     * An index past the end of mUnknown90 empties the text instead. The column and the context are
     * not read.
     *
     * @param nItem The row of mUnknown90.
     * @param nColumn The cell index, which the body does not read.
     * @param pText The cell.
     * @param nContext The list context, which the body does not read.
     * @return Always 1.
     * @ghidraAddress 0x0034d8b0
     */
    virtual int ProvideText(int nItem, int nColumn, Rnd::Text *pText, int nContext);

    /**
     * Leave the cell as it is.
     *
     * @param nItem The row, which the body does not read.
     * @param nColumn The cell index, which the body does not read.
     * @param pMesh The cell, which the body does not read.
     * @param nContext The list context, which the body does not read.
     * @return Always 1.
     * @ghidraAddress 0x00352588
     */
    virtual int ProvideMesh(int nItem, int nColumn, Rnd::Mesh *pMesh, int nContext);

private:
    /**
     * Show one row of the catalogue on MetRemixDataScreen, or hide the data past the end.
     *
     * The data screen is taken from the registry without a cast check or a null test.
     *
     * @param nIndex The row.
     * @ghidraAddress 0x0034a748
     */
    void ShowRowOnDataScreen(unsigned nIndex);

    /**
     * Switch the catalogue to the button the ring now selects.
     *
     * The first button chooses the card catalogue and the second the factory catalogue, with the
     * same titles as EnterAndShow(), set through MetScreenTitleScreen::ReplaceTitle(). The list is
     * refilled, moved to the first row, and that row is shown.
     *
     * @ghidraAddress 0x0034a838
     */
    void OnButtonRingMoved();

    // The chosen catalogue, an entry of MetRemixManager::mRemixes. +0x90
    std::vector<MetRemixRecord> *mUnknown90;
    // Built by the first EnterAndShow(), and deleted and then cleared by the destructor. +0x94
    ScrollingList *mUnknown94;
    int mUnknown98; // +0x98, not written by any routine of this class
    int mUnknown9c; // +0x9c, not written by any routine of this class
    // The font a row draws in when its record was made on this disc, and the font every other row
    // draws in. Slot 38 resolves both.
    Rnd::Font *mUnknowna0;     // +0xa0
    Rnd::Font *mUnknowna4;     // +0xa4
    MetButtonList *mUnknowna8; // +0xa8
};
