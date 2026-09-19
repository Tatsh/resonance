#pragma once

#include <iostream.h>

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/idable.h"

/**
 * One participant in a session, local or remote.
 *
 * `Player` in the RTTI descriptor at `0x009020c0`, over three bases: `IDable<Player>` at offset 0,
 * `MsgSink` at 8, and `MsgSource` at 12. Three classes derive from it, `LocalPlayer`, `NetPlayer`,
 * and `NullPlayer`.
 *
 * Three vtables belong to it, each walked to its all-zero terminator rather than counted from the
 * slot titles, which understate every one of them. The primary at `0x007d2170` has 21 entries, so
 * this class declares 19 virtuals of its own after slot 0 and the destructor. The `MsgSink` table
 * at `0x007d2148` has 4 and overrides only `HandleMessage`, and the `MsgSource` table at
 * `0x007d2120` has 4 and overrides nothing, leaving `AddSink` and `RemoveSink` as inherited.
 *
 * Almost all of the primary table is inert defaults: slots 2 and 4 return -1, six return 0, two
 * return 1, one returns 0.0f, and five are empty. Only slots 11 and 13 have a body of any size, so
 * this class is an interface with defaults and the three subclasses carry the behaviour.
 *
 * The base subobjects account for `+0x00` through `+0x1f`, which the destructor at `0x00132ae8`
 * confirms by restoring a vptr at `+0x04` for `IDable<Player>`, at `+0x08` for `MsgSink`, and at
 * `+0x1c` for `MsgSource`, whose own `mSinks` vector it tears down at `+0x10`. This class's own
 * members start at `+0x20`, and the only one the destructor releases is the pointer at `+0x28`.
 *
 * A slot whose verb is unrecovered keeps its table index as its title, because the index is part
 * of the layout. The comment records the behaviour recovered instead.
 */
class Player : public IDable<Player>, public MsgSink, public MsgSource {
public:
    /** @ghidraAddress 0x00132ae8 */
    virtual ~Player();

    /**
     * Slot 2. Returns -1. The verb is unrecovered.
     *
     * @ghidraAddress 0x00132c20
     */
    virtual int Slot2();

    /**
     * Test whether this player is the stand-in for an absent one.
     *
     * Returns false here. Only `NullPlayer` returns true, and Print() branches on the answer to
     * write "{player null}", which is what recovers the verb.
     *
     * @return Non-zero when this player is a stand-in.
     * @ghidraAddress 0x00132c60
     */
    virtual int IsNull();

    /**
     * Slot 4. Returns -1.
     *
     * @ghidraAddress 0x00132c98
     */
    virtual int Slot4();

    /**
     * Slot 5. Returns zero.
     *
     * @ghidraAddress 0x00132ca0
     */
    virtual int Slot5();

    /**
     * Slot 6. Returns zero.
     *
     * @ghidraAddress 0x00132ca8
     */
    virtual int Slot6();

    /**
     * Slot 7. Empty.
     *
     * @ghidraAddress 0x00132cb0
     */
    virtual void Slot7();

    /**
     * Slot 8. Empty here, so it ignores both arguments.
     *
     * The parameters are proven by `LocalPlayer::Slot8`, which stores the first at `+0x70` and the
     * second at `+0x74`. An empty base body reveals nothing about its own parameter list.
     *
     * @ghidraAddress 0x00132cb8
     */
    virtual void Slot8(int first, int second);

    /**
     * Slot 9. Returns zero, ignoring its argument.
     *
     * The parameter is proven by `LocalPlayer::Slot9`, which compares it against `+0x70`.
     *
     * @ghidraAddress 0x00132cc0
     */
    virtual int Slot9(int value);

    /**
     * Slot 10. Returns zero.
     *
     * @ghidraAddress 0x00132cc8
     */
    virtual int Slot10();

    /**
     * Slot 11. Publishes a message through the `MsgSource` subobject.
     *
     * Builds the message on the stack from this player and the value at `+0x34` clamped to a
     * maximum of 800, then sends it with `MsgSource::Send`. The verb is unrecovered.
     *
     * @ghidraAddress 0x0012f788
     */
    virtual void Slot11();

    /** @ghidraAddress 0x00132d30 */
    virtual void Slot12();

    /**
     * Write a description of this player to stream.
     *
     * Writes the literal "{player null}" when IsNull() reports true, and otherwise "{player "
     * followed by the identifier at `+0x20`. Those two literals at `0x007d2048` and `0x007d2058`
     * are what attest both this routine and IsNull().
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00133110
     */
    virtual void Print(ostream &stream);

    /**
     * Slot 14. Leaves the return register untouched, so the value it yields is indeterminate.
     *
     * The return type is proven by `LocalPlayer::Slot14`, which computes `+0xb0` plus one into it.
     * A base whose default yields an indeterminate value is faithful to the image rather than a
     * reconstruction error, and it means the base is not meant to be called.
     *
     * @ghidraAddress 0x00132d70
     */
    virtual int Slot14();

    /**
     * Slot 15. Leaves the return register untouched, as Slot14 does.
     *
     * The return type is proven by `LocalPlayer::Slot15`, which computes `+0xac` plus one into it.
     *
     * @ghidraAddress 0x00132d78
     */
    virtual int Slot15();

    /**
     * Slot 16. Returns 1, ignoring its argument.
     *
     * The parameter is proven by `LocalPlayer::Slot16`, which compares it against `+0x7c`.
     *
     * @ghidraAddress 0x00132d80
     */
    virtual int Slot16(int value);

    /**
     * Slot 17. Returns zero.
     *
     * @ghidraAddress 0x00132d88
     */
    virtual int Slot17();

    /**
     * Slot 18. Returns 0.0f, which is what fixes the return type.
     *
     * @ghidraAddress 0x00132d90
     */
    virtual float Slot18();

    /**
     * Slot 19. Returns zero.
     *
     * @ghidraAddress 0x00132da0
     */
    virtual int Slot19();

    /**
     * Slot 20. Returns 1, ignoring its argument.
     *
     * The parameter is proven by `LocalPlayer::Slot20`, which compares it against `+0x78`.
     *
     * @ghidraAddress 0x00132db0
     */
    virtual int Slot20(int value);

    /**
     * Receive one message.
     *
     * The only `MsgSink` virtual this class overrides, at slot 3 of its `MsgSink` table.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00133240
     */
    virtual void HandleMessage(Message *pMsg);

protected:
    // Declared in recovered offset order. The base subobjects occupy +0x00 through +0x1f.
    // Printed by Print() after the "{player " literal, and compared against a field of an
    // incoming message by HandleMessage, so this is the identifier a message addresses.
    int mId20;      // +0x20
    int mUnknown24; // +0x24
    // Released by ~Player, so this member owns its allocation.
    void *mUnknown28; // +0x28
};
