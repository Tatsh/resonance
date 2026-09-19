#pragma once

#include "met/metmultitipsbasescreen.h"

/**
 * One page of the multiplayer loading tips.
 *
 * `19MetMultiTips2Screen` in the RTTI descriptor at `0x008eedf8`, with MetMultiTipsBaseScreen as
 * its one public non-virtual base at offset 0. The class declares no data member, and the 39-entry
 * vtable at `0x00801390` is the same length as the MetMultiTipsBaseScreen table, so it declares no
 * virtual of its own either.
 *
 * The constructor at `0x00307d68` takes only the renderer and the load priority. It runs the
 * MetMultiTipsBaseScreen constructor at `0x00306cd8` with `tp2` for the screen
 * name, `multi_tip_02` for the container, and two screen registry keys, `MetMultiTips1Screen` for
 * the previous page and `MetMultiTips3Screen` for the next. The five tip pages form a ring through
 * those two keys, and both ends of the ring lead to `MetLocNumPlayersScreen`.
 *
 * The destructor is at `0x0030da98`.
 *
 * Apart from the type function and the destructor, the slots that differ from the
 * MetMultiTipsBaseScreen table are 38 `0x00307f38`.
 */
class MetMultiTips2Screen : public MetMultiTipsBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00307d68
     */
    MetMultiTips2Screen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0030da98
     */
    virtual ~MetMultiTips2Screen();
};
