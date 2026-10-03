#pragma once

#include "met/metjukeboxbasescreen.h"

/**
 * Jukebox list of the remixes that shipped with the game.
 *
 * Its RTTI descriptor is at `0x008eee68`. It has MetJukeboxBaseScreen as its one public non-virtual
 * base at offset 0. The class declares no data member, and the object is the 0x150 bytes of
 * MetJukeboxBaseScreen alone. The 43-entry primary vtable is at `0x007eeff0`, the same length as
 * the MetJukeboxBaseScreen table, and the class declares no new virtual. The four-entry
 * ListDataProvider table at `0x007eefc8` adjusts `this` by `-140` in every entry and overrides no
 * inherited entry.
 *
 * The constructor at `0x0023ae58` takes only the renderer and the load priority. It runs the
 * MetJukeboxBaseScreen constructor at `0x0021dcc0` with `jbf` for the screen name,
 * `metagame/Shared` for the directory, and `juke_factory` for the container, and sets mCatalogueKey
 * to -1. That value is what separates this screen from the other two, which both clear the same
 * member. The destructor at `0x00240780` releases the object with the tag `MsgSink` and does
 * nothing of its own.
 *
 * Two inherited slots differ from the MetJukeboxBaseScreen table beyond the destructor. Slot 38 at
 * `0x0023afd8` extends the base view resolution, and slot 39 at `0x00240840` supplies the pure
 * virtual with a body byte-for-byte identical to the MetJukeboxCustomRemixesScreen override. Slots
 * 40, 41, and 42 are inherited unchanged.
 */
class MetJukeboxFactoryRemixesScreen : public MetJukeboxBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0023ae58
     */
    MetJukeboxFactoryRemixesScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00240780
     */
    virtual ~MetJukeboxFactoryRemixesScreen();

    /**
     * Report the number of factory remixes.
     *
     * Slot 39. The size of the catalogue mCatalogue addresses, read without a null check. The
     * body coincides byte for byte with MetJukeboxCustomRemixesScreen::GetItemCount(), and the
     * vtable slot each occupies is what separates them.
     *
     * @return The row count.
     * @ghidraAddress 0x00240840
     */
    virtual int GetItemCount();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00240868
     */
    static MetJukeboxFactoryRemixesScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Resolve the base views, build both scrolling lists, and resolve every detail object.
     *
     * Slot 38. The same layout as MetJukeboxCustomRemixesScreen::ResolveContainerViews() over the
     * `jbf_` objects, except that the catalogue row view is also shown, and the playlist caption
     * is resolved from `dbf_CREATE PLAYLIST.txt`, the name the image records. The European release
     * also sets the caption to the `My Playlist` text of the current language.
     *
     * @ghidraAddress NTSC-U/C: 0x0023afd8
     * @ghidraAddress PAL: 0x0024f6c8
     */
    virtual void ResolveContainerViews();
};
