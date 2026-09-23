#pragma once

#include "gs/pitcher.h"
#include "msg/message.h"

class EraseMsg;
class InvalidateSeekerMsg;
class PhraseMgr;
class Player;
class Quantizer;
class TrackData;

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Pitcher that drives the vocal track.
 *
 * `5Voxer` in the RTTI descriptor at `0x008f0830`, with Pitcher as its one base. Its three tables
 * are at `0x007e61a8`, `0x007e6180`, and `0x007e6150`, and it overrides exactly the two slots
 * Pitcher leaves pure: HandleMessage() in the primary table and Tick() in the TickTask table.
 * VoxingSTG's tagged allocation measures the object at 0x70 bytes, and VoxingSTG builds exactly
 * one of these without branching.
 *
 * The constructor takes four arguments, the fourth in `t0`, and fixes the layout of the derived
 * part. It constructs the TickTask base with a period of 1920 ticks, which is one measure at 480
 * ticks per quarter note. VoxingSTG's call at `0x001da0b0` passes its phrase manager, its
 * quantiser, the song clock, and its track description, which types all four.
 *
 * The constructor and HandleMessage() are written. The five routines HandleMessage() dispatches to
 * and the Tick() override are declared with their addresses and their bodies are not written.
 */
class Voxer : public Pitcher {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track. Its `+0x34` is copied into mUnknown48.
     * @param pQuantizer The quantiser for the track.
     * @param pClock The clock the TickTask base is posted against.
     * @param pTrackData The track description, which arrives in `t0`. Its `+0x04` is copied into
     *                   mUnknown44 and its `+0x08` byte into mChannel.
     * @ghidraAddress 0x001d81b8
     */
    Voxer(PhraseMgr *pPhraseMgr,
          Quantizer *pQuantizer,
          Sch::TickClock *pClock,
          const TrackData *pTrackData);

    /**
     * @ghidraAddress 0x001d9dd0
     */
    virtual ~Voxer();

    /**
     * @ghidraAddress 0x001d8dc8
     */
    virtual int Tick(int nElapsedTicks);

protected:
    /**
     * React to a PitchRiffMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d8370
     */
    void OnPitchRiff(Message *pMsg);

    /**
     * React to a StopRiffMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d8430
     */
    void OnStopRiff(Message *pMsg);

    /**
     * React to a TrackSelectMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d8840
     */
    void OnTrackSelect(Message *pMsg);

    /**
     * React to an EraseMsg. The body is not written.
     *
     * @param nBar The message's `+0x08` divided by mUnknown48.
     * @param nUnknown The message's `+0x10`.
     * @param bUnknown Always 1 at the one recovered call site.
     * @ghidraAddress 0x001d8638
     */
    void OnErase(int nBar, int nUnknown, int bUnknown);

    /**
     * React to an InvalidateSeekerMsg. The body is not written.
     *
     * @param nUnknown The message's `+0x04`.
     * @ghidraAddress 0x001d8fb0
     */
    void OnInvalidateSeeker(int nUnknown);

    /**
     * Act on a message.
     *
     * Primary table slot 3.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d9050
     */
    virtual void HandleMessage(Message *pMsg);

private:
    // The out-of-line copy of the EraseMsg branch HandleMessage() expands inline.
    // 0x001d9e40
    void OnEraseMsg(EraseMsg *pMsg);

    // The out-of-line copy of the InvalidateSeekerMsg branch HandleMessage() expands inline.
    // 0x001d9e98
    void OnInvalidateSeekerMsg(InvalidateSeekerMsg *pMsg);

    // Returns TrackData::QueryBar() for the bar on mTrackData. The routine at 0x001d8508 calls it.
    // 0x001d9ec8
    int QueryBar(int nBar);

    PhraseMgr *mPhraseMgr;       // +0x38
    Quantizer *mQuantizer;       // +0x3c
    const TrackData *mTrackData; // +0x40
    // Matched against an EraseMsg's `+0x0c` and an InvalidateSeekerMsg's `+0x08`, so it identifies
    // the track this Voxer serves.
    int mUnknown44; // +0x44 copied from the track description's `+0x04`
    // Divisor that turns an elapsed tick count into a bar index.
    int mUnknown48;         // +0x48 copied from the phrase manager's `+0x34`
    unsigned char mChannel; // +0x4c copied from the track description's `+0x08`
    // Matched against an EraseMsg's `+0x04`, and defaulted to g_nullPlayer, which Mixer also
    // defaults its own selection to.
    Player *mUnknown50; // +0x50
    int mUnknown54;     // +0x54 cleared on construction
    int mUnknown58;     // +0x58 cleared on construction
    int mUnknown5c;     // +0x5c set to -1 on construction
    int mUnknown60;     // +0x60 set to -1 on construction
    int mUnknown64;     // +0x64 not written by the constructor
    int mUnknown68;     // +0x68 cleared on construction as one doubleword with mUnknown6c
    int mUnknown6c;     // +0x6c
};
