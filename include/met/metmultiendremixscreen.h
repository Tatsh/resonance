#pragma once

#include <vector>

#include "met/metscreen.h"
#include "rnd/object.h"

/**
 * End-of-remix screen for a multiplayer session.
 *
 * `22MetMultiEndRemixScreen` in the RTTI descriptor at `0x008f07e0`, with MetScreen as its one
 * public non-virtual base at offset 0. The object is 0xc0 bytes and the 39-entry vtable is at
 * `0x007fed80`, the same length as the MetScreen table, so the class declares no virtual of its
 * own.
 *
 * The constructor at `0x002f0918` takes only the renderer and the load priority, and supplies
 * `erm` for the screen name, `metagame/Shared` for the directory, and `end_multi_remix` for the
 * container. It empties the three vectors below. The constructor resolves two texture pairs into
 * its own vectors, `gSongLogo1.tex` with `gSongLogo2.tex` and `gSongLabel1.tex` with
 * `gSongLabel2.tex`, through the helper at `0x00246de0`.
 *
 * The three words from `+0x8c` to `+0x98` are never written by the constructor, which is what
 * distinguishes this layout from MetRemixDataScreen. That screen resolves the same two texture
 * pairs and places its three vectors 0x10 lower. The purpose of the span is not recovered.
 *
 * The destructor at `0x002f1500` releases the three vectors through the helper at `0x001fc568`,
 * returns their buffers to the pool, runs the MetScreen destructor, and releases the object with
 * the tag `MsgSink`.
 *
 * Five slots differ from the MetScreen table, and only the destructor has a recovered name. The
 * rest are 5 `0x002f16a0`, 26 `0x002f55c8`, 36 `0x002f5650`, and 38 `0x002f0da8`.
 */
class MetMultiEndRemixScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002f0918
     */
    MetMultiEndRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002f1500
     */
    virtual ~MetMultiEndRemixScreen();

private:
    // Never written by the constructor and not recovered.
    unsigned char mUnknown8c[0x10];        // +0x8c
    std::vector<Rnd::Object *> mUnknown9c; // +0x9c
    std::vector<Rnd::Object *> mUnknowna8; // +0xa8
    std::vector<Rnd::Object *> mUnknownb4; // +0xb4
};
