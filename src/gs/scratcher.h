#pragma once

#include <vector>

#include "gs/pitcher.h"
#include "mid/mbt.h"
#include "msg/message.h"

class AxisRegisterMsg;
class EraseMsg;
class InvalidateSeekerMsg;
class PhraseMgr;
class PitchRiffMsg;
class Player;
class Quantizer;
class TrackData;
class TrackSelectMsg;

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Pitcher that drives the scratch track.
 *
 * Its RTTI descriptor is at `0x00901fe0`. It has Pitcher as its one base. Its three tables are at
 * `0x007e57b8`, `0x007e5790`, and `0x007e5760`, and it overrides exactly the two slots Pitcher
 * declares pure. PitchingSTG's tagged allocation measures the object at 0x90 bytes, and PitchingSTG
 * builds one of these when the track's kind word is 3 and a NotePitcher when it is 2.
 *
 * Tick() is what fixes mBarDivisor. It divides the elapsed tick count by that member, sends the
 * result through SendSeekerMsg(), and then, while mSwitchesBanks is set, tests the track
 * description against the same bar and dispatches a slot on the synthesiser that
 * Globals::GetSynth() returns.
 *
 * The constructor fixes the member map. It copies the bar length from the phrase manager's `+0x34`
 * into mBarDivisor and the track's identity and MIDI channel out of the track description, starts
 * both player references at the stand-in player, and sizes mReadings to three zeroed elements.
 */
class Scratcher : public Pitcher {
public:
    /**
     * Construct a scratcher for one track.
     *
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pClock The clock the producer schedules against.
     * @param pTrackData The track description.
     * @ghidraAddress NTSC-U/C: 0x001cf988
     * @ghidraAddress PAL: 0x001d5840
     */
    Scratcher(PhraseMgr *pPhraseMgr,
              Quantizer *pQuantizer,
              Sch::TickClock *pClock,
              const TrackData *pTrackData);

    /**
     * @ghidraAddress NTSC-U/C: 0x001d1bf0
     * @ghidraAddress PAL: 0x001d7aa8
     */
    virtual ~Scratcher();

    /**
     * Advance to the bar the elapsed tick count falls in.
     *
     * Divides the elapsed count by mBarDivisor and sends that bar through SendSeekerMsg(). While
     * mSwitchesBanks is set and the bar starts a step, it then selects the synthesiser bank of that
     * step for the channel in mChannel, as AxePhraseMaker::OnPeriod() does.
     *
     * @param nElapsedTicks Ticks since the epoch.
     * @return 1 always.
     * @ghidraAddress NTSC-U/C: 0x001d1d68
     * @ghidraAddress PAL: 0x001d7c20
     */
    virtual int Tick(int nElapsedTicks);

protected:
    /**
     * Turn an axis reading for this track and player into scratches.
     *
     * The routine sends a NowBarMsg at lane one minus the value, then tracks the reading's
     * movement against the ring of past readings in mReadings and replays the last gem through
     * OnPitchRiff() at a step of up to 3 in either direction.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001cfd20
     * @ghidraAddress PAL: 0x001d5bd8
     */
    void PostNowBarMsg(AxisRegisterMsg *pMsg);

    /**
     * Erase a player's phrases at an EraseMsg's position for this track.
     *
     * The bar, or with the message's last word set every bar of its step, is cleared wherever it
     * belongs to the message's player, and clearing the message's bar also sends an AllNotesOffMsg.
     * When anything was cleared, `SND_ERASE_SECTION` or `SND_ERASE` plays, a ShowEraseEffectMsg
     * identifying mPlayer goes out, and SendSeekerMsg() runs for the bar.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001d0038
     * @ghidraAddress PAL: 0x001d5ef0
     */
    void EraseGemRange(EraseMsg *pMsg);

    /**
     * Install the player a TrackSelectMsg for this track selects.
     *
     * A real new player first gets a NowBarMsg at lane 0.5. A message with a zero second word
     * installs the player, and a real player then has SendSeekerMsg() run for the message's bar.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001d0248
     * @ghidraAddress PAL: 0x001d6100
     */
    void OnTrackSelect(TrackSelectMsg *pMsg);

