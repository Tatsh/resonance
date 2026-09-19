#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `15PowerupCountMsg` in the RTTI descriptor at `0x00901d10`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x00812c00`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e3d08`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class PowerupCountMsg : public Message {
public:
    /**
     * Report a new stored count for one entry of a player's collection.
     *
     * No address attaches to the constructor. PowerupCollection builds the message on its own
     * stack at `0x001cb280`, `0x001cb554`, and `0x001cb658` and hands the address to
     * MsgSource::Send(), and the compiler expands the constructor into each of the three sites.
     * The three arguments are the three members in declaration order.
     *
     * @param nIndex The entry, as its index into the collection.
     * @param nCount The stored count after the change.
     * @param pOwner The player whose collection changed.
     */
    PowerupCountMsg(int nIndex, int nCount, Player *pOwner);

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dd270
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPowerupCountMsgType.
     * @ghidraAddress 0x003dd2c8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PowerupCountMsg`.
     * @ghidraAddress 0x003dd2d8
     */
    virtual const char *Name();

private:
    // The three names come from PowerupCollection, the one producer of the message, which writes
    // the entry index, the count after the change, and the owning player into them in this order.
    int mIndex;     // +0x04
    int mCount;     // +0x08
    Player *mOwner; // +0x0c
};

/**
 * Identity that PowerupCountMsg::Type() reports.
 *
 * This word belongs to PowerupCountMsg because PowerupCountMsg::Type() at `0x003dd2c8` returns
 * it. Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0214
 */
extern int g_nPowerupCountMsgType;
