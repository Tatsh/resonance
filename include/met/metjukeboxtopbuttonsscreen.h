#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"

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
 * container. It zeroes the four words below and then allocates a MetButtonList of 0x18 bytes
 * tagged `MetButtonList` into mUnknown94.
 *
 * The destructor at `0x00246778` restores the vptr, releases one block through the untagged path,
 * runs the MetScreen destructor, and releases the object with the tag `MsgSink`. Which of the four
 * members that release covers is not determined, because the destructor reads no offset before
 * it.
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
    int mUnknown8c;            // +0x8c
    int mUnknown90;            // +0x90
    MetButtonList *mUnknown94; // +0x94
    int mUnknown98;            // +0x98
};
