#pragma once

#include "met/metjukeboxbasescreen.h"

/**
 * Jukebox list of the remixes a player saved.
 *
 * Its RTTI descriptor is at `0x008ef470`. It has MetJukeboxBaseScreen as its one public non-virtual
 * base at offset 0. The class declares no data member, and the object is the 0x150 bytes of
 * MetJukeboxBaseScreen alone. The 43-entry primary vtable is at `0x007ecfc8`, the same length as
 * the MetJukeboxBaseScreen table, and the class declares no new virtual. The four-entry
 * ListDataProvider table at `0x007ecfa0` adjusts `this` by `-140` in every entry and overrides no
 * inherited entry.
 *
 * The constructor at `0x00224f40` takes only the renderer and the load priority. It runs the
 * MetJukeboxBaseScreen constructor at `0x0021dcc0` with `jbs` for the screen name,
 * `metagame/Shared` for the directory, and `juke_saved` for the container, and clears
 * mCatalogueKey.
 *
 * The destructor at `0x0022a850` releases the object with the tag `MsgSink` and does nothing of
 * its own.
 *
 * Two inherited slots differ from the MetJukeboxBaseScreen table beyond the destructor. Slot 38 at
 * `0x002250c0` extends the base view resolution, and slot 39 at `0x0022a910` supplies the pure
 * virtual. Slots 40, 41, and 42 are inherited unchanged.
 */
class MetJukeboxCustomRemixesScreen : public MetJukeboxBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x00224f40
     * @ghidraAddress PAL: 0x00237ee8
     */
    MetJukeboxCustomRemixesScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x0022a850
     * @ghidraAddress PAL: 0x0023dc00
     */
    virtual ~MetJukeboxCustomRemixesScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x0022a938
     * @ghidraAddress PAL: 0x0023dce8
     */
    static MetJukeboxCustomRemixesScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Resolve the base views, build both scrolling lists, and resolve every detail object.
     *
     * Slot 38. The catalogue list clones `jbs_remix_factory_01.view` with the highlight and both
     * arrows, and the playlist list clones `jbs_remix_playlist_01.view` five rows deep with none
     * of the three. The warning text takes the `remix_unavail_disc` prompt. The European release
     * also sets the playlist caption to the `My Playlist` text of the current language.
     *
     * @ghidraAddress NTSC-U/C: 0x002250c0
     * @ghidraAddress PAL: 0x002380c8
     */
    virtual void ResolveContainerViews();

    /**
     * Report the number of saved remixes.
     *
     * Slot 39. The size of the catalogue mCatalogue addresses, read without a null check.
     *
     * @return The row count.
     * @ghidraAddress NTSC-U/C: 0x0022a910
     * @ghidraAddress PAL: 0x0023dcc0
     */
    virtual int GetItemCount();
};