    /**
     * Play a gem at a pitch step.
     *
     * The routine checks the bar, sends the riff transposed by nStep, records the gem, and
     * announces it with several messages, among them a DurGemMsg and a PitchMsg.
     *
     * @param nGem The gem, a PitchRiffMsg's first word.
     * @param nStep The pitch step, zero from a PitchRiffMsg and -3 to 3 from PostNowBarMsg().
     * @param nTick The song position.
     * @ghidraAddress NTSC-U/C: 0x001d0358
     * @ghidraAddress PAL: 0x001d6210
     */
    void OnPitchRiff(int nGem, int nStep, int nTick);

    /**
     * Turn mPlayer's seeker off, unless mPlayer is the stand-in.
     *
     * @param nBar Not read.
     * @ghidraAddress NTSC-U/C: 0x001d08e0
     * @ghidraAddress PAL: 0x001d6798
     */
    void SendSeekerMsg(int nBar) const;

    /**
     * Act on a message.
     *
     * Primary table slot 3.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001d0980
     * @ghidraAddress PAL: 0x001d6838
     */
    virtual void DispatchPriv(Message *pMsg);

private:
    // The out-of-line copy of the PitchRiffMsg branch DispatchPriv() expands inline.
    // NTSC-U/C: 0x001d1cc8, PAL: 0x001d7b80
    void OnPitchRiffMsg(PitchRiffMsg *pMsg);

    // The out-of-line copy of the InvalidateSeekerMsg branch DispatchPriv() expands inline.
    // NTSC-U/C: 0x001d1d18, PAL: 0x001d7bd0
    void OnInvalidateSeeker(InvalidateSeekerMsg *pMsg);

    // Returns TrackData::QueryBar() for the bar on mTrackData. OnPitchRiff() calls it (at
    // `0x001d03dc`).
    // NTSC-U/C: 0x001d1d48, PAL: 0x001d7c00
    int QueryBar(int nBar);

    PhraseMgr *mPhraseMgr;       // +0x38
    Quantizer *mQuantizer;       // +0x3c
    const TrackData *mTrackData; // +0x40, the object Tick() tests the current bar against
    // Copied from the track description's `+0x04`. Matched against a PitchRiffMsg's `+0x10` and
    // an InvalidateSeekerMsg's `+0x08`, so it identifies the track this Scratcher serves.
    int mTrack; // +0x44
    // Copied from the phrase manager's `+0x34`. Turns an elapsed tick count into a bar index.
    int mBarDivisor;        // +0x48
    Sch::TickClock *mClock; // +0x4c
    // An IDable identifier, kIDableUnregistered from the constructor. The class's routines do not
    // read it.
    int mId; // +0x50
    // Tick() performs its second half only while this is set. The constructor derives it from two
    // configuration queries.
    int mSwitchesBanks; // +0x54
    // The track description's MIDI channel byte, and the argument Tick() hands to the synthesiser
    // slot.
    int mChannel; // +0x58
    // Starts at NullPlayer::sInstance. Matched against a PitchRiffMsg's `+0x08`, which
    // msg/pitchriffmsg.h types as an int.
    Player *mPlayer; // +0x5c
    // The position of the last scratch, kMBTInfinity at first.
    Mid::MBT mLastScratchPosition; // +0x60
    // The player of the last scratch, NullPlayer::sInstance at first.
    Player *mLastScratchPlayer; // +0x64
    // The gem of the last PitchRiffMsg. PostNowBarMsg() replays it.
    int mLastGem;          // +0x68
    int mAnnouncedBar;     // +0x6c, the bar last announced, -1 at first
    int mScratchDirection; // +0x70, the scratch direction PostNowBarMsg() last detected
    // The ring of past axis positions. The constructor builds three elements from an int zero
    // through the float fill instantiation at 0x001d1f90. The fill converts each with cvt.s.w.
    std::vector<float> mReadings; // +0x74
    int mNewestReading;     // +0x80, the newest slot of mReadings, not written by the constructor
    Mid::MBT mLastGemEnd;   // +0x84, where the last scratch gem ends
    float mLastGemEndBlend; // +0x88, the blend the last scratch gem ends at
    int mLastStep;          // +0x8c, the step of the last scratch
};
