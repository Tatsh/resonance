#pragma once

#include <vector>

#include "game/jukeboxplaylist.h"
#include "met/listdataprovider.h"
#include "met/metremixrecord.h"
#include "met/metscreen.h"
#include "met/scrollinglist.h"
#include "met/texturepairrecord.h"
#include "rnd/drawable.h"
#include "rnd/object.h"

namespace Rnd {
class Font;
} // namespace Rnd

/**
 * Base of the three jukebox screens.
 *
 * `20MetJukeboxBaseScreen` in the RTTI descriptor at `0x00902330`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00` and ListDataProvider at `+140`. The class therefore
 * starts its own members at `+144`, and the object is 0x150 bytes, which the constructor fixes by
 * writing `+0x14c` as its last member.
 *
 * The constructor is at `0x0021dcc0`. It takes the renderer, the load priority, and the three
 * names, and all three children run it rather than inlining it.
 *
 * Three classes derive from the class, MetJukeboxCustomRemixesScreen,
 * MetJukeboxEditPlaylistScreen, and MetJukeboxFactoryRemixesScreen. None of the three declares a
 * data member. Each writes mUnknownc8 in its own constructor, which is what promotes that member
 * to protected.
 *
 * Two vtables belong to the class. The primary table at `0x007ec6d0` has 43 entries, four more
 * than the MetScreen table, so the class declares four virtuals of its own at slots 39 through 42.
 * Slot 39 stores the `__pure_virtual` handler at `0x005381a8`, which makes the original class
 * abstract, and the three children supply it. The secondary table at `0x007ec6a8` has four
 * entries, is addressed by the ListDataProvider vptr, adjusts `this` by `-140` in every entry, and
 * supplies the two ListDataProvider pure virtuals at `0x0021ef30` and `0x00224ae8`. The
 * ListDataProvider table itself is at `0x007ec830`, also with four entries, and its slots 2 and 3
 * are the two pure ones.
 *
 * The constructor resolves no object name. It builds the two TexturePairRecord members from
 * `gSongLogo1.tex` with `gSongLogo2.tex` and `gSongLabel1.tex` with `gSongLabel2.tex`, at
 * `0x007ec520` through `0x007ec550`. The four object names
 * `met_jukebox_base_screen_help_tab`, `met_jukebox_base_screen_ticker_tape`,
 * `met_jukebox_base_screen_error_tab`, and `met_jukebox_base_screen_error_ticker_tape`, at
 * `0x007ec590` through `0x007ec608`, are resolved by slot 41 instead.
 *
 * Fifteen slots of the primary table differ from the MetScreen table. Slots 20 through 24 are
 * two-instruction `jr ra` stubs, so a jukebox screen plays none of those five sounds. Slot 25 is
 * inherited unchanged, so the error sound still plays. None of the nine remaining overrides beyond
 * the destructor and SetShowing() has a recovered name.
 *
 *  - 1 `0x0021dfc0` the destructor.
 *  - 5 `0x0021e908` replaces the show-and-animate routine at `0x003900a8`.
 *  - 7 `0x00224a90` replaces an empty MetScreen slot.
 *  - 17 `0x00224af0` SetShowing().
 *  - 19 `0x0021e268` replaces an empty MetScreen slot.
 *  - 20 `0x00224968` PlaySlideSound(), overridden empty.
 *  - 21 `0x00224970` PlayLeaveSound(), overridden empty.
 *  - 22 `0x00224978` PlayHighSound(), overridden empty.
 *  - 23 `0x00224980` PlayCycleLeftSound(), overridden empty.
 *  - 24 `0x00224988` PlayCycleRightSound(), overridden empty.
 *  - 26 `0x0021fc88` replaces an empty MetScreen slot.
 *  - 33 `0x00224990` replaces an empty MetScreen slot.
 *  - 36 `0x002249e0` replaces an empty MetScreen slot.
 *  - 38 `0x0021e0f0` replaces the view-resolving routine at `0x0038b1b0`, and resolves the two
 *    fonts `font1_pink_2` and `font1_pinkgrey_2` into mUnknownd8 and mUnknowndc.
 *  - 39 `0x005381a8` the pure virtual the three children supply.
 *  - 40 `0x0021f3e8`.
 *  - 41 `0x0021fe98`.
 *  - 42 `0x0021e3f8`.
 *
 * Every member of the class is protected. The slot 33, 38, 39, 40, and 41 overrides of the three
 * children read or write most of the block directly, including the two scrolling lists, the five
 * drawables, the two row-count pointers, and the three words at `+0x140`. A few members appear
 * only in this class's own code, and they are not separated out, because the register attribution
 * in the larger child overrides is not firm enough to assert that a given member is untouched
 * there.
 */
