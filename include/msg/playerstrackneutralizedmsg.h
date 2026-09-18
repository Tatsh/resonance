#pragma once

#include "msg/cmdmsg.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `26PlayersTrackNeutralizedMsg` in the RTTI descriptor at `0x008eed28`, with CmdMsg as its one
 * base. The object is 0x10 bytes and its vtable is at `0x00812270`. The members below are the
 * whole of the class: everything recovered comes from them, and no other routine in the image
 * refers to this type by anything but its vtable. The fields through `+0x0f` belong to CmdMsg and
 * are declared there.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * Clone() copies only as far as `0x4` of the 0x10 bytes it allocates, so the remaining 12 are
 * either alignment padding or a field the copy omits.
 *
 * The class overrides Message::Print() at `0x003e3f30`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class PlayersTrackNeutralizedMsg : public CmdMsg {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e05b0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPlayersTrackNeutralizedMsgType.
     * @ghidraAddress 0x003e0618
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PlayersTrackNeutralizedMsg`.
     * @ghidraAddress 0x003e0628
     */
    virtual const char *Name();
};

/**
 * Identity that PlayersTrackNeutralizedMsg::Type() reports.
 *
 * This word belongs to PlayersTrackNeutralizedMsg because PlayersTrackNeutralizedMsg::Type() at
 * `0x003e0618` returns it. Several handlers elsewhere read the same word to compare against it,
 * which is the expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0324
 */
extern int g_nPlayersTrackNeutralizedMsgType;
