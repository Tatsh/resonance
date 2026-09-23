#pragma once

#include <vector>

#include "gs/pitcher.h"
#include "mid/mbt.h"
#include "msg/message.h"

class InvalidateSeekerMsg;
class PhraseMgr;
class PitchRiffMsg;
class Player;
class Quantizer;
class TrackData;

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Pitcher that drives the scratch track.
 *
 * `9Scratcher` in the RTTI descriptor at `0x00901fe0`, with Pitcher as its one base. Its three
 * tables are at `0x007e57b8`, `0x007e5790`, and `0x007e5760`, and it overrides exactly the two
 * slots Pitcher leaves pure. PitchingSTG's tagged allocation measures the object at 0x90 bytes,
 * and PitchingSTG builds one of these when the track's kind word is 3 and a NotePitcher when it is
 * 2.
 *
 * Tick() is what fixes mBarDivisor. It divides the elapsed tick count by that member, sends the
 * result through SendSeekerMsg(), and then, while mUnknown54 is set, tests the track description
 * against the same bar and dispatches a slot on the synthesiser that Globals::GetSynth() returns.
 *
 * The constructor fixes the member map. It copies the bar length from the phrase manager's `+0x34`
 * into mBarDivisor and the track's identity and MIDI channel out of the track description, starts
 * both player references at the stand-in player, and sizes mUnknown74 to three zeroed words.
 *
 * Only HandleMessage() is written. The five routines it dispatches to, Tick(), and the constructor
 * are declared with their addresses and their bodies are not written.
 */
class Scratcher : public Pitcher {
public:
    /**
     * Construct a scratcher for one track.
     *
     * The body is not written.
     *
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pClock The clock the producer schedules against.
     * @param pTrackData The track description.
     * @ghidraAddress 0x001cf988
     */
    Scratcher(PhraseMgr *pPhraseMgr,
              Quantizer *pQuantizer,
              Sch::TickClock *pClock,
              const TrackData *pTrackData);

    /**
     * @ghidraAddress 0x001d1bf0
     */
    virtual ~Scratcher();

    /**
     * Advance to the bar the elapsed tick count falls in.
     *
     * Divides the elapsed count by mBarDivisor and sends that bar through SendSeekerMsg(). While
     * mUnknown54 is set and the test at `0x001d79d0` accepts the bar, it then dispatches slot 0 of
     * whatever Globals::GetSynth() returns, with mUnknown58 as one argument. That second half
     * reads three routines whose verbs are unrecovered, which is why the body records the shape
     * rather than the work.
     *
     * @param nElapsedTicks Ticks since the epoch.
     * @return 1 always.
     * @ghidraAddress 0x001d1d68
     */
    virtual int Tick(int nElapsedTicks);

protected:
    /**
     * React to an AxisRegisterMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001cfd20
     */
    void PostNowBarMsg(Message *pMsg);

    /**
     * React to an EraseMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d0038
     */
    void EraseGemRange(Message *pMsg);

    /**
     * React to a TrackSelectMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d0248
     */
    void OnTrackSelect(Message *pMsg);

    /**
     * React to a PitchRiffMsg. The body is not written.
     *
     * @param nUnknown The message's `+0x04`.
     * @param bUnknown Always zero at the one recovered call site.
     * @param nUnknown2 The message's `+0x0c`.
     * @return The value HandleMessage() stores in mUnknown68.
     * @ghidraAddress 0x001d0358
     */
    int OnPitchRiff(int nUnknown, int bUnknown, int nUnknown2);

    /**
     * Send the seeker message for one bar. The body is not written.
     *
     * @param nBar The bar.
     * @ghidraAddress 0x001d08e0
     */
    void SendSeekerMsg(int nBar);

    /**
     * Act on a message.
     *
     * Primary table slot 3.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d0980
     */
    virtual void HandleMessage(Message *pMsg);

private:
    // The out-of-line copy of the PitchRiffMsg branch HandleMessage() expands inline.
    // 0x001d1cc8
    void OnPitchRiffMsg(PitchRiffMsg *pMsg);

    // The out-of-line copy of the InvalidateSeekerMsg branch HandleMessage() expands inline.
    // 0x001d1d18
    void OnInvalidateSeeker(InvalidateSeekerMsg *pMsg);

    // Returns TrackData::QueryBar() for the bar on mTrackData. OnPitchRiff() calls it at
    // 0x001d03dc.
    // 0x001d1d48
    int QueryBar(int nBar);

    PhraseMgr *mPhraseMgr;       // +0x38
    Quantizer *mQuantizer;       // +0x3c
    const TrackData *mTrackData; // +0x40, the object Tick() tests the current bar against
    // Copied from the track description's `+0x04`. Matched against a PitchRiffMsg's `+0x10` and
    // an InvalidateSeekerMsg's `+0x08`, so it identifies the track this Scratcher serves.
    int mUnknown44; // +0x44
    // Copied from the phrase manager's `+0x34`. Turns an elapsed tick count into a bar index.
    int mBarDivisor;        // +0x48
    Sch::TickClock *mClock; // +0x4c
    int mUnknown50;         // +0x50, starts at kIDableUnregistered
    // Tick() performs its second half only while this is set. The constructor derives it from two
    // configuration queries.
    int mUnknown54; // +0x54
    // The track description's MIDI channel byte, and the argument Tick() hands to the synthesiser
    // slot.
    int mUnknown58; // +0x58
    // Starts at g_nullPlayer. Matched against a PitchRiffMsg's `+0x08`, which msg/pitchriffmsg.h
    // types as an int.
    Player *mUnknown5c;          // +0x5c
    Mid::MBT mUnknown60;         // +0x60, starts at kMBTInfinity
    Player *mUnknown64;          // +0x64, starts at g_nullPlayer
    int mUnknown68;              // +0x68, result of the PitchRiffMsg handler
    int mUnknown6c;              // +0x6c, starts at -1
    int mUnknown70;              // +0x70
    std::vector<int> mUnknown74; // +0x74, three zeroed words on construction
    int mUnknown80;              // +0x80, not written by the constructor
    int mUnknown84;              // +0x84
    int mUnknown88;              // +0x88
    int mUnknown8c;              // +0x8c
};
