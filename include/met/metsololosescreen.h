#pragma once

#include "met/metscreen.h"

/**
 * End-of-game button row shown after a solo loss.
 *
 * `17MetSoloLoseScreen` in the RTTI descriptor at `0x008ef180`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080cc90`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x00399be8` takes only the renderer and the load priority, and supplies
 * `egb` for the screen name, `metagame/_Solo` for the directory, and `end_game_butts` for the
 * container. It writes only `+0x8c`, into which it allocates a MetButtonList.
 *
 * It loads the same container as MetMultiEndScreen under the same screen name `egb`.
 *
 * The object is at least 0x90 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x0039e010`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x00399f88`, 19 `0x00399db8`, 21 `0x0039df80`, 23 `0x0039df70`, 24 `0x0039df78`, 30
 * `0x0039a6e8`, 36 `0x0039a880`.
 */
class MetSoloLoseScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00399be8
     */
    MetSoloLoseScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0039e010
     */
    virtual ~MetSoloLoseScreen();
};
