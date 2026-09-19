#pragma once

#include "met/metscreen.h"

/**
 * Screen that picks the number of local players.
 *
 * `19MetLocNumPlayScreen` in the RTTI descriptor at `0x008f08e0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007f9228`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002ad7e8` takes only the renderer and the load priority, and supplies
 * `mnp` for the screen name, `metagame/_Local` for the directory, and `num_players` for the
 * container. It writes only `+0x8c`, into which it allocates a MetButtonList.
 *
 * It pushes four object names into the container object-name vector that MetScreen owns, `loc_2p`,
 * `loc_3p`, `loc_4p`, and `multi_tips`. Its own registry key is the literal
 * `MetLocNumPlayersScreen`, which differs from the class name by three letters, and the five
 * MetMultiTips screens use that literal at both ends of their ring.
 *
 * The object is at least 0x90 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x002b1008`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x002ae218`, 19 `0x002adef0`, 23 `0x002b0f70`, 24 `0x002b0f78`, 30 `0x002ae350`, 36
 * `0x002ae4f0`, 38 `0x002adbf0`.
 */
class MetLocNumPlayScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002ad7e8
     */
    MetLocNumPlayScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002b1008
     */
    virtual ~MetLocNumPlayScreen();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002b0f70
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002b0f78
     */
    virtual void PlayCycleRightSound(int nSelector);
};
