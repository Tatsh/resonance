#pragma once

#include "met/metmultitipsbasescreen.h"

/**
 * One page of the multiplayer loading tips.
 *
 * `19MetMultiTips3Screen` in the RTTI descriptor at `0x008eee08`, with MetMultiTipsBaseScreen as
 * its one public non-virtual base at offset 0. The class declares no data member, and the 39-entry
 * vtable at `0x00801250` is the same length as the MetMultiTipsBaseScreen table, so it declares no
 * virtual of its own either.
 *
 * The constructor at `0x003086c0` takes only the renderer and the load priority. It runs the
 * MetMultiTipsBaseScreen constructor at `0x00306cd8` with `tp3` for the screen
 * name, `multi_tip_03` for the container, and two screen registry keys, `MetMultiTips2Screen` for
 * the previous page and `MetMultiTips4Screen` for the next. The five tip pages form a ring through
 * those two keys, and both ends of the ring lead to `MetLocNumPlayersScreen`.
 *
 * The destructor is at `0x0030dc28`.
 *
 * Apart from the type function and the destructor, the slots that differ from the
 * MetMultiTipsBaseScreen table are 38 `0x00308890`.
 */
class MetMultiTips3Screen : public MetMultiTipsBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003086c0
     */
    MetMultiTips3Screen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0030dc28
     */
    virtual ~MetMultiTips3Screen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x0030dca8
     */
    static MetMultiTips3Screen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Resolve the container views and fill the page's seven texts from configuration code 0x258.
     *
     * Slot 38. The texts are not tested for null.
     *
     * @ghidraAddress 0x00308890
     */
    virtual void ResolveContainerViews();
};
