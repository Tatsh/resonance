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
 * both player references at the stand-in player, and sizes mUnknown74 to three zeroed elements.
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
     * mUnknown54 is set and the bar starts a step, it then selects the synthesiser bank of that
     * step for the channel in mUnknown58, as AxePhraseMaker::Slot4() does.
     *
     * @param nElapsedTicks Ticks since the epoch.
     * @return 1 always.
     * @ghidraAddress 0x001d1d68
     */
    virtual int Tick(int nElapsedTicks);

protected:
    /**
     * Turn an axis reading for this track and player into scratches.
     *
     * The routine sends a NowBarMsg at lane one minus the value, then tracks the reading's
     * movement against the ring of past readings in mUnknown74 and replays the last gem through
     * OnPitchRiff() at a step of up to 3 in either direction.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001cfd20
     */
    void PostNowBarMsg(AxisRegisterMsg *pMsg);

    /**
     * Erase a player's phrases at an EraseMsg's position for this track.
     *
     * The bar, or with the message's last word set every bar of its step, is cleared wherever the
     * message's player owns it, and clearing the message's own bar also sends an AllNotesOffMsg.
     * When anything was cleared, `SND_ERASE_SECTION` or `SND_ERASE` plays, a ShowEraseEffectMsg
     * naming mUnknown5c goes out, and SendSeekerMsg() runs for the bar.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d0038
     */
    void EraseGemRange(EraseMsg *pMsg);

    /**
     * Install the player a TrackSelectMsg for this track selects.
     *
     * A real new player first gets a NowBarMsg at lane 0.5. A message with a zero second word
     * installs the player, and a real player then has SendSeekerMsg() run for the message's bar.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d0248
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
     * @ghidraAddress 0x001d0358
     */
    void OnPitchRiff(int nGem, int nStep, int nTick);

    /**
     * Turn mUnknown5c's seeker off, unless mUnknown5c is the stand-in.
     *
     * @param nBar Not read.
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

    // Returns TrackData::QueryBar() for the bar on mTrackData. OnPitchRiff() calls it (at
    // `0x001d03dc`).
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
    Player *mUnknown5c;  // +0x5c
    Mid::MBT mUnknown60; // +0x60, the position of the last scratch, kMBTInfinity at first
    Player *mUnknown64;  // +0x64, the player of the last scratch, g_nullPlayer at first
    int mUnknown68;      // +0x68, the gem of the last PitchRiffMsg, which PostNowBarMsg() replays
    int mUnknown6c;      // +0x6c, the bar last announced, -1 at first
    int mUnknown70;      // +0x70, the scratch direction PostNowBarMsg() last detected
    // +0x74. The constructor builds three elements from an int zero through the float fill
    // instantiation at 0x001d1f90, which converts each with cvt.s.w.
    std::vector<float> mUnknown74;
    int mUnknown80;      // +0x80, the newest slot of mUnknown74, not written by the constructor
    Mid::MBT mUnknown84; // +0x84, where the last scratch gem ends
    float mUnknown88;    // +0x88, the blend the last scratch gem ends at
    int mUnknown8c;      // +0x8c, the step of the last scratch
};