class MetJukeboxBaseScreen : public MetScreen, public ListDataProvider {
public:
    /**
     * Construct the screen with no list and no resolved object.
     *
     * The five drawables from mUnknowna4 through mUnknownb4 start null and slot 5 runs
     * Rnd::Drawable::SetShowing() on each without a null check, so a screen whose slot 38 has not
     * run yet faults there.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @param name The screen name.
     * @param directory The directory the container loads from.
     * @param file The container name, without the `.rnd` suffix.
     * @ghidraAddress 0x0021dcc0
     */
    MetJukeboxBaseScreen(MetRenderer *pRenderer,
                         int nPriority,
                         const HxStr &name,
                         const HxStr &directory,
                         const HxStr &file);

    /**
     * Release the two scrolling lists.
     *
     * @ghidraAddress 0x0021dfc0
     */
    virtual ~MetJukeboxBaseScreen();

    /**
     * Show or hide the view, both scrolling lists, and the list frame.
     *
     * Slot 17. The base implementation runs first and the rest of the body is skipped while the
     * container load has not finished, which MetScreen::mUnknown48 records. This read is the
     * evidence that makes that member protected. Hiding the screen passes zero to mUnknown9c
     * without a null check, so a screen hidden before slot 38 has run faults.
     *
     * @param nShowing Non-zero to draw the screen.
     * @ghidraAddress 0x00224af0
     */
    virtual void SetShowing(int nShowing);

    /**
     * Overridden empty, so a jukebox screen plays no slide sound.
     *
     * @param nSelector Declared by the slot and ignored.
     * @ghidraAddress 0x00224968
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Overridden empty, so a jukebox screen plays no departure sound.
     *
     * @ghidraAddress 0x00224970
     */
    virtual void PlayLeaveSound();

    /**
     * Overridden empty, so a jukebox screen plays no emphasis sound.
     *
     * @param nSelector Declared by the slot and ignored.
     * @ghidraAddress 0x00224978
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * Overridden empty, so a jukebox screen plays no left-cycle sound.
     *
     * @param nSelector Declared by the slot and ignored.
     * @ghidraAddress 0x00224980
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Overridden empty, so a jukebox screen plays no right-cycle sound.
     *
     * @param nSelector Declared by the slot and ignored.
     * @ghidraAddress 0x00224988
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Report the number of rows the list displays.
     *
     * Slot 39, pure here. Each of the three children divides the byte span of one vector by its
     * element size and returns the quotient, which is what fixes both the return type and the
     * empty argument list. The name is inferred from that arithmetic and from the
     * ListDataProvider role of the class.
     *
     * @return The row count.
     * @ghidraAddress 0x005381a8
     */
    virtual int GetItemCount() = 0;

    /**
     * Unrecovered. Slot 40.
     *
     * SetShowing() runs it with no argument whenever the screen becomes visible. The body
     * constructs six strings, resolves objects through Rnd::Manager, and writes mUnknown14c.
     *
     * @ghidraAddress 0x0021f3e8
     */
    virtual void OnUnknownSlot40();

    /**
     * Unrecovered. Slot 41.
     *
     * The body reads the count of the vector mUnknownc4 addresses, compares it against the global
     * at `0x0069ad78`, and resolves the four `met_jukebox_base_screen_` object names. It reads no
     * argument.
     *
     * @ghidraAddress 0x0021fe98
     */
    virtual void OnUnknownSlot41();

