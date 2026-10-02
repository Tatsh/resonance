#pragma once

#include <algorithm>
#include <cstddef>

#include "app/msgsink.h"
#include "mid/mbt.h"
#include "msg/musemsg.h"
#include "os/mem.h"
#include "sch/genericsequencer.h"
#include "sch/sequencercmd.h"
#include "sch/tickclock.h"

/**
 * Dispatcher of one range of timed messages against a clock.
 *
 * The one instantiation in the image is `Sequencer<TickObj<MuseMsg *> const *>`, whose descriptor
 * is at `0x008eec48` with GenericSequencer as its one base at offset 0 and whose table is at
 * `0x007cc7e8`. MultiMusePlayer::Start() and BarSequencer::Tick() construct it, each taking 0x2c
 * bytes under the tag `Sequencer`.
 *
 * Post() records the clock and the sink and sends the range in order, each message at its own
 * position relative to the clock's position at the post. A message due at once is sent
 * immediately, and a later one waits for SequencerCmd. The bodies are template code, and the one
 * instantiation's out-of-line copies sit in the Sequencer unit with further per-unit copies
 * elsewhere.
 */
template <typename T>
class Sequencer : public GenericSequencer {
public:
    /**
     * Prepare a sequencer over a range, before any post.
     *
     * Inline, and expanded into both allocations. The next tick starts at kMBTInfinity.
     *
     * @param begin First object of the range.
     * @param finish One past the last object of the range.
     */
    Sequencer(T begin, T finish) : mBegin(begin), mFinish(finish) {
    }

    /**
     * Withdraw the queued command, release it, and free the sequencer.
     *
     * @ghidraAddress 0x00100df8
     */
    virtual ~Sequencer() {
        Withdraw();
        if (mCommand != nullptr) {
            mCommand->Release();
        }
    }

    /**
     * Allocate a sequencer under the tag `Sequencer`.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     */
    static void *operator new(size_t nSize) {
        return AllocateTaggedMemory(nSize, "Sequencer");
    }

    /**
     * Release a sequencer under the tag `Sequencer`.
     *
     * @param pBlock The block.
     */
    static void operator delete(void *pBlock) {
        FreeTaggedMemory(pBlock, "Sequencer");
    }

    /**
     * Send a copy of the message at the cursor stamped with its tick, then schedule the next.
     *
     * The copy is deleted once the sink has handled it.
     *
     * @ghidraAddress 0x00100e70
     */
    virtual void Dispatch() {
        MuseMsg *pMsg = mCursor->mValue->CloneAt(mNextTick.mTick);
        mSink->Handle(pMsg);
        delete pMsg;
        ++mCursor;
        ScheduleNext();
    }

    /**
     * Record the clock and the sink and start sending the range.
     *
     * The title is inferred.
     *
     * @param pClock The clock to post against.
     * @param pSink The sink every message goes to.
     * @ghidraAddress 0x00100ef8
     */
    void Post(Sch::TickClock *pClock, MsgSink *pSink) {
        mSink = pSink;
        mClock = pClock;
        mStartTick.mTick = pClock->SongTick();
        mCursor = mBegin;
        ScheduleNext();
    }

    /**
     * Queue or send the message at the cursor.
     *
     * Does nothing at the end of the range. The message's tick is the start tick plus its own
     * position less mOffset, each sum brought into the finite range. A tick equal to the start
     * tick is dispatched at once, and any other is posted, creating SequencerCmd on the first
     * post. The title is inferred.
     *
     * @ghidraAddress 0x00100970
     */
    void ScheduleNext() {
        if (mCursor == mFinish) {
            return;
        }

        const Mid::MBT at(ClampTick(mStartTick.mTick + mCursor->mPosition.mTick));
        mNextTick = Mid::MBT(ClampTick(at.mTick - mOffset.mTick));
        if (mNextTick.mTick == mStartTick.mTick) {
            Dispatch();
            return;
        }

        if (mCommand == nullptr) {
            mCommand = new SequencerCmd(this);
        }
        mClock->PostAtSongTick(mCommand, mNextTick.mTick, mCmdId);
    }

    /**
     * The next object to send.
     *
     * Public because MultiMusePlayer::PlayerFinished() compares it against mFinish from outside the
     * hierarchy and the image exposes no accessor.
     */
    T mCursor;

private:
    static int ClampTick(int nTick) {
        return std::min(std::max(nTick, kMBTMinimum), kMBTMaximum);
    }

    // The tick of the message at the cursor.
    Mid::MBT mNextTick;

public:
    T mBegin;  /*!< First object of the range. */
    T mFinish; /*!< One past the last object of the range. */
};
