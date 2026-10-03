#pragma once

#include "app/hudutil.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008eeca8`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x00812c90`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). Both members are public
 * because Overlay::OnCaughtPowerbar() at `0x0041eba0` reads them directly with no accessor in the
 * image. It compares mPlayer with HudTrack::mPlayer and passes mKind to HudPowerupName() at
 * `0x0041ebf4`. That call types mKind.
 *
 * The destructor at `0x003dce20` is compiler-generated and has no declaration here.
 */
class CaughtPowerbarMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6f58
     * @ghidraAddress PAL: 0x0040ee48
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dcf10
     * @ghidraAddress PAL: 0x00415348
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nCaughtPowerbarMsgType.
     * @ghidraAddress NTSC-U/C: 0x003dcf60
     * @ghidraAddress PAL: 0x00415398
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `CaughtPowerbarMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dcf70
     * @ghidraAddress PAL: 0x004153a8
     */
    virtual const char *GetName() const;

    /**
     * Write the item kind to a diagnostic stream as a number.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3cb8
     * @ghidraAddress PAL: 0x0041be88
     */
    virtual void PrintExtra(std::ostream &stream) const;

    PowerupType mKind; /*!< The captured item. +0x04 */
    Player *mPlayer;   /*!< The capturing player. +0x08 */
};

/**
 * Identity that CaughtPowerbarMsg::Type() reports.
 *
 * This word belongs to CaughtPowerbarMsg because CaughtPowerbarMsg::Type() at `0x003dcf60`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0204
 * @ghidraAddress PAL: 0x0071399c
 */
extern int g_nCaughtPowerbarMsgType;
