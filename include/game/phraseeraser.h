#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/erasemsg.h"
#include "msg/message.h"

class PhraseMgr;
class Player;

/**
 * Listener that records the bar a player erases on one track.
 *
 * `12PhraseEraser` in the RTTI descriptor at `0x00901f20`, over MsgSource at offset 0 and MsgSink
 * at offset 20. The primary table is at `0x007e1f98` and the MsgSink table at `0x007e1f70`, which
 * adjusts `this` by `-20` and fills slot 3 with HandleMessage(). No routine in the image calls the
 * constructor, so the class is compiled but never built. The last member read or written is at
 * `+0x38`, and no allocation measures the object.
 *
 * The destructor at `0x001b9908` is implicitly declared.
 */
class PhraseEraser : public MsgSource, public MsgSink {
public:
    /**
     * Store the five arguments. Three are not read by any recovered routine, so their types are
     * recorded as words.
     *
     * @param nTrack The track the eraser listens for.
     * @param nUnknown34 Stored at `+0x34`.
     * @param pPhraseMgr The phrase manager whose bar length divides an erase position.
     * @param nUnknown38 Stored at `+0x38`.
     * @param nUnknown28 Stored at `+0x28`.
     * @ghidraAddress 0x001b99c8
     */
    PhraseEraser(int nTrack, int nUnknown34, PhraseMgr *pPhraseMgr, int nUnknown38, int nUnknown28);

    /**
     * Act on a message.
     *
     * Slot 3 of the MsgSink table. An EraseMsg for mTrack goes through the branch OnEraseMsg()
     * copies, and every other message is discarded.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b9ad0
     */
    virtual void HandleMessage(Message *pMsg);

private:
    // The out-of-line copy of the EraseMsg branch HandleMessage() expands inline. Marks the eraser
    // active, records the erased bar twice and the player, and passes the bar to EraseBar().
    // 0x001b9a60
    void OnEraseMsg(EraseMsg *pMsg);

    // An empty body. The title is inferred from its one argument, the erased bar.
    // 0x001b9ac8
    void EraseBar(int nBar);

    int mActive;           // +0x18
    int mFirstBar;         // +0x1c
    int mLastBar;          // +0x20
    Player *mPlayer;       // +0x24
    int mUnknown28;        // +0x28
    int mTrack;            // +0x2c
    PhraseMgr *mPhraseMgr; // +0x30
    int mUnknown34;        // +0x34
    int mUnknown38;        // +0x38
};
