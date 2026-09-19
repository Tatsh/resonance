#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

/**
 * Owner of the gem phrases on one track, and the seam between the network and the gems.
 *
 * `9PhraseMgr` in the RTTI descriptor at `0x00901fd0`, with MsgSink at offset 0 and MsgSource at
 * offset 4, and titled after `GsPhraseMgr.cpp`, the translation unit its two file-local classes
 * record. Those two are `Cmd` at `0x008f0960` and `ExportCmd` at `0x009021f0`, both deriving from
 * Sch::Command. Its primary table is at `0x007e28d0` and its MsgSource table at `0x007e28a8`, and
 * each runs four entries.
 *
 * Every member below posts one message through its own MsgSource half, which is what the titles
 * describe. PostPhraseMsg() is the one body recovered in full. It looks the phrase up by calling
 * `0x001b8e50` on mUnknown24 with its argument, and when that lookup reports a phrase it builds a
 * PhraseMsg with the argument at `+0x04`, mUnknown30 at `+0x08`, and the looked-up phrase at
 * `+0x0c`, and delivers it through MsgSource::Deliver(). No body is written, because the class
 * that declares `0x001b8e50` is unrecovered and titling the lookup would invent a member on an
 * unidentified class.
 *
 * HandleMessage() dispatches six identities, three of them packets rather than messages. A
 * PhrasePacket, a CaughtPhrasePacket, and a GemPacket arrive from the network, and an
 * InvalidateTrackMsg, a RefreshNetMsg, and a GameBeginMsg arrive locally. The PhrasePacket and the
 * InvalidateTrackMsg paths both loop, clearing gems through PostClearGemsMsg() as they go, and the
 * PhrasePacket path is guarded on the packet's `+0x14` matching mUnknown30.
 *
 * No body of the class is written, and the layout is recovered only as far as the destructor and
 * HandleMessage() reveal. The destructor releases the objects at `+0x24` and
 * `+0x2c` through slot 2 of each and then tears down a further region at `+0x10`, so the class has
 * at least those three members besides the track identity at `+0x30`.
 */
class PhraseMgr : public MsgSink, public MsgSource {
public:
    /**
     * Release both owned objects and the region at `+0x10`.
     *
     * The body is not written.
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
    // Torn down by the destructor after both owned objects, and the destructor is the only
    // recovered routine that touches it.
    int mUnknown10; // +0x10
    // Released by the destructor through slot 2 of its own table, and the object PostPhraseMsg()
    // and three HandleMessage() paths look a phrase up in.
    void *mUnknown24; // +0x24
    // Released by the destructor through slot 2 of its own table.
    void *mUnknown2c; // +0x2c
    // The track this manager serves. A PhrasePacket's `+0x14` is matched against it, and
    // PostPhraseMsg() copies it into the message's `+0x08`.
    int mUnknown30; // +0x30
};
