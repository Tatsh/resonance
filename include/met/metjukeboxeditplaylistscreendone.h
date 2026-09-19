#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"

/**
 * Confirmation buttons for the jukebox playlist editor.
 *
 * `32MetJukeboxEditPlaylistScreenDone` in the RTTI descriptor at `0x008eebf8`, with MetScreen as
 * its one public non-virtual base at offset 0. The object is 0x9c bytes and the 39-entry vtable is
 * at `0x007ee0f8`, the same length as the MetScreen table, so the class declares no virtual of its
 * own.
 *
 * The constructor at `0x00231728` takes only the renderer and the load priority, and supplies
 * `jbd` for the screen name, `metagame/Shared` for the directory, and `juke_done_butts` for the
 * container. The four object names the screen resolves are `met_jukebox_done_screen_tab_play`,
 * `met_jukebox_done_screen_ticker_play`, `met_jukebox_done_screen_tab_save`, and
 * `met_jukebox_done_screen_ticker_save`, at `0x007edfe8` through `0x007ee060`.
 *
 * The destructor at `0x00237210` restores the vptr, runs the MetScreen destructor, and releases
 * the object with the tag `MsgSink`. It releases nothing of its own, so the button list at
 * `+0x8c` outlives the screen.
 *
 * Fourteen slots differ from the MetScreen table. Slots 20 through 24 sit eight bytes apart at
 * `0x00237160` through `0x00237180` and are two-instruction `jr ra` stubs, so this screen plays
 * none of those five sounds. Of the rest only the destructor has a recovered name, and the others
 * are 5 `0x00237330`, 17 `0x002373e0`, 19 `0x00237268`, 30 `0x002321f8`, 33 `0x002373a8`,
 * 36 `0x00231b50`, and 38 `0x00231908`.
 */
class MetJukeboxEditPlaylistScreenDone : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00231728
     */
    MetJukeboxEditPlaylistScreenDone(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00237210
     */
    virtual ~MetJukeboxEditPlaylistScreenDone();

    /**
     * @ghidraAddress 0x00237160
     */
    virtual void PlaySlideSound();

    /**
     * @ghidraAddress 0x00237168
     */
    virtual void PlayLeaveSound();

    /**
     * @ghidraAddress 0x00237170
     */
    virtual void PlayHighSound();

    /**
     * @ghidraAddress 0x00237178
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x00237180
     */
    virtual void PlayCycleRightSound();

private:
    MetButtonList *mUnknown8c; // +0x8c
    int mUnknown90;            // +0x90
    int mUnknown94;            // +0x94
    int mUnknown98;            // +0x98
};
