#pragma once

#include "app/hudutil.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `18DeployedPowerupMsg` in the RTTI descriptor at `0x008ef7c0`, with Message as its one base. The
 * object is 0x1c bytes and its vtable is at `0x007d0080`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). The first three words are
 * public because Overlay::OnDeployedPowerup() at `0x0041eda8` reads them directly with no accessor
 * in the image. It passes mKind to HudPowerupName(), compares mPlayer with HudTrack::mPlayer, and,
 * when mKind is kHudItemBumper, compares mTarget with HudTrack::mPlayer as well. The last three
 * words are public because AppTunnel's powerup handler at `0x00448d58` reads them directly. It
 * scales mFirstBar by the 1920 ticks of a bar, walks mBarCount bars from it, and matches mTrack
 * against each tunnel item's track.
 */
class DeployedPowerupMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 306.
     *
     * @return The message.
     * @ghidraAddress 0x003d7000
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00122290
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nDeployedPowerupMsgType.
     * @ghidraAddress 0x00122300
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `DeployedPowerupMsg`.
     * @ghidraAddress 0x00122310
     */
    virtual const char *Name();

    HudItemKind mKind; /*!< The deployed powerup. +0x04 */
    Player *mPlayer;   /*!< The deploying player. +0x08 */
    Player *mTarget;   /*!< The player a bumper strikes. +0x0c */
    int mFirstBar;     /*!< The first bar the powerup covers. +0x10 */
    int mBarCount;     /*!< The number of bars it covers. +0x14 */
    int mTrack;        /*!< The track it acts on. +0x18 */
};

/**
 * Identity that DeployedPowerupMsg::Type() reports.
 *
 * This word belongs to DeployedPowerupMsg because DeployedPowerupMsg::Type() at `0x00122300`
 * returns it, and the registration at `0x003d9818` passes the same value, 306, as the identity of
 * this class's factory.
 *
 * @ghidraAddress 0x006d021c
 */
extern int g_nDeployedPowerupMsgType;
