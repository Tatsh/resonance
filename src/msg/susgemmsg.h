#pragma once

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00902270`. It has Message as its one base. The object is 0x1c bytes
 * and its vtable is at `0x008124f8`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * A second, identical vtable at `0x007decf0` is emitted in the gem makers' unit, and
 * AxeNewGemMaker::PostGemMessages() stores it at `0x001a3094` and `0x001a3110`.
 *
 * The payload layout comes from the run of field copies in Clone(). Every member is public because
 * AppTunnel::HandleMessage() at `0x0044987c` reads them directly with no accessor in the image. It
 * copies the colour name of mPlayer, stops the strip mStripId when mStop is set, and otherwise
 * extends it on mLane to mFrame and mBlend.
 */
class SusGemMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 407.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7580
     * @ghidraAddress PAL: 0x0040f480
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x001a44d0
     * @ghidraAddress PAL: 0x001aa238
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nSusGemMsgType.
     * @ghidraAddress NTSC-U/C: 0x001a4540
     * @ghidraAddress PAL: 0x001aa2a8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `SusGemMsg`.
     * @ghidraAddress NTSC-U/C: 0x001a4550
     * @ghidraAddress PAL: 0x001aa2b8
     */
    virtual const char *GetName() const;

    int mStripId;              /*!< The sustain strip. +0x04 */
    int mStop;                 /*!< Non-zero to stop the strip. +0x08 */
    int mLane;                 /*!< The lane. +0x0c */
    int mFrame = kMBTInfinity; /*!< The frame the strip extends to. +0x10 */
    float mBlend;              /*!< The blend at that frame. +0x14 */
    Player *mPlayer;           /*!< The player the strip belongs to. +0x18 */
};

/**
 * Identity that SusGemMsg::Type() reports.
 *
 * This word belongs to SusGemMsg because SusGemMsg::Type() at `0x001a4540` returns it, and the
 * registration at `0x003d9818` passes the same value, 407, as the identity of this class's
 * factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d02dc
 * @ghidraAddress PAL: 0x00713a74
 */
extern int g_nSusGemMsgType;
