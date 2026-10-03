#pragma once

#include "met/metmultitipsbasescreen.h"

/**
 * One page of the multiplayer loading tips.
 *
 * Its RTTI descriptor is at `0x008eee28`. It has MetMultiTipsBaseScreen as its one public
 * non-virtual base at offset 0. The class declares no data member. The 39-entry vtable at
 * `0x00800fd0` is the same length as the MetMultiTipsBaseScreen table, and the class declares no
 * new virtual.
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
     * @ghidraAddress NTSC-U/C: 0x00309fa0
     * @ghidraAddress PAL: 0x0032f610
     */
    MetMultiTips5Screen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x0030df48
     * @ghidraAddress PAL: 0x00333b60
     */
    virtual ~MetMultiTips5Screen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x0030dfc8
     * @ghidraAddress PAL: 0x00333c08
     */
    static MetMultiTips5Screen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Return to the player-count screen on a select, and otherwise run the base slot.
     *
     * Slot 36. The last page has no next page to push.
     *
     * @ghidraAddress NTSC-U/C: 0x0030a4d8
     * @ghidraAddress PAL: 0x0032fec8
     */
    virtual void OnExitFinished();

    /**
     * Resolve the container views and fill the page's three texts from configuration code 0x258.
     *
     * Slot 38. The texts are not tested for null. The European release fills the texts from the
     * current language instead and also labels the two track texts `tp5_track_01.txt` and
     * `tp5_track_02.txt` with `SYNTH` and `BASS`.
     *
     * @ghidraAddress NTSC-U/C: 0x0030a170
     * @ghidraAddress PAL: 0x0032f858
     */
    virtual void ResolveContainerViews();
};
