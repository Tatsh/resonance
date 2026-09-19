#pragma once

#include "met/metremixsaver.h"
#include "met/metscreen.h"

/**
 * End-of-remix screen for a solo session.
 *
 * `21MetSoloEndRemixScreen` in the RTTI descriptor at `0x008f07f0`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00` and MetRemixSaver at `+140`. The class declares no
 * own members at `+0x90`, and the object is 0x11c bytes. The 39-entry primary vtable is at
 * `0x0080c4f0`, the same length as the MetScreen table, so the class declares no virtual of its
 * own, and the five-entry MetRemixSaver table at `0x0080c4c0` adjusts `this` by `-140` in every
 * entry. That table is where this screen supplies the three MetRemixSaver pure virtuals.
 *
 * The constructor at `0x003943f8` takes only the renderer and the load priority, and supplies
 * `erss` for the screen name, `metagame/_Solo` for the directory, and `end_remix` for the
 * container. It builds the two records at `+0xbc` and `+0xec` from the texture pairs
 * `gSongLogo1.tex` with `gSongLogo2.tex` and `gSongLabel1.tex` with `gSongLabel2.tex`.
 *
 * Two 0x30-byte records sit among its members, each built by the constructor at `0x00246de0` from
 * a pair of texture names. That routine zeroes `+0x00` through `+0x0c`, sets `+0x10` to one,
 * zeroes `+0x14` and `+0x18`, copy-constructs an `HxStr` at `+0x1c` from its second argument and
 * another at `+0x24` from its third, zeroes `+0x2c`, and returns the record. Eleven call sites
 * across the subsystem build one. The record's class is not identified, so both members are
 * recorded as a reserved span, and the size above depends on that span being 0x30 bytes rather
 * than on any declaration.
 *
 * The destructor at `0x003996b8` restores both vptrs, releases two entries through the helper at
 * `0x001fc568`, restores the MetRemixSaver vptr to `0x007ffcd0`, runs the MetScreen destructor,
 * and releases the object with the tag `MsgSink`.
 *
 * Five slots differ from the MetScreen table, and only the destructor has a recovered name. The
 * rest are 5 `0x00394e10`, 7 `0x003997d0`, 26 `0x00399748`, 36 `0x00399870`, and
 * 38 `0x00394728`.
 */
class MetSoloEndRemixScreen : public MetScreen, public MetRemixSaver {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003943f8
     */
    MetSoloEndRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003996b8
     */
    virtual ~MetSoloEndRemixScreen();

private:
    // Never written by the constructor and not recovered.
    unsigned char mUnknown90[0x2c]; // +0x90
    // The records the constructor at 0x00246de0 builds from a texture-name pair.
    unsigned char mUnknownbc[0x30]; // +0xbc
    unsigned char mUnknownec[0x30]; // +0xec
};
