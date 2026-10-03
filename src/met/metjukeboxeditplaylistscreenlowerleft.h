#pragma once

#include "met/metscreen.h"

/**
 * Detail panel in the lower left of the jukebox playlist editor.
 *
 * Its RTTI descriptor is at `0x008ef2d0`. It has MetScreen as its one public non-virtual base at
 * offset 0. The class declares no data member, and the object is the 0x8c bytes of MetScreen alone.
 * The 39-entry vtable at `0x007ee758` is the same length as the MetScreen table, and the class
 * declares no new virtual.
 *
 * The constructor at `0x00237758` takes only the renderer and the load priority, and supplies
 * `jbed` for the screen name, `metagame/Shared` for the directory, and `juke_edit_data` for the
 * container. Beyond the three names and its own vptr it writes nothing.
 *
 * The destructor at `0x0023ab10` restores the vptr, runs the MetScreen destructor, and releases
 * the object with the tag `MsgSink`.
 *
 * Eleven slots differ from the MetScreen table. Slots 20 through 24 sit eight bytes apart at
 * `0x0023aa40` through `0x0023aa60` and are two-instruction `jr ra` stubs. Of the rest only the
 * destructor has a recovered name, and the others are 19 `0x0023ab68`, 33 `0x0023ab70`,
 * 36 `0x0023ab78`, and 38 `0x0023aaf0`.
 */
class MetJukeboxEditPlaylistScreenLowerLeft : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x00237758
     * @ghidraAddress PAL: 0x0024b7b8
     */
    MetJukeboxEditPlaylistScreenLowerLeft(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x0023ab10
     * @ghidraAddress PAL: 0x0024f180
     */
    virtual ~MetJukeboxEditPlaylistScreenLowerLeft();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x0023aa68
     * @ghidraAddress PAL: 0x0024f0f8
     */
    static MetJukeboxEditPlaylistScreenLowerLeft *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Ignore every command.
     *
     * Slot 19, overridden empty.
     *
     * @param pCommand The command, which the body does not read.
     * @ghidraAddress NTSC-U/C: 0x0023ab68
     * @ghidraAddress PAL: 0x0024f1d8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Slot 33, overridden empty.
     *
     * @ghidraAddress NTSC-U/C: 0x0023ab70
     * @ghidraAddress PAL: 0x0024f1e0
     */
    virtual void OnEnterFinished();

    /**
     * Slot 36, overridden empty.
     *
     * @ghidraAddress NTSC-U/C: 0x0023ab78
     * @ghidraAddress PAL: 0x0024f1e8
     */
    virtual void OnExitFinished();

    /**
     * Resolve the container views.
     *
     * Slot 38. Forwards to the MetScreen body. The North American release does not add
     * behaviour. The European release then fills the four instruction texts from the current
     * language.
     *
     * @ghidraAddress NTSC-U/C: 0x0023aaf0
     * @ghidraAddress PAL: 0x0024b988
     */
    virtual void ResolveContainerViews();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress NTSC-U/C: 0x0023aa40
     * @ghidraAddress PAL: 0x0024f0d0
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * @param nSelector The pad index of the command, which the body does not read.
     * @ghidraAddress NTSC-U/C: 0x0023aa48
     * @ghidraAddress PAL: 0x0024f0d8
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress NTSC-U/C: 0x0023aa50
     * @ghidraAddress PAL: 0x0024f0e0
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress NTSC-U/C: 0x0023aa58
     * @ghidraAddress PAL: 0x0024f0e8
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress NTSC-U/C: 0x0023aa60
     * @ghidraAddress PAL: 0x0024f0f0
     */
    virtual void PlayCycleRightSound(int nSelector);
};
