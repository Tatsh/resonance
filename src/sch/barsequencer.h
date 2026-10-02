#pragma once

#include <cstddef>
#include <iostream>

#include "app/ticktask.h"
#include "mid/tickobj.h"
#include "os/mem.h"
#include "sch/sequencer.h"

class MsgSink;
class MuseMsg;
class TrackData;

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Task that sends one track's MIDI a bar at a time.
 *
 * `12BarSequencer` in the RTTI descriptor at `0x008ef5f0`, deriving publicly from TickTask at
 * offset 0. Its type function is at `0x00100bb8` and its table at `0x007cc808` keeps
 * Attachment::Destroy(). The task runs once a bar, and each run replaces the Sequencer it owns
 * with one over the bar's MIDI and posts it against the clock.
 *
 * ScoreTrackGraph and the routine at `0x0013fc48` construct it.
 */
class BarSequencer : public TickTask {
public:
    /**
     * Prepare the task with no sequencer.
     *
     * @param pClock The clock the task and its sequencers run against.
     * @param pTrack The track whose MIDI is sent.
     * @param pSink The sink every message goes to.
     * @param nUnmapped Non-zero to index the track's bars directly rather than through the play
     *        map.
     * @ghidraAddress 0x00100c70
     */
    BarSequencer(Sch::TickClock *pClock, TrackData *pTrack, MsgSink *pSink, int nUnmapped);

    /**
     * Delete the sequencer and release the task.
     *
     * @ghidraAddress 0x00100d78
     */
    virtual ~BarSequencer();

    /**
     * Allocate a task under the tag `BarSequencer`.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x00100c08
     */
    static void *operator new(size_t nSize);

    /**
     * Release a task under the tag `BarSequencer`.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x00100c28
     */
    static void operator delete(void *pBlock);

    /**
     * Write `{BarSequencer}`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00100c48
     */
    virtual void Print(std::ostream &stream);

    /**
     * Start sending the MIDI of the bar a position falls in.
     *
     * Deletes the previous sequencer, builds one over the bar's MIDI, and posts it.
     *
     * @param nTick The song position of the run, in MIDI ticks.
     * @return 1, which keeps the task running.
     * @ghidraAddress 0x00100370
     */
    virtual int Tick(int nTick);

private:
    TrackData *mTrack;
    MsgSink *mSink;
    Sch::TickClock *mClock;
    int mUnmapped;
    Sequencer<const TickObj<MuseMsg *> *> *mSequencer;
};
