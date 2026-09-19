#pragma once

#include <vector>

#include "met/metbuttonlist.h"
#include "met/metscreenmultisoundbank.h"
#include "rnd/object.h"

/**
 * Screen that edits the in-game options.
 *
 * `26MetConfigGameOptionsScreen` in the RTTI descriptor at `0x00901ee0`, with
 * MetScreenMultiSoundBank as its one public non-virtual base at offset 0. The object is 0xa8 bytes
 * and the 39-entry vtable is at `0x007eaea8`, the same length as the MetScreen table, so the class
 * declares no virtual of its own.
 *
 * The constructor at `0x0020c3e0` takes only the renderer and the load priority, and supplies
 * `nop` for the screen name, `metagame/shared` for the directory, and `net_options_pangame` for
 * the container. It writes the MetScreenMultiSoundBank vtable at `0x0080b560` before its own,
 * which is how that base constructor is known to be inline here rather than called. It then
 * allocates a MetButtonList tagged `MetButtonList`, empties both vectors below, and pushes
 * `pangame_audio` and `pangame_force_feedback` into the container object-name vector that
 * MetScreen owns.
 *
 * The destructor at `0x0020ce20` releases both vectors, runs the MetScreen destructor, and
 * releases the object with the tag `MsgSink`.
 *
 * Six slots differ from the MetScreenMultiSoundBank table, and only the destructor has a recovered
 * name.
 *
 *  - 1 `0x0020ce20` the destructor.
 *  - 5 `0x0020d310` replaces MetScreen::EnterAndShow at `0x003900a8`.
 *  - 19 `0x0020cf70` replaces an empty MetScreen slot.
 *  - 36 `0x0020d5a8` replaces an empty MetScreen slot.
 *  - 38 `0x0020c800` replaces MetScreen::ResolveContainerViews at `0x0038b1b0`.
 *
 * The five sounds the base swapped for the multiplayer bank stay as MetScreenMultiSoundBank
 * defined them.
 */
class MetConfigGameOptionsScreen : public MetScreenMultiSoundBank {
public:
    /**
     * Construct the in-game options screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0020c3e0
     */
    MetConfigGameOptionsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0020ce20
     */
    virtual ~MetConfigGameOptionsScreen();

private:
    MetButtonList *mUnknown8c;             // +0x8c
    std::vector<Rnd::Object *> mUnknown90; // +0x90
    std::vector<Rnd::Object *> mUnknown9c; // +0x9c
};
