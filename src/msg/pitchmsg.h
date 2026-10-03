#pragma once

#include "mid/tick.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901af0`. It has Message as its one base. The object is 0x14 bytes
 * and its vtable is at `0x007e5560`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). The last three words are
 * public because AppTunnel's pitch handler at `0x00447cc0` reads them directly with no accessor in
 * the image. It compares `+0x10` with the player each tunnel item stores, which types it.
 *
 * A second, identical vtable at `0x007e1320` is emitted in NotePitcher's unit, and
 * NotePitcher::PostPitchMsg() stores it at `0x001b2024`.
 *
 * The destructor at `0x001b3890` is compiler-generated and has no declaration here.
 */
class PitchMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 409.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7600
     * @ghidraAddress PAL: 0x0040f500
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x001b3940
     * @ghidraAddress PAL: 0x001b9718
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPitchMsgType.
     * @ghidraAddress NTSC-U/C: 0x001b39a0
     * @ghidraAddress PAL: 0x001b9778
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PitchMsg`.
     * @ghidraAddress NTSC-U/C: 0x001b39b0
     * @ghidraAddress PAL: 0x001b9788
     */
    virtual const char *GetName() const;

    /**
     * The quantised song position, in MIDI ticks. +0x04
     *
     * Public because NotePitcher::PostPitchMsg() at `0x001b1f10` writes it directly.
     */
    int mTick = kTickInfinity;

    int mTrack;      /*!< The track. AppTunnel looks up the tunnel ring for it. +0x08 */
    int mGem;        /*!< The gem. AppTunnel converts it to a lane blend and flashes it. +0x0c */
    Player *mPlayer; /*!< The player. +0x10 */
};

/**
 * Identity that PitchMsg::Type() reports.
 *
 * This word belongs to PitchMsg because PitchMsg::Type() at `0x001b39a0` returns it, and the
 * registration at `0x003d9818` passes the same value, 409, as the identity of this class's
 * factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d02ec
 * @ghidraAddress PAL: 0x00713a84
 */
extern int g_nPitchMsgType;
