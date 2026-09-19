#pragma once

#include "memcard/memcarduser.h"
#include "met/metscreen.h"

/**
 * End-of-game button row shown after a solo win.
 *
 * `16MetSoloWinScreen` in the RTTI descriptor at `0x008ef190`, with two public non-virtual bases at
 * fixed offsets, MetScreen at `+0x00`, and MemcardUser at `+140`.
 *
 * The 39-entry primary vtable is at `0x0080ea80`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The twenty-one-entry MemcardUser table at `0x0080e9d0` adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x003b55a8` takes only the renderer and the load priority, and supplies
 * `egwb` for the screen name, `metagame/_Solo` for the directory, and `end_win_butts` for the
 * container. It writes `+0x8c`, which is the MemcardUser vptr, then `+0x90` and `+0x94`.
 *
 * The object is at least 0x98 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x003b9ce8`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003b5968`, 19 `0x003b57a0`, 21 `0x003b9b90`, 23 `0x003b9b80`, 24 `0x003b9b88`, 30
 * `0x003b5f88`, 33 `0x003b9d80`, 36 `0x003b6188`.
 */
class MetSoloWinScreen : public MetScreen, public MemcardUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003b55a8
     */
    MetSoloWinScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003b9ce8
     */
    virtual ~MetSoloWinScreen();
};
