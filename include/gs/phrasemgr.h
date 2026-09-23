#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

class PhraseDatabase;
class PlayMap;
class TrackData;

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Owner of the gem phrases on one track, and the seam between the network and the gems.
 *
 * `9PhraseMgr` in the RTTI descriptor at `0x00901fd0`, with MsgSink at offset 0 and MsgSource at
 * offset 4, and titled after `GsPhraseMgr.cpp`, the translation unit its two file-local classes
 * record. Those two are `Cmd` at `0x008f0960` and `ExportCmd` at `0x009021f0`, both deriving from
 * Sch::Command. Its primary table is at `0x007e28d0` and its MsgSource table at `0x007e28a8`, and
 * each runs four entries. ScoreTrackGraph's tagged allocation measures the object at 0x60 bytes.
 * The mangled signature of `Catcher::Catcher()`,
 * `__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick`, also records the
 * title, and every stage passes the manager it retains to the catcher it builds.
 *
 * Every member below posts one message through its own MsgSource half, which is what the titles
 * describe. PostPhraseMsg() looks the phrase up through PhraseDatabase::GetPhraseAt() on
 * mDatabase, and when that lookup reports a phrase it builds a PhraseMsg with the argument at
 * `+0x04`, mUnknown30 at `+0x08`, and the phrase at `+0x0c`, and delivers it through
 * MsgSource::Send().
 *
 * HandleMessage() dispatches six identities, three of them packets rather than messages. A
 * PhrasePacket, a CaughtPhrasePacket, and a GemPacket arrive from the network, and an
 * InvalidateTrackMsg, a RefreshNetMsg, and a GameBeginMsg arrive locally. The PhrasePacket and the
 * InvalidateTrackMsg paths both loop, clearing gems through PostClearGemsMsg() as they go, and the
 * PhrasePacket path is guarded on the packet's `+0x14` matching mUnknown30.
 *
 * The constructor fixes the member map from `+0x18` to the end. The MsgSource subobject occupies
 * `+0x04` through `+0x17`, so the region at `+0x10` the destructor tears down is that subobject's
 * vector rather than a member of this class. The destructor deletes mDatabase and mUnknown2c
 * through slot 1 of each table, which is the destructor slot.
 */
class PhraseMgr : public MsgSink, public MsgSource {
    // ScoreTrackGraph::GetPhraseDatabase() at 0x001cf978 reads mDatabase directly.
    friend class ScoreTrackGraph;

public:
    /**
     * Construct the manager of one track and its phrase database.
     *
     * The fifth argument arrives in `t1`. ScoreTrackGraph passes the song clock, 1920, the play
     * map Globals reports, a configuration value, and its track description. The body is not
     * written, because it ends in the routine at `0x001ba3d8`, which is not recovered.
     *
     * @param pClock The song clock.
     * @param nBarTicks The length of one bar in MIDI ticks.
     * @param pMap The play map the phrase database is sized from.
     * @param nConfig The configuration value ScoreTrackGraph reads from the query at
     *                `0x00509110` with the identifier 0x2be.
     * @param pTrackData The track description.
     * @ghidraAddress 0x001ba0d0
     */
    PhraseMgr(Sch::TickClock *pClock,
              int nBarTicks,
              PlayMap *pMap,
              int nConfig,
              const TrackData *pTrackData);

    /**
     * Delete both owned objects.
     *
     * The body is not written. It runs the routine at `0x001c0450` first.
     *
     * @ghidraAddress 0x001ba2b8
     */
    virtual ~PhraseMgr();

    /**
     * Post a PhraseMsg for one phrase.
     *
     * Performs no work when the lookup at `0x001b8e50` reports nothing.
     *
     * @param nPhrase The value the message's `+0x04` receives.
     * @ghidraAddress 0x001bc468
     */
    void PostPhraseMsg(int nPhrase);

    /**
     * Post a GemMsg. The body is not written.
     *
     * The GemPacket path of HandleMessage() is the one recovered caller.
     *
     * @param pMsg The packet the gem is read out of.
     * @ghidraAddress 0x001ba6d0
     */
    void PostGemMsg(Message *pMsg);

    /**
     * Post a second form of GemMsg. The body is not written.
     *
     * No caller is recovered. The address belongs to this class and the program already titles it.
     *
     * @ghidraAddress 0x001bc0f0
     */
    void PostGemMsgSecond();

    /**
     * Post a third form of GemMsg. The body is not written.
     *
     * No caller is recovered.
     *
     * @ghidraAddress 0x001bc290
     */
    void PostGemMsgThird();

    /**
     * Post a DurGemMsg. The body is not written.
     *
     * No caller is recovered. At 0x2ac bytes it is the largest routine of the class.
     *
     * @ghidraAddress 0x001bbcf0
     */
    void PostDurGemMsg();

    /**
     * Post a ClearGemsMsg. The body is not written.
     *
     * Three loops inside HandleMessage() reach it.
     *
     * @ghidraAddress 0x001bbb90
     */
    void PostClearGemsMsg();

    /**
     * Post a BarStatusMsg. The body is not written.
     *
     * No caller is recovered.
     *
     * @ghidraAddress 0x001bb9f8
     */
    void PostBarStatusMsg();

    /**
     * Post a CaughtPhrasePacket. The body is not written.
     *
     * The CaughtPhrasePacket path of HandleMessage() is the one recovered caller.
     *
     * @param pMsg The packet.
     * @ghidraAddress 0x001ba928
     */
    void PostCaughtPhrasePacket(Message *pMsg);

protected:
    /**
     * Act on a message.
     *
     * Primary table slot 3. The body is not written.
     *
     * @param pMsg The message or packet.
     * @ghidraAddress 0x001bc718
     */
    virtual void HandleMessage(Message *pMsg);

private:
    int mUnknown18; // +0x18, cleared on construction

public:
    /**
     * Word the stage classes install through their table slot 8.
     *
     * The constructor clears it and every one of the four stages writes it from outside the class,
     * which is what records the member public. A friend declaration on the stage classes fits the
     * image equally well.
     *
     * +0x1c
     */
    int mUnknown1c;

private:
    const TrackData *mTrackData; // +0x20
    // Created by the constructor over mMap, deleted by the destructor, and the object
    // PostPhraseMsg() and three HandleMessage() paths look a phrase up in.
    PhraseDatabase *mDatabase; // +0x24
    PlayMap *mMap;             // +0x28
    // Cleared on construction and deleted by the destructor through slot 1 of the table at its
    // `+0x10`, which is the MsgSource position. Its class is unrecovered.
    MsgSource *mUnknown2c; // +0x2c
    // Copied from the track description's `+0x04`. The track this manager serves. A PhrasePacket's
    // `+0x14` is matched against it, and PostPhraseMsg() copies it into the message's `+0x08`.
    int mUnknown30;         // +0x30
    int mBarTicks;          // +0x34
    int mConfig;            // +0x38
    int mUnknown3c;         // +0x3c
    int mUnknown40;         // +0x40
    int mUnknown44;         // +0x44
    int mUnknown48;         // +0x48
    Sch::TickClock *mClock; // +0x4c
    int mUnknown50;         // +0x50, starts at kIDableUnregistered
    int mUnknown54;         // +0x54, starts at kIDableUnregistered
    int mUnknown58;         // +0x58, copied from the track description's `+0x0c`
    int mPlayMode;          // +0x5c, the play mode Globals reported at construction
};
