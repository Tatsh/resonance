#pragma once

#include "memcard/memcarduser.h"
#include "met/metloadfreqbasescreen.h"

/**
 * Screen that loads a saved FreQ identity from a memory card.
 *
 * `17MetLoadFreqScreen` in the RTTI descriptor at `0x00901f10`, with two public non-virtual bases
 * at fixed offsets, MetLoadFreqBaseScreen at `+0x00` and MemcardUser at `+164`. The object is 0xac
 * bytes. The 47-entry primary vtable is at `0x007f6fc0` and the 21-entry MemcardUser table at
 * `0x007f6f10` adjusts `this` by `-164`. The primary is the same length as the
 * MetLoadFreqBaseScreen table, so the class declares no virtual of its own.
 *
 * The constructor at `0x0029bcf0` takes only the renderer and the load priority, runs the
 * MetLoadFreqBaseScreen constructor at `0x00291e00`, which supplies all three names, and writes
 * its own two vptrs and mUnknowna8. The destructor at `0x0029bd38` restores the primary vptr,
 * restores the MemcardUser vptr to `0x007daf78`, runs the MetLoadFreqBaseScreen destructor, and
 * releases the object with the tag `MsgSink`.
 *
 * Eleven slots differ from the MetLoadFreqBaseScreen table, and only the destructor has a
 * recovered name. The others are 5 `0x00297448`, 15 `0x00298510`, 33 `0x0029bdc8`,
 * 39 `0x002976f8`, 40 `0x002978d0`, 41 `0x00297528`, 43 `0x00297c10`, 44 `0x0029bda0`, and
 * 45 `0x00296fe8`.
 */
class MetLoadFreqScreen : public MetLoadFreqBaseScreen, public MemcardUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0029bcf0
     */
    MetLoadFreqScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0029bd38
     */
    virtual ~MetLoadFreqScreen();

private:
    int mUnknowna8; // +0xa8
};
