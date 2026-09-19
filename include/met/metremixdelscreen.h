#pragma once

#include "met/listdataprovider.h"
#include "met/metmemcardpickeruser.h"
#include "met/metsaveremix.h"

/**
 * Screen that deletes a remix from a memory card.
 *
 * `17MetRemixDelScreen` in the RTTI descriptor at `0x008ef690`, with three public non-virtual
 * bases at fixed offsets, MetSaveRemix at `+0x00`, ListDataProvider at `+232`, and
 * MetMemCardPickerUser at `+236`. The object is at least 0x140 bytes.
 *
 * Four vtables belong to the class, the 43-entry primary at `0x008061d8`, the four-entry
 * ListDataProvider table at `0x008060e0` that adjusts `this` by `-232`, the three-entry MetKBUser
 * table at `0x00806108` that adjusts it by `-144`, and the 21-entry MemcardUser table at
 * `0x00806128` that adjusts it by `-140`. There is no fifth table, and the constructor writes no
 * vptr at `+0xec`, which is one of the four observations that prove MetMemCardPickerUser declares
 * no virtual function.
 *
 * The constructor at `0x003394a0` takes only the renderer and the load priority. It runs the
 * MetSaveRemix constructor at `0x00372120` with `mcrd` for the screen name, `metagame/Shared` for
 * the directory, and `memcard_remix_del` for the container, writes its four vptrs, zeroes `+0xf4`,
 * `+0xfc`, `+0x100`, and `+0x104`, writes `+0x108` and `+0x120`, zeroes `+0x138` and `+0x13c`,
 * clears MetScreen::mUnknown60, and pushes `mem_del_remix` into the container object-name vector
 * that MetScreen owns. It also addresses a nested object through `+0x0c`, `+0x10`, and `+0x14` of a
 * register the disassembly does not resolve to a member, so the span from `+0x10c` to `+0x137` is
 * recorded as reserved.
 *
 * The destructor at `0x003397a8` restores the four vptrs, restores the ListDataProvider vptr to
 * `0x007ec830`, runs the MetSaveRemix destructor, and releases the object with the tag `MsgSink`.
 *
 * Thirteen slots differ from the MetSaveRemix table. Slots 23 and 24 sit eight bytes apart at
 * `0x00343f18` and `0x00343f20` and are two-instruction `jr ra` stubs. Of the rest only the
 * destructor has a recovered name, and the others are 5 `0x0033a098`, 7 `0x00344078`,
 * 15 `0x0033b280`, 16 `0x003441a0`, 19 `0x00339b30`, 20 `0x00343f28`, 33 `0x00344058`,
 * 36 `0x0033ce00`, 38 `0x00339880`, 40 `0x0033e5c0`, 41 `0x0033e6d8`, and 42 `0x0033e7f0`.
 *
 * Slot 16 is otherwise empty in every class of the subsystem, and this screen is the only one that
 * fills it.
 */
class MetRemixDelScreen :
    public MetSaveRemix,
    public ListDataProvider,
    public MetMemCardPickerUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003394a0
     */
    MetRemixDelScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003397a8
     */
    virtual ~MetRemixDelScreen();

    /**
     * @ghidraAddress 0x00343f28
     */
    virtual void PlaySlideSound();

    /**
     * @ghidraAddress 0x00343f18
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x00343f20
     */
    virtual void PlayCycleRightSound();

private:
    int mUnknownf0;  // +0xf0, not written by the constructor
    int mUnknownf4;  // +0xf4
    int mUnknownf8;  // +0xf8, not written by the constructor
    int mUnknownfc;  // +0xfc
    int mUnknown100; // +0x100
    int mUnknown104; // +0x104
    int mUnknown108; // +0x108
    // Span the constructor addresses through a register the disassembly does not resolve to a
    // member. Not recovered.
    unsigned char mUnknown10c[0x2c]; // +0x10c
    int mUnknown138;                 // +0x138
    int mUnknown13c;                 // +0x13c
};
