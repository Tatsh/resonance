#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/player.h"

/**
 * Player driven by a controller on this machine.
 *
 * `LocalPlayer` in the RTTI descriptor at `0x008eeff8`, with `Player` as its only base. Its three
 * vtables are at `0x007d0160`, `0x007d0138`, and `0x007d0110`, each walked to its terminator.
 *
 * The primary table has **23 entries against the base's 21**, so this class adds two virtuals past
 * the end of the base table rather than overriding into it. It replaces slots 2 and 4 through 12
 * and 14 through 20, and inherits only slots 3 and 13. In the `MsgSource` table it replaces both
 * `AddSink` and `RemoveSink`, which the base leaves inherited, and in the `MsgSink` table it
 * replaces `HandleMessage`.
 *
 * Nine of its own members are recovered, eight being the values its accessor slots return and the
 * ninth the pair of message sources at `+0xa4` and `+0xa8`. AddSink() and RemoveSink() fan
 * registration out to both of those through slots 2 and 3 of a `MsgSource` table, and the vptr
 * each is read through sits at `+0x10` of the target, which is where `MsgSource` places its own.
 * The one at `+0xa8` also answers a slot 5, past the four entries a `MsgSource` table has, so its
 * dynamic type extends `MsgSource` the way this class extends `Player`. The destructor releases
 * that object by calling a virtual on it rather than by freeing it.
 *
 * A slot whose verb is unrecovered keeps its table index as its title, because the index is part
 * of the layout.
 */
class LocalPlayer : public Player {
public:
    /** @ghidraAddress 0x0011e348 */
    virtual ~LocalPlayer();

    /** @ghidraAddress 0x00121ea0 */
    virtual int Slot2();

    /** @ghidraAddress 0x00121e90 */
    virtual int Slot4();

    /** @ghidraAddress 0x00121e98 */
    virtual int Slot5();

    /** @ghidraAddress 0x00122890 */
    virtual int Slot6();

    /** @ghidraAddress 0x00121ea8 */
    virtual void Slot7();

    /** @ghidraAddress 0x00122898 */
    virtual void Slot8(int first, int second);

    /** @ghidraAddress 0x001228a8 */
    virtual int Slot9(int value);

    /** @ghidraAddress 0x00121eb0 */
    virtual int Slot10();

    /** @ghidraAddress 0x0011e4e0 */
    virtual void Slot11();

    /**
     * Slot 12. Forwards to slot 5 of the object at `+0xa8`.
     *
     * Not reconstructed. That object answers a slot past the four a `MsgSource` table has, so its
     * dynamic type extends `MsgSource` and is unidentified, and the call cannot be written through
     * a `MsgSource` pointer.
     *
     * @ghidraAddress 0x001228c8
     */
    virtual void Slot12();

    /** @ghidraAddress 0x00121ec0 */
    virtual int Slot14();

    /** @ghidraAddress 0x00121ed0 */
    virtual int Slot15();

    /** @ghidraAddress 0x00122ca0 */
    virtual int Slot16(int value);

    /** @ghidraAddress 0x00121ee0 */
    virtual int Slot17();

    /**
     * Report the proportion mCount9c is of the total it forms with mCounta0.
     *
     * Returns zero when mCount9c is zero, so the division never runs on an empty total.
     *
     * @return A fraction between 0 and 1.
     * @ghidraAddress 0x00122cc8
     */
    virtual float Slot18();

    /** @ghidraAddress 0x00121ee8 */
    virtual int Slot19();

    /** @ghidraAddress 0x00122c00 */
    virtual int Slot20(int value);

    /**
     * Slot 21. Declared by this class rather than inherited.
     *
     * Returns at once unless mMode68 is 2. Otherwise it stores its first argument in `+0x60` and
     * divides a field of its second by 0x780. Not reconstructed past that.
     *
     * @ghidraAddress 0x0011e810
     */
    virtual void Slot21(int value, int *pCounts);

    /**
     * Slot 22. Declared by this class rather than inherited.
     *
     * Returns at once unless mMode68 is 2. Otherwise it stores its argument in `+0x64` and
     * publishes a message through the `MsgSource` subobject. Not reconstructed, because the
     * message class behind the vptr at `0x007cf4e8` is unidentified.
     *
     * @ghidraAddress 0x0011e908
     */
    virtual void Slot22(int value);

    /** @ghidraAddress 0x0011ed98 */
    virtual void HandleMessage(Message *pMsg);

    /** @ghidraAddress 0x001228f8 */
    virtual void AddSink(MsgSink *pSink);

    /** @ghidraAddress 0x00122968 */
    virtual void RemoveSink(MsgSink *pSink);

private:
    int mUnknown50; // +0x50 returned by Slot2
    int mUnknown58; // +0x58 returned by Slot4
    int mUnknown5c; // +0x5c returned by Slot5
    int mUnknown60; // +0x60 written by Slot21 and returned by Slot10
    int mUnknown6c; // +0x6c returned by Slot19
    int mUnknown70; // +0x70 written by Slot8, compared by Slot9
    int mUnknown74; // +0x74 written by Slot8
    int mUnknown78; // +0x78 compared by Slot20
    int mUnknown7c; // +0x7c compared by Slot16
    int mUnknown88; // +0x88 returned by Slot17
    int mUnknown64; // +0x64 written by Slot22
    // Both Slot21 and Slot22 return at once unless this is 2, so it selects a mode.
    int mMode68;          // +0x68
    int mCount9c;         // +0x9c numerator of Slot18
    int mCounta0;         // +0xa0 the rest of Slot18's total
    int mUnknownac;       // +0xac returned plus one by Slot15
    int mUnknownb0;       // +0xb0 returned plus one by Slot14
    MsgSource *mSourceA4; // +0xa4
    MsgSource *mSourceA8; // +0xa8
};
