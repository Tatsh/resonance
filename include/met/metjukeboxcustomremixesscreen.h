#pragma once

#include "met/metjukeboxbasescreen.h"

/**
 * Jukebox list of the remixes a player saved.
 *
 * `29MetJukeboxCustomRemixesScreen` in the RTTI descriptor at `0x008ef470`, with
 * MetJukeboxBaseScreen as its one public non-virtual base at offset 0. The class declares no data
 * member, so the object is the 0x150 bytes of MetJukeboxBaseScreen alone. The 43-entry primary
 * vtable is at `0x007ecfc8`, the same length as the MetJukeboxBaseScreen table, so the class
 * declares no virtual of its own. The four-entry ListDataProvider table at `0x007ecfa0` adjusts
 * `this` by `-140` in every entry and overrides no inherited entry.
 *
 * The constructor at `0x00224f40` takes only the renderer and the load priority. It runs the
 * MetJukeboxBaseScreen constructor at `0x0021dcc0` with `jbs` for the screen name,
 * `metagame/Shared` for the directory, and `juke_saved` for the container, and clears mUnknownc8.
 *
 * The destructor at `0x0022a850` releases the object with the tag `MsgSink` and does nothing of
 * its own.
 *
 * Two inherited slots differ from the MetJukeboxBaseScreen table beyond the destructor. Slot 38 at
 * `0x002250c0` extends the base view resolution, and slot 39 at `0x0022a910` supplies the pure
 * virtual. Slots 40, 41, and 42 are inherited unchanged, which an earlier reading recorded as
 * overrides.
 */
class MetJukeboxCustomRemixesScreen : public MetJukeboxBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00224f40
     */
    MetJukeboxCustomRemixesScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0022a850
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
     * @ghidraAddress 0x0022a938
     */
    static MetJukeboxCustomRemixesScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Resolve the base views, build both scrolling lists, and resolve every detail object.
     *
     * Slot 38. The catalogue list clones `jbs_remix_factory_01.view` with the highlight and both
     * arrows, and the playlist list clones `jbs_remix_playlist_01.view` five rows deep with none
     * of the three. The warning text takes the `remix_unavail_disc` prompt.
     *
     * @ghidraAddress 0x002250c0
     */
    virtual void ResolveContainerViews();

    /**
     * Report the number of saved remixes.
     *
     * Slot 39. The size of the catalogue mUnknowna0 addresses, read without a null check.
     *
     * @return The row count.
     * @ghidraAddress 0x0022a910
     */
    virtual int GetItemCount();
};
