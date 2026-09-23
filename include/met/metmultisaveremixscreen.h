#pragma once

#include <vector>

#include "met/metremixsaver.h"
#include "met/metscreen.h"
#include "rnd/object.h"

/**
 * Dialogue that writes a multiplayer remix to a memory card.
 *
 * `23MetMultiSaveRemixScreen` in the RTTI descriptor at `0x008ef530`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00` and MetRemixSaver at `+140`. The 39-entry primary
 * vtable is at `0x007ffb90`, the same length as the MetScreen table, so the class declares no
 * virtual of its own, and the five-entry MetRemixSaver table at `0x007ffb60` adjusts `this` by
 * `-140` in every entry. That table is where this screen supplies the three MetRemixSaver pure
 * virtuals.
 *
 * The constructor at `0x002fa0b0` takes only the renderer and the load priority, and supplies
 * `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for the
 * container, the same container MetGlobalSettingsSaverScreen and MetRemixManager load.
 *
 * The constructor default-constructs the four vectors from `+0x90` through `+0xbf`, zeroes
 * mUnknownd4, and default-constructs a fifth vector at `+0xd8`. It never writes the span from
 * `+0xc0` through `+0xd3`, which is recorded as reserved. The object is at least 0xe4 bytes, from
 * the end of the fifth vector.
 *
 * The destructor at `0x002fa280` tears down the fifth vector first and then the other four in
 * reverse declaration order, restores the MetRemixSaver vptr to `0x007ffcd0`, runs the MetScreen
 * destructor, and releases the object with the tag `MsgSink`. Every part of that teardown is
 * compiler-generated member destruction, so no destructor body is reconstructed. The constructor
 * likewise supplies only the three names and the one zeroed word.
 *
 * Eight slots differ from the MetScreen table. Slots 21 through 24 sit eight bytes apart at
 * `0x002feda0` through `0x002fedb8`, and the declaration order there puts the cycle sounds ahead
 * of high and leave, so the addresses do not ascend with the slot numbers. Of the rest only the
 * destructor has a recovered name, and the others are 5 `0x002fa4e8` and 36 `0x002fa7c0`.
 */
class MetMultiSaveRemixScreen : public MetScreen, public MetRemixSaver {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002fa0b0
     */
    MetMultiSaveRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002fa280
     */
    virtual ~MetMultiSaveRemixScreen();

    /**
     * Silence the leave sound.
     *
     * All four overrides are two-instruction stubs, so each was written inline with an empty body.
     *
     * @ghidraAddress 0x002fedb8
     */
    virtual void PlayLeaveSound(int) {
    }

    /**
     * Silence the high sound.
     *
     * @ghidraAddress 0x002fedb0
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Silence the cycle-left sound.
     *
     * @ghidraAddress 0x002feda0
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x002feda8
     */
    virtual void PlayCycleRightSound(int) {
    }

private:
    std::vector<Rnd::Object *> mUnknown90; // +0x90
    std::vector<Rnd::Object *> mUnknown9c; // +0x9c
    std::vector<Rnd::Object *> mUnknowna8; // +0xa8
    std::vector<Rnd::Object *> mUnknownb4; // +0xb4
    // Never written by the constructor and not recovered. +0xc0
    unsigned char mUnknownc0[0x14];
    int mUnknownd4;                        // +0xd4
    std::vector<Rnd::Object *> mUnknownd8; // +0xd8
};
