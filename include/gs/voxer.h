#pragma once

#include <bitset>

#include "gs/pitcher.h"
#include "msg/message.h"

class EraseMsg;
class InvalidateSeekerMsg;
class Phrase;
class PhraseMgr;
class PitchRiffMsg;
class Player;
class Quantizer;
class StopRiffMsg;
class TrackData;
class TrackSelectMsg;

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
 * The Voxer records what the singer holds into a Phrase per bar, as AxePhraseMaker does for a
 * guitar. Holding any PitchRiffMsg level presses the sustain controller (46) and releasing the
 * last one lets it go. OnTrackSelect() is the one routine not written.
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
     * Close the previous bar and keep the sustain in step with the song.
     *
     * At bar zero the sustain controller is released at position kMBTInfinity. The routine
     * finishes the phrase of the previous bar and turns the seeker off. At a bar
     * TrackData::QueryBar() rejects while sustaining, it clears the held levels, records and
     * sends the sustain release in a new phrase, and releases the button with an AxeButtonMsg.
     *
     * @param nElapsedTicks Ticks since the epoch.
     * @return 1 always.
     * @ghidraAddress 0x001d8dc8
     */
    virtual int Tick(int nElapsedTicks);

protected:
    /**
     * Hold a level for this track and player.
     *
     * The level's bit joins mHeldLevels, UpdateSustain() runs at the message's position, and an
     * AxeButtonMsg presses the button.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d8370
     */
    void OnPitchRiff(PitchRiffMsg *pMsg);

    /**
     * Release a level for this track and player.
     *
     * The level's bit leaves mHeldLevels and UpdateSustain() runs at the message's position. With
     * no level left held, an AxeButtonMsg releases the button.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d8430
     */
    void OnStopRiff(StopRiffMsg *pMsg);

    /**
     * Install the player a TrackSelectMsg for this track selects.
     *
     * A player still holding levels has them cleared, the sustain updated, and the button
     * released. A real new player gets a NowBarMsg at lane 0.5 and its seeker turned off. The
     * body is not written, because NowBarMsg's word at `+0x04` is private and the class has no
     * payload constructor.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d8840
     */
    void OnTrackSelect(TrackSelectMsg *pMsg);

    /**
     * Erase mUnknown50's phrases around a bar.
     *
     * The bar, or with bWholeStep set every bar of its step, is cleared wherever mUnknown50 owns
     * it, and clearing nBar itself also releases the sustain controller at position
     * kMBTInfinity. When anything was cleared, bAnnounce plays `SND_ERASE_SECTION` or `SND_ERASE`
     * and sends a ShowEraseEffectMsg, and the seeker is turned off either way.
     *
     * @param nBar The bar.
     * @param bWholeStep Non-zero to erase the whole step, an EraseMsg's `+0x10`.
     * @param bAnnounce Non-zero for an EraseMsg, and zero from StartPhrase().
     * @ghidraAddress 0x001d8638
     */
    void OnErase(int nBar, int bWholeStep, int bAnnounce);

    /**
     * Turn mUnknown50's seeker off, unless mUnknown50 is the stand-in.
     *
     * @param nBar Not read.
     * @ghidraAddress 0x001d8fb0
     */
    void OnInvalidateSeeker(int nBar);

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

    // Returns TrackData::QueryBar() for the bar on mTrackData. UpdateSustain() calls it.
    // 0x001d9ec8
    int QueryBar(int nBar);

    // When the held levels no longer match mSustaining, sends the sustain controller (0 while
    // held, 127 once released) at nTick, recorded into the phrase at its offset in the bar. A
    // bar QueryBar() rejects plays SND_INACTIVE instead. The title is inferred.
    // 0x001d8508
    void UpdateSustain(int nTick);

    // Unless the bar of nTick is mPhraseBar, finishes the phrase, sends a BarStatusMsg, starts a
    // new Phrase for mUnknown50 there, and silently erases the bar. The title is inferred.
    // 0x001d89d0
    void StartPhrase(int nTick);

    // For nBar equal to mPhraseBar with a phrase in progress, closes a held sustain one tick
    // before the bar ends, installs the phrase in the phrase manager, and releases it. A held
    // sustain then reopens at the start of the next bar in a new phrase. The title is inferred.
    // 0x001d8b00
    void FinishPhrase(int nBar);

    static constexpr int kLevelBits = 64;

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
    // Non-zero while the sustain controller is down, which is while any level is held.
    int mSustaining; // +0x54
    Phrase *mPhrase; // +0x58, the phrase being recorded, or null
    int mPhraseBar;  // +0x5c, the bar mPhrase records, -1 at first
    int mUnknown60;  // +0x60 set to -1 on construction
    int mUnknown64;  // +0x64 not written by the constructor
    // One bit per PitchRiffMsg level held down. The constructor clears it as one doubleword.
    std::bitset<kLevelBits> mHeldLevels; // +0x68
};
