#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "msg/message.h"
#include "msg/neutralizemsg.h"

/**
 * Neutraliser of the phrases a catching track has already captured.
 *
 * `17PhraseNeutralizer` in the RTTI descriptor at `0x008ff3f0`, over MsgSource at offset 0 and
 * MsgSink at offset 20. Its primary table is at `0x007e2e88` with four entries and its MsgSink
 * subobject table at `0x007e2e60`, which adjusts `this` by `-20` and fills slot 3 with
 * HandleMessage(). MsgSource coming first is why CatchingSTG's wiring adjusts by `+0x14` when it
 * registers this object as a sink and guards the adjustment against a null pointer. The object is
 * 0x3c bytes, which CatchingSTG's tagged allocation measures.
 *
 * The destructor at `0x001c1478` is implicitly declared. It restores the base tables, frees
 * MsgSource's vector, and releases the object under MsgSink's tag.
 */
class PhraseNeutralizer : public MsgSource, public MsgSink {
public:
    /**
     * @param pTrackData The track description. The constructor copies its track number.
     * @param pPhraseMgr The phrase manager of the track.
     * @ghidraAddress 0x001c0918
     */
    PhraseNeutralizer(const TrackData *pTrackData, PhraseMgr *pPhraseMgr);

    /**
     * Act on a message.
     *
     * Slot 3 of the MsgSink table. A NeutralizeMsg goes to PostTrackNeutralizedMsg(), and every
     * other message is discarded.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001c1700
     */
    virtual void HandleMessage(Message *pMsg);

private:
    // Clears the owned phrases of the four bars after the message's bar on this track. Each owner
    // loses the phrase's value from its score through the Player routine at 0x0012f808. When a
    // phrase was cleared, the message is marked handled, a DeployedPowerupMsg covering the four
    // bars is sent, and a PlayersTrackNeutralizedMsg follows for each player that lost points.
    // 0x001c0980
    void PostTrackNeutralizedMsg(NeutralizeMsg *pMsg);

    int mUnknown18[5];           // +0x18, not written by any recovered routine
    int mTrack;                  // +0x2c, copied from TrackData::mUnknown04
    PhraseMgr *mPhraseMgr;       // +0x30
    int mUnknown34;              // +0x34, not written by any recovered routine
    const TrackData *mTrackData; // +0x38
};
