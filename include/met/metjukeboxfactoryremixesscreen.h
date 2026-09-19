#pragma once

#include "met/metjukeboxbasescreen.h"

/**
 * Jukebox list of the remixes that shipped with the game.
 *
 * `30MetJukeboxFactoryRemixesScreen` in the RTTI descriptor at `0x008eee68`, with
 * MetJukeboxBaseScreen as its one public non-virtual base at offset 0. The class declares no data
 * member, so the object is the 0x150 bytes of MetJukeboxBaseScreen alone. The 43-entry primary
 * vtable is at `0x007eeff0`, the same length as the MetJukeboxBaseScreen table, so the class
 * declares no virtual of its own. The four-entry ListDataProvider table at `0x007eefc8` adjusts
 * `this` by `-140` in every entry and overrides no inherited entry.
 *
 * The constructor at `0x0023ae58` takes only the renderer and the load priority. It runs the
 * MetJukeboxBaseScreen constructor at `0x0021dcc0` with `jbf` for the screen name,
 * `metagame/Shared` for the directory, and `juke_factory` for the container, and sets mUnknownc8
 * to -1. That value is what separates this screen from the other two, which both clear the same
 * member. The destructor at `0x00240780` releases the object with the tag `MsgSink` and does
 * nothing of its own.
 *
 * Two inherited slots differ from the MetJukeboxBaseScreen table beyond the destructor. Slot 38 at
 * `0x0023afd8` extends the base view resolution, and slot 39 at `0x00240840` supplies the pure
 * virtual with a body byte-for-byte identical to the MetJukeboxCustomRemixesScreen override. Slots
 * 40, 41, and 42 are inherited unchanged, which an earlier reading recorded as overrides.
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
     * Slot 39. Divides the byte span of the vector mUnknowna0 addresses by the 0x38-byte record
     * size. The vector is dereferenced without a null check.
     *
     * The body is not written, for the reason recorded on
     * MetJukeboxCustomRemixesScreen::GetItemCount(). The two bodies coincide byte for byte, and
     * the vtable slot each occupies is what separates them.
     *
     * @return The row count.
     * @ghidraAddress 0x00240840
     */
    virtual int GetItemCount();
};
