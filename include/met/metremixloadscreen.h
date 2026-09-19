#pragma once

#include "met/listdataprovider.h"
#include "met/metbuttonlist.h"
#include "met/metscreen.h"

/**
 * Screen that lists the remixes on a memory card for loading.
 *
 * `18MetRemixLoadScreen` in the RTTI descriptor at `0x00901c10`, with two public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00` and ListDataProvider at `+140`. The object is 0xac bytes.
 * The 39-entry primary vtable is at `0x00807300`, the same length as the MetScreen table, so the
 * class declares no virtual of its own, and the four-entry ListDataProvider table at `0x008072d8`
 * adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x00349dc0` takes only the renderer and the load priority, and supplies
 * `mcrl` for the screen name, `metagame/Shared` for the directory, and `memcard_remix_load` for
 * the container. It then clears MetScreen::mUnknown60, which is why that member is protected
 * rather than private, and allocates a MetButtonList tagged `MetButtonList` into mUnknowna8. The
 * words at `+0x90`,
 * `+0x98`, and `+0x9c` are never written.
 *
 * The destructor at `0x00352618` restores both vptrs, clears mUnknown94, restores the
 * ListDataProvider vptr to `0x007ec830`, runs the MetScreen destructor, and releases the object
 * with the tag `MsgSink`.
 *
 * Nine slots differ from the MetScreen table, and only the destructor has a recovered name. The
 * rest are 5 `0x0034b568`, 19 `0x0034a2a8`, 20 `0x003526d0`, 22 `0x00352720`, 33 `0x00352770`,
 * 36 `0x0034cbd0`, and 38 `0x00349fc8`.
 */
class MetRemixLoadScreen : public MetScreen, public ListDataProvider {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00349dc0
     */
    MetRemixLoadScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00352618
     */
    virtual ~MetRemixLoadScreen();

private:
    int mUnknown90;            // +0x90, not written by the constructor
    int mUnknown94;            // +0x94
    int mUnknown98;            // +0x98
    int mUnknown9c;            // +0x9c
    int mUnknowna0;            // +0xa0
    int mUnknowna4;            // +0xa4
    MetButtonList *mUnknowna8; // +0xa8
};
