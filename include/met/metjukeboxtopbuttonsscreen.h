#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

/**
 * Row of buttons along the top of the jukebox.
 *
 * `26MetJukeboxTopButtonsScreen` in the RTTI descriptor at `0x008ef910`, with MetScreen as its one
 * public non-virtual base at offset 0. The object is 0x9c bytes and the 39-entry vtable is at
 * `0x007ef8b0`, the same length as the MetScreen table, so the class declares no virtual of its
 * own.
 *
 * The constructor at `0x00240c28` takes only the renderer and the load priority, and supplies
 * `jbb` for the screen name, `metagame/Shared` for the directory, and `juke_butts` for the
 * container. It zeroes the members below and then allocates a MetButtonList of 0x18 bytes tagged
 * `MetButtonList` into mUnknown94.
 *
 * The destructor at `0x00246778` restores the vptr, frees the buffer at `+0x90` through the
 * untagged path, runs the MetScreen destructor, and releases the object with the tag `MsgSink`.
 * That free is the inlined HxStr destructor of mUnknown8c rather than a statement of the
 * destructor's own, which is what identifies the eight bytes at `+0x8c` as an `HxStr`. The
 * constructor zeroing both of its words, and the destructor freeing the second word alone, are
 * the two halves of that evidence. An earlier reading recorded the four words as integers and
 * could not attribute the free.
 *
 * Nothing frees the button list, so it is never released.
 *
 * Nine slots differ from the MetScreen table, and only the destructor has a recovered name. The
 * rest are 5 `0x00241b08`, 9 `0x00242140`, 19 `0x002467e8`, 26 `0x002468d8`, 33 `0x002468b8`,
 * 36 `0x00241120`, and 38 `0x00240e28`.
 */
class MetJukeboxTopButtonsScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00240c28
     */
    MetJukeboxTopButtonsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00246778
     */
    virtual ~MetJukeboxTopButtonsScreen();

private:
    HxStr mUnknown8c;          // +0x8c
    MetButtonList *mUnknown94; // +0x94
    int mUnknown98;            // +0x98
};
