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
 * Screen that lists the remixes on a memory card for loading.
 *
 * `18MetRemixLoadScreen` in the RTTI descriptor at `0x00901c10`, with two public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00` and ListDataProvider at `+140`. The object is 0xac bytes,
 * which the factory at `0x00352590` confirms by requesting exactly that many with the tag
 * `MsgSink`.
 * The 39-entry primary vtable is at `0x00807300`, the same length as the MetScreen table, so the
 * class declares no virtual of its own, and the four-entry ListDataProvider table at `0x008072d8`
 * adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x00349dc0` takes only the renderer and the load priority, and supplies
 * `mcrl` for the screen name, `metagame/Shared` for the directory, and `memcard_remix_load` for
 * the container. It then clears MetScreen::mUnknown60, which is why that member is protected
 * rather than private, and allocates a MetButtonList tagged `MetButtonList` into mUnknowna8. The
 * words at `+0x90`, `+0x98`, and `+0x9c` are never written.
 *
 * The destructor at `0x00352618` restores both vptrs, deletes mUnknown94 and then clears it,
 * deletes mUnknowna8, restores the ListDataProvider vptr to `0x007ec830`, runs the MetScreen
 * destructor, and releases the object with the tag `MsgSink`. mUnknown94 is released through slot
 * 1 of a table at `+0x94` of the object itself, which is where ScrollingList places its vptr.
 *
 * Nine entries of the primary table differ from the MetScreen table, which a diff of the two
 * tables settles rather than the title each routine carries. They are 0 `0x00352508`, the
 * compiler-generated GetTypeInfo, 1 `0x00352618` the destructor, 5 `0x0034b568`, 19 `0x0034a2a8`,
 * 20 `0x003526d0`, 22 `0x00352720`, 33 `0x00352770`, 36 `0x0034cbd0`, and 38 `0x00349fc8`. All
 * seven behaviour slots are declared below.
 *
 * Both pure virtuals of the four-entry ListDataProvider table are supplied here, ProvideText() at
 * `0x0034d8b0` and ProvideMesh() at `0x00352588`.
 */
class MetRemixLoadScreen : public MetScreen, public ListDataProvider {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00349dc0
     */
    MetRemixLoadScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00352618
     */
    virtual ~MetRemixLoadScreen();

    /**
     * Show the screen and build the list of remixes. Slot 5.
     *
     * The body is not written. It runs for roughly 0x580 instructions, requesting the remix
     * catalogue from the memcard layer, building the ScrollingList at mUnknown94 over this
     * screen's own ListDataProvider subobject, and filling the two button entries. Most of the
     * routines it drives belong to the memcard and data-array layers and none of them is
     * identified.
     *
     * @ghidraAddress 0x0034b568
     */
    virtual void EnterAndShow();

    /**
     * Act on a navigation command. Slot 19.
     *
     * The body is not written and every part of it is recovered. A six-entry jump table at
     * `0x00807130` indexed by the command code less one selects the branch, and a code outside one
     * through six returns at once.
     *
     * Codes 1 and 2 walk the list. Each returns unless mUnknown90 is set, then reads the selected
     * row index through the ScrollingList accessor at `0x00401160`, and code 1 requires that index
     * to be above zero while code 2 requires it below one less than mUnknown90's element count.
     * The move itself runs through `0x00400ec8` for code 1 and `0x00400f78` for code 2, and both
     * branches end by reading the index again and handing it to the private data-screen helper.
     *
     * Codes 3 and 4 dispatch slots 2 and 3 of the MetButtonList at mUnknowna8, the two virtuals
     * that walk the button ring in opposite directions, and both end by running the private
     * helper at `0x0034a838`.
     *
     * Code 5 acts on the selection. It returns unless mUnknown90 is set and non-empty, reads the
     * selected row, and compares that row's unknown34_ against the cached value the routine at
     * `0x003f7ad8` vends. A row that does not match plays the error sound with the command's own
     * pad index and nothing else. A row that matches clears the shared ticker text through
     * `0x00317368`, records 2 in MetScreen::mUnknown18, exits `MetScreenTitleScreen`,
     * `MetHelpScreen`, and `MetRemixDataScreen` in that order, and then starts its own exit.
     *
     * Code 6 departs. It clears the ticker text the same way, clears MetScreen::mUnknown18, exits
     * `MetScreenTitleScreen` and `MetRemixDataScreen`, and then starts its own exit.
     *
     * What blocks the body is three declarations that do not exist. ScrollingList declares neither
     * its selected-row accessor nor either of its two move methods, MetButtonList declares neither
     * of the two ring virtuals, and the shared ticker routine at `0x00317368` has no title and no
     * declaration in the MetHelpScreen header it belongs to.
     *
     * @param pCommand The command the renderer translated from an input message.
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
     * Show the load options on the help screen once the enter animation has finished. Slot 33.
     *
     * The body is not written. It runs the private data-screen helper with a row index of zero,
     * posts `remix_load_opt` to the shared help screen through the routine at `0x00317298`, and
     * then posts the first element of MetScreen::mUnknown38 as ticker text at the renderer's
     * current time through `0x00317368`. The constructor of this class appends nothing to
     * mUnknown38, so that read takes the first element of an empty vector. Neither help-screen
     * routine has a title or a declaration.
     *
     * @ghidraAddress 0x00352770
     */
    virtual void OnUnknownSlot33();

    /**
     * Act on the button the user chose once the exit animation has finished. Slot 36.
     *
     * The body is not written. It hides both MetButtonList entries through `0x001fef40` and
     * `0x005349e0`, resolves the game manager, and then branches on MetScreen::mUnknown18 and on
     * the entry the button list reports, pushing `MetLeftGizmoScreen` among others. Neither button
     * routine is identified and MetScreen::mUnknown18 is read rather than written here.
     *
     * @ghidraAddress 0x0034cbd0
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the two buttons and the two row fonts. Slot 38.
     *
     * The body is not written and every part of it is recovered. It runs the MetScreen slot 38
     * body, resets the MetButtonList at mUnknowna8 through `0x001fedb0`, and adds two entries to it
     * through `0x001fcb28`, the first named `mcrl_SAVED.but` and the second `mcrl_FACTORY.but`.
     * Each entry's label comes from the data-array property lookup at `0x005096d0`, called with
     * the symbol 600 and the keys `rl_saved` and `rl_factory`. It then resolves `font1_pink_2` into
     * mUnknowna0 and `font1_pinkgrey_2` into mUnknowna4, each through Rnd::Manager::Find() followed
     * by a dynamic_cast to Rnd::Font, which is what types both members.
     *
     * What blocks the body is the two MetButtonList routines and the property lookup, none of which
     * is declared.
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
    // The row catalogue the ListDataProvider override at `0x0034d8b0` indexes. Never written by any
    // routine of this class, so it is filled from outside. +0x90
    std::vector<MetRemixRecord> *mUnknown90;
    // Deleted and then cleared by the destructor. +0x94
    ScrollingList *mUnknown94;
    int mUnknown98; // +0x98, not written by the constructor
    int mUnknown9c; // +0x9c, not written by the constructor
    // The font a row draws in when its record matches the cached value from `0x003f7ad8`, and the
    // font every other row draws in. Slot 38 resolves both.
    Rnd::Font *mUnknowna0;     // +0xa0
    Rnd::Font *mUnknowna4;     // +0xa4
    MetButtonList *mUnknowna8; // +0xa8
};
