#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `16ChoosePowerupMsg` in the RTTI descriptor at `0x008ef810`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x00812c48`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e3ce0`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class ChoosePowerupMsg : public Message {
public:
    /**
     * Report a new selection in a player's collection.
     *
     * No address attaches to the constructor. The two collections build the message on their own
     * stack at five sites, `0x001cb3e8`, `0x001cb494`, `0x001cb6e0`, `0x001cb8d4`, and
     * `0x001cb98c`, and hand the address to MsgSource::Send(). The compiler expands the
     * constructor into each site. The three arguments are the three members in declaration order.
     *
     * @param nIndex The selected entry, or 0 from SinglePowerupCollection, which stores one.
     * @param pOwner The player whose selection changed.
     * @param nType The selected powerup's kind, or -1 for none.
     */
    ChoosePowerupMsg(int nIndex, Player *pOwner, int nType);

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dd0b8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nChoosePowerupMsgType.
     * @ghidraAddress 0x003dd110
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ChoosePowerupMsg`.
     * @ghidraAddress 0x003dd120
     */
    virtual const char *Name();

private:
    // The three names come from the two collections, the only producers of the message, which
    // write the selected entry, the owning player, and the selected kind into them in this order.
    int mIndex;     // +0x04
    Player *mOwner; // +0x08
    int mType;      // +0x0c
};

/**
 * Identity that ChoosePowerupMsg::Type() reports.
 *
 * This word belongs to ChoosePowerupMsg because ChoosePowerupMsg::Type() at `0x003dd110` returns
 * it. Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d020c
 */
extern int g_nChoosePowerupMsgType;