    /**
     * Unrecovered. Slot 42.
     *
     * SetShowing() runs it with no argument before anything else it does. The body resolves the
     * shared MetRemixManager, writes mUnknowna0 and mUnknownc4 from it, and pushes a row count
     * into each of the two scrolling lists through the routine at `0x00401088`.
     *
     * @ghidraAddress 0x0021e3f8
     */
    virtual void OnUnknownSlot42();

    /**
     * Show one row of whichever list the context selects.
     *
     * Context 0 is the remix catalogue at mUnknowna0. Its row shows the record's second string, in
     * mUnknownd8 when the record's unknown34_ matches GetAlbumJukeboxValue() and in mUnknowndc
     * otherwise. Context 1 is the playlist at mUnknownc4. The body copies the whole entry vector
     * first and shows the indexed name from the copy. An index past the end empties the text in
     * both contexts, and any other context leaves the cell as it is. The column is not read.
     *
     * @param nItem The row.
     * @param nColumn The cell index, which the body does not read.
     * @param pText The cell.
     * @param nContext 0 for the catalogue and 1 for the playlist.
     * @return Always 1.
     * @ghidraAddress 0x0021ef30
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
     * @ghidraAddress 0x00224ae8
     */
    virtual int ProvideMesh(int nItem, int nColumn, Rnd::Mesh *pMesh, int nContext);

protected:
    // The row pitch and the visible row count the child slot 38 hands to the ScrollingList
    // constructor.
    int mUnknown90; // +0x90, starts at 16
    int mUnknown94; // +0x94, starts at 10
    // The two lists the child slot 38 allocates, the first over the 0x38-byte records mUnknowna0
    // addresses and the second over the four-byte entries mUnknownc4 addresses.
    ScrollingList *mUnknown98; // +0x98
    ScrollingList *mUnknown9c; // +0x9c
    // The catalogue slots 39 and 42 count rows from. Slot 42 sets it from the shared
    // MetRemixManager and the base constructor starts it null.
    std::vector<MetRemixRecord> *mUnknowna0; // +0xa0
    Rnd::Drawable *mUnknowna4;               // +0xa4
    Rnd::Drawable *mUnknowna8;               // +0xa8
    Rnd::Drawable *mUnknownac;               // +0xac
    Rnd::Drawable *mUnknownb0;               // +0xb0
    Rnd::Drawable *mUnknownb4;               // +0xb4
    Rnd::Drawable *mUnknownb8;               // +0xb8, the frame SetShowing() hides with the screen
    int mUnknownbc;                          // +0xbc, written by the child slot 38
    int mUnknownc0;                          // +0xc0, written by the child slot 38
    // The playlist the edit screen counts rows from. Slot 42 sets it to the instance embedded at
    // `+0xc4` of the shared MetRemixManager.
    JukeboxPlayList *mUnknownc4; // +0xc4
    // Distinguishes the three jukebox lists. Zero for the saved and edit screens and -1 for the
    // factory screen. Each of the three children writes it in its own constructor.
    int mUnknownc8;                        // +0xc8
    std::vector<Rnd::Object *> mUnknowncc; // +0xcc
    // The two fonts slot 38 resolves, `font1_pink_2` and `font1_pinkgrey_2`. ProvideText() hands
    // them to Rnd::Text::SetFont(), which is what types both.
    Rnd::Font *mUnknownd8;         // +0xd8
    Rnd::Font *mUnknowndc;         // +0xdc
    TexturePairRecord mUnknowne0;  // +0xe0, the song logo pair
    TexturePairRecord mUnknown110; // +0x110, the song label pair
    int mUnknown140;               // +0x140, written by the child slot 38
    int mUnknown144;               // +0x144, written by the child slot 38
    int mUnknown148;               // +0x148, written by two of the three child slot 38 overrides
    int mUnknown14c;               // +0x14c, written by slots 26 and 40
};
