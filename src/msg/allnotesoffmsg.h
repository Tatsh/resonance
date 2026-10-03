#pragma once

#include "msg/musemsg.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008effe0`. It has MuseMsg as its one base. The object is 0x8 bytes
 * and its vtable is at `0x007dd438`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The fields through `+0x07` belong to MuseMsg and are declared there.
 *
 * This class adds no field of its own, and the size is exactly the size of the base, which is what
 * fixes the size of the base.
 *
 * The table has nine entries and a zero terminator at index 9, one more than MuseMsg's. The class
 * introduces one virtual at slot 8. A second emission of the same table at `0x00812e40`
 * agrees entry for entry.
 *
 * The destructor at `0x0019a538` is compiler-generated and has no declaration here.
 */
class AllNotesOffMsg : public MuseMsg {
public:
    /**
     * Construct a message with the song position at kMBTInfinity.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    AllNotesOffMsg() {
    }

    /**
     * Construct a message at a song position.
     *
     * Inline, with no address of its own. AutoRiffer::OnStopRiff() at `0x001993c4` expands it on
     * its stack, storing only the position.
     *
     * @param nTick The song position, in MIDI ticks.
     */
    explicit AllNotesOffMsg(int nTick) : MuseMsg(nTick) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 203.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6dc8
     * @ghidraAddress PAL: 0x0040ecb8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x0019a618
     * @ghidraAddress PAL: 0x001a0380
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_dwAllNotesOffMsgType.
     * @ghidraAddress NTSC-U/C: 0x0019a670
     * @ghidraAddress PAL: 0x001a03d8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AllNotesOffMsg`.
     * @ghidraAddress NTSC-U/C: 0x0019a680
     * @ghidraAddress PAL: 0x001a03e8
     */
    virtual const char *GetName() const;

    /**
     * Report that this message is an all-notes-off message. Slot 8, the one virtual this class
     * introduces.
     *
     * The body returns 1. The image records no direct caller. The title is inferred from a
     * constant result on a virtual only this class declares.
     *
     * @return 1.
     * @ghidraAddress NTSC-U/C: 0x0019a690
     * @ghidraAddress PAL: 0x001a03f8
     */
    virtual int IsAllNotesOff();
};

/**
 * Identity that AllNotesOffMsg::Type() reports.
 *
 * This word belongs to AllNotesOffMsg because AllNotesOffMsg::Type() at `0x0019a670` returns it,
 * and the registration at `0x003d9818` passes the same value, 203, as the identity of this class's
 * factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d01d4
 * @ghidraAddress PAL: 0x0071396c
 */
extern unsigned int g_dwAllNotesOffMsgType;
