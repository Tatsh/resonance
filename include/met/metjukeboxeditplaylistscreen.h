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
 * This is the one jukebox child that overrides more than the view resolution and the row count.
 * Seven inherited slots differ from the MetJukeboxBaseScreen table beyond the destructor, and only
 * slot 39 has a recovered name. They are 7 `0x002313d0`, 19 `0x0022bdb8`, 33 `0x00231388`,
 * 38 `0x0022ae78`, 39 `0x002312e8`, 40 `0x0022bfd8`, and 41 `0x0022c7f0`. Slot 42 is inherited
 * unchanged, which an earlier reading recorded as an override.
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
     * Report the number of entries in the playlist under edit.
     *
     * Slot 39. Divides the byte span of the vector mUnknownc4 addresses by four, which is what
     * separates this override from the two that divide by 0x38. The vector is dereferenced without
     * a null check.
     *
     * The body is not written. mUnknownc4 addresses a vector inside the shared MetRemixManager
     * whose element class is not recovered, so the division cannot be expressed as a size query.
     * MetScreen::StartRepeatingSound() records a gap of the same shape.
     *
     * @return The row count.
     * @ghidraAddress 0x002312e8
     */
    virtual int GetItemCount();
};
