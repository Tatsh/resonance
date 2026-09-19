#pragma once

#include "met/metjukeboxbasescreen.h"

/**
 * Jukebox list screen.
 *
 * `28MetJukeboxEditPlaylistScreen` in the RTTI descriptor at `0x008ef4e0`, with
 * MetJukeboxBaseScreen as its one public non-virtual base at offset 0. Its own members start at
 * `+0xc8`, which is the size of MetJukeboxBaseScreen, and the object is 0xcc bytes. The 43-entry
 * primary vtable is at `0x007ed8d0`, the same length as the MetJukeboxBaseScreen table, so the
 * class declares no virtual of its own. The four-entry ListDataProvider table at `0x007ed8a8`
 * adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x0022acf8` takes only the renderer and the load priority. It runs
 * the MetJukeboxBaseScreen constructor at `0x0021dcc0` with `jbep` for the screen
 * name, `metagame/Shared` for the directory, and `juke_edit_playlist` for the container, writes
 * its own two vptrs, and sets mUnknownc8 to zero. The destructor at `0x00231228` restores
 * both vptrs, runs the MetJukeboxBaseScreen destructor, and releases the object with
 * the tag `MsgSink`.
 *
 * Of the slots that differ from the MetJukeboxBaseScreen table, only the destructor
 * has a recovered name. The rest are 7 `0x002313d0`, 19 `0x0022bdb8`, 33 `0x00231388`, 38
 * `0x0022ae78`, 39 `0x002312e8`, 40 `0x0022bfd8`, 41 `0x0022c7f0`, 42 `0x0021e3f8`.
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

private:
    // Distinguishes the three jukebox lists. Zero for the saved and edit screens and
    // non-zero for the factory screen.
    int mUnknownc8; // +0xc8
};
