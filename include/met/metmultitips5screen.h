#pragma once

#include "met/metmultitipsbasescreen.h"

/**
 * One page of the multiplayer loading tips.
 *
 * `19MetMultiTips5Screen` in the RTTI descriptor at `0x008eee28`, with MetMultiTipsBaseScreen as
 * its one public non-virtual base at offset 0. The class declares no data member, and the 39-entry
 * vtable at `0x00800fd0` is the same length as the MetMultiTipsBaseScreen table, so it declares no
 * virtual of its own either.
 *
 * The constructor at `0x00309fa0` takes only the renderer and the load priority. It runs the
 * MetMultiTipsBaseScreen constructor at `0x00306cd8` with `tp5` for the screen
 * name, `multi_tip_05` for the container, and two screen registry keys, `MetMultiTips4Screen` for
 * the previous page and `MetLocNumPlayersScreen` for the next. The five tip pages form a ring
 * through those two keys, and both ends of the ring lead to `MetLocNumPlayersScreen`.
 *
 * The destructor is at `0x0030df48`.
 *
 * Apart from the type function and the destructor, the slots that differ from the
 * MetMultiTipsBaseScreen table are 36 `0x0030a4d8` and 38 `0x0030a170`.
 */
class MetMultiTips5Screen : public MetMultiTipsBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00309fa0
     */
    MetMultiTips5Screen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0030df48
     */
    virtual ~MetMultiTips5Screen();
};
