#pragma once

#include "met/metmultitipsbasescreen.h"

/**
 * One page of the multiplayer loading tips.
 *
 * `19MetMultiTips1Screen` in the RTTI descriptor at `0x008eede8`, with MetMultiTipsBaseScreen as
 * its one public non-virtual base at offset 0. The class declares no data member, and the 39-entry
 * vtable at `0x008014d0` is the same length as the MetMultiTipsBaseScreen table, so it declares no
 * virtual of its own either.
 *
 * The constructor at `0x00307480` takes only the renderer and the load priority. It runs the
 * MetMultiTipsBaseScreen constructor at `0x00306cd8` with `tp1` for the screen
 * name, `multi_tip_01` for the container, and two screen registry keys, `MetLocNumPlayersScreen`
 * for the previous page and `MetMultiTips2Screen` for the next. The five tip pages form a ring
 * through those two keys, and both ends of the ring lead to `MetLocNumPlayersScreen`.
 *
 * The destructor is at `0x0030d908`.
 *
 * Apart from the type function and the destructor, the slots that differ from the
 * MetMultiTipsBaseScreen table are 36 `0x00307bc8` and 38 `0x00307650`.
 */
class MetMultiTips1Screen : public MetMultiTipsBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00307480
     */
    MetMultiTips1Screen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0030d908
     */
    virtual ~MetMultiTips1Screen();
};
