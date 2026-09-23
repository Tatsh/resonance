#pragma once

#include "met/metjukeboxbasescreen.h"

/**
 * Jukebox playlist editor.
 *
 * `28MetJukeboxEditPlaylistScreen` in the RTTI descriptor at `0x008ef4e0`, with
 * MetJukeboxBaseScreen as its one public non-virtual base at offset 0. The class declares no data
 * member, so the object is the 0x150 bytes of MetJukeboxBaseScreen alone. The 43-entry primary
 * vtable is at `0x007ed8d0`, the same length as the MetJukeboxBaseScreen table, so the class
 * declares no virtual of its own. The four-entry ListDataProvider table at `0x007ed8a8` adjusts
 * `this` by `-140` in every entry and overrides slot 2 at `0x0022ba08`.
 *
 * The constructor at `0x0022acf8` takes only the renderer and the load priority. It runs the
 * MetJukeboxBaseScreen constructor at `0x0021dcc0` with `jbep` for the screen name,
 * `metagame/Shared` for the directory, and `juke_edit_playlist` for the container, and clears
 * mUnknownc8. The destructor at `0x00231228` releases the object with the tag `MsgSink` and does
 * nothing of its own.
 *
 * The screen builds only the playlist list, as mUnknown9c, and leaves mUnknown98 null. Its rows
 * and details come from the playlist rather than the catalogue.
 *
 * Seven inherited slots differ from the MetJukeboxBaseScreen table beyond the destructor. They are
 * 7 `0x002313d0`, 19 `0x0022bdb8`, 33 `0x00231388`, 38 `0x0022ae78`, 39 `0x002312e8`,
 * 40 `0x0022bfd8`, and 41 `0x0022c7f0`. Slot 42 is inherited unchanged, which an earlier reading
 * recorded as an override.
 */
class MetJukeboxEditPlaylistScreen : public MetJukeboxBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0022acf8
     */
    MetJukeboxEditPlaylistScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00231228
     */
    virtual ~MetJukeboxEditPlaylistScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00231300
     */
    static MetJukeboxEditPlaylistScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Run the base slot and nothing else.
     *
     * Slot 7. The override adds no behaviour.
     *
     * @ghidraAddress 0x002313d0
     */
    virtual void OnUnknownSlot7();

    /**
     * Edit the playlist.
     *
     * Slot 19. Previous and next move the selection. Select removes the selected entry and keeps
     * the selection on the row that follows it, or on the last row. Command 8 empties the
     * playlist, and commands 11 and 12 move the selected entry up or down by one. Every command
     * that changes the list refreshes it and runs slot 40.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x0022bdb8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Forget the catalogue and refresh the playlist.
     *
     * Slot 33. mUnknowna0 becomes null, and the playlist list takes the entry count.
     *
     * @ghidraAddress 0x00231388
     */
    virtual void OnUnknownSlot33();

    /**
     * Resolve the base views, build the playlist list, and resolve every detail object.
     *
     * Slot 38. The list clones `jbep_remix_factory_01.view` with the highlight and both arrows,
     * becomes mUnknown9c, and uses context 0. mUnknown98 stays null.
     *
     * @ghidraAddress 0x0022ae78
     */
    virtual void ResolveContainerViews();

    /**
     * Report the number of entries in the playlist under edit.
     *
     * Slot 39. The entry count of the playlist mUnknownc4 addresses, read without a null check.
     *
     * @return The row count.
     * @ghidraAddress 0x002312e8
     */
    virtual int GetItemCount();

    /**
     * Fill the detail texts from the selected playlist entry.
     *
     * Slot 40. The same layout as the base slot, for the record MetRemixManager::LookupRemix()
     * finds by the entry's name. There is no album test, so the lock text never shows, and the
     * result of the lookup is not tested for null.
     *
     * @ghidraAddress 0x0022bfd8
     */
    virtual void ShowRemixDetails();

    /**
     * Select the editor's help text.
     *
     * Slot 41. Selects the `met_jukebox_edit_screen_help_tab` layout and posts
     * `met_jukebox_edit_screen_ticker_tape`.
     *
     * @ghidraAddress 0x0022c7f0
     */
    virtual void UpdateHelpText();

    /**
     * Show one row of the playlist.
     *
     * ListDataProvider slot 2. Context 0 shows the indexed entry's name from a copy of the entry
     * vector, and an index past the end empties the text. Any other context leaves the cell as it
     * is. The column is not read.
     *
     * @param nItem The row.
     * @param nColumn The cell index, which the body does not read.
     * @param pText The cell.
     * @param nContext 0 for the playlist.
     * @return Always 1.
     * @ghidraAddress 0x0022ba08
     */
    virtual int ProvideText(int nItem, int nColumn, Rnd::Text *pText, int nContext);
};
