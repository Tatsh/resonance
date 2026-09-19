#pragma once

#include "met/metbuttonlist.h"
#include "met/metsaveremix.h"
#include "os/hxstr.h"

/**
 * Solo screen that writes a finished remix to a memory card.
 *
 * `18MetSaveRemixScreen` in the RTTI descriptor at `0x008ef8c0`, with MetSaveRemix as its one
 * public non-virtual base at offset 0. Its own members start at `+0xe8`, which fixes the size of
 * MetSaveRemix, and the object is at least 0x10c bytes. Three vtables belong to the class, the
 * 43-entry primary at `0x0080a690`, the 21-entry MemcardUser table at `0x0080a5e0` that adjusts
 * `this` by `-140`, and the three-entry MetKBUser table at `0x0080a5c0` that adjusts it by `-144`.
 * The primary is the same length as the MetSaveRemix table, so the class declares no virtual of
 * its own.
 *
 * The constructor at `0x0037ace0` takes only the renderer and the load priority. It runs the
 * MetSaveRemix constructor at `0x00372120` with `ers` for the screen name, `metagame/_Solo` for
 * the directory, and `save_remix` for the container, writes its own three vptrs, zeroes
 * mUnknowne8, mUnknownec, and mUnknown100, default-constructs mUnknown104, allocates a
 * MetButtonList tagged `MetButtonList` into mUnknowne8, and pushes `remix_save` into the
 * container object-name vector MetScreen declares at `+0x38`. It then clears
 * MetScreen::mUnknown5c, which is why MetScreen::mUnknown5c is protected rather than private, and
 * MetSaveRemix::mUnknowne0, which requires that MetSaveRemix member to be protected as well. The
 * image emits the mUnknown5c store on both paths of the temporary release above it, which is one
 * source statement rather than two.
 *
 * The destructor at `0x00381868` restores the three vptrs, deletes mUnknowne8 through slot 1 of
 * the MetButtonList table with the deleting `__in_chrg` value, releases the mUnknown104 buffer
 * through the inlined HxStr destructor, runs the MetSaveRemix destructor, and releases the object
 * with the tag `MsgSink`.
 *
 * Thirteen slots differ from the MetSaveRemix table. Slots 21 through 24 sit eight bytes apart at
 * `0x003817c0` through `0x003817d8` and are two-instruction `jr ra` stubs, and the declaration
 * order there puts leave before high, so the addresses do not ascend with the slot numbers. Of the
 * rest only the destructor has a recovered name, and the others are 5 `0x0037b8e8`,
 * 7 `0x0037b718`, 15 `0x0037cac8`, 19 `0x0037b258`, 20 `0x00381968`, 30 `0x0037c110`,
 * 36 `0x0037c260`, 38 `0x0037af98`, 40 `0x003819f0`, 41 `0x00381a28`, and 42 `0x0037ccc8`.
 */
class MetSaveRemixScreen : public MetSaveRemix {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0037ace0
     */
    MetSaveRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00381868
     */
    virtual ~MetSaveRemixScreen();

    /**
     * Play the slide sound when the selector matches MetSaveRemix::mUnknownc8.
     *
     * This is the one sound override of the class that is not an empty stub, and the only one in
     * the band that genuinely compares the selector against a recorded value.
     *
     * @param nSelector Compared against MetSaveRemix::mUnknownc8, then passed through unchanged.
     * @ghidraAddress 0x00381968
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Silence the leave sound.
     *
     * The four overrides below are two-instruction stubs, so each was written inline with an empty
     * body. Their declaration order puts leave before high, which is why their addresses do not
     * ascend with the slot numbers.
     *
     * @ghidraAddress 0x003817d8
     */
    virtual void PlayLeaveSound() {
    }

    /**
     * Silence the high sound.
     *
     * @ghidraAddress 0x003817c0
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Silence the cycle-left sound.
     *
     * @ghidraAddress 0x003817c8
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x003817d0
     */
    virtual void PlayCycleRightSound(int) {
    }

private:
    MetButtonList *mUnknowne8; // +0xe8
    int mUnknownec;            // +0xec
    // Never written by the constructor and not recovered.
    unsigned char mUnknownf0[0x10]; // +0xf0
    int mUnknown100;                // +0x100
    // Default-constructed, and its buffer is what the destructor releases at `+0x108`. +0x104
    HxStr mUnknown104;
};
