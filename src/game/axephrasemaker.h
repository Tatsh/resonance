#pragma once

#include <vector>

#include "game/phrase.h"
#include "game/phrasemaker.h"
#include "game/player.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "mid/mbt.h"
#include "msg/invalidateseekermsg.h"
#include "msg/message.h"
#include "msg/musemsg.h"
#include "msg/stdmidimsg.h"
#include "msg/sustainnotemsg.h"
#include "msg/trackselectmsg.h"
#include "sch/tickclock.h"

/**
 * Phrase maker for a guitar track.
 *
 * Its RTTI descriptor is at `0x008f2a50`. It has PhraseMaker as its only base at offset 0. Its
 * primary table is at `0x007ddc50` with six entries and its MsgSource subobject table at
 * `0x007ddc28` with four. The class introduces two virtuals. The object is 0x50 bytes, measured by
 * AxingSTG's tagged allocation.
 *
 * Slot 0 of the table at `0x007ddc50` addresses the accessor at `0x0019d390`. The accessor guards
 * on the descriptor at `0x008f2a50`, and the descriptor settles the class name.
 *
 * The maker records what the player plays over a bar into a fresh Phrase and installs it in the
 * phrase manager when the bar ends. A note-on is held until its note-off (or the end of the bar)
 * gives it a length, and every recorded message also records the current axis value.
 *
 * The destructor at `0x0019b7f0` is implicitly declared. It destroys mHeldNotes and MsgSource's
 * vector and releases the object under MsgSink's tag.
 */
class AxePhraseMaker : public PhraseMaker {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pTrackData The track description. The constructor copies its track and channel.
     * @param pClock Not read.
     * @ghidraAddress NTSC-U/C: 0x0019b5d0
     * @ghidraAddress PAL: 0x001a1338
     */
    AxePhraseMaker(PhraseMgr *pPhraseMgr,
                   Quantizer *pQuantizer,
                   const TrackData *pTrackData,
                   Sch::TickClock *pClock);

    /**
     * Act on a message.
     *
     * Slot 3. An AxisRegisterMsg for this track and player stores its value in mValue. A
     * StdMidiMsg goes to OnStdMidi(), a SustainNoteMsg to the branch OnSustainNote() copies, a
     * TrackSelectMsg to the branch OnTrackSelect() copies, and an InvalidateSeekerMsg to the branch
     * OnInvalidateSeeker() copies.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x0019c408
     * @ghidraAddress PAL: 0x001a2170
     */
    virtual void DispatchPriv(Message *pMsg);

    /**
     * Act on the start of a bar. Slot 4.
     *
     * A bar right after mPhraseBar finishes the phrase in progress, and the seeker is cleared.
     * With mSwitchBanks set, a bar that starts a step selects the synthesiser bank of that step
     * for mChannel.
     *
     * @param nBar The bar.
     * @ghidraAddress NTSC-U/C: 0x0019d990
     * @ghidraAddress PAL: 0x001a36f8
     */
    virtual void OnPeriod(int nBar);

    /**
     * Report the song position periods are counted from. Slot 5.
     *
     * @return 6 always, after a discarded finiteness test on the same value.
     * @ghidraAddress NTSC-U/C: 0x0019d438
     * @ghidraAddress PAL: 0x001a31a0
     */
    virtual int GetPeriodOrigin();

    /**
     * Report whether a bar can be played.
     *
     * AutoRiffer is the recovered caller.
     *
     * @param nBar The bar.
     * @return Non-zero when TrackData::QueryBar() accepts the bar and Player::IsFreestyleBar()
     *         reports non-zero for it.
     * @ghidraAddress NTSC-U/C: 0x0019da58
     * @ghidraAddress PAL: 0x001a37c0
     */
    int IsBarPlayable(int nBar);

    /**
     * Erase a player's phrase at a song position.
     *
     * The bar of the position, or with bWholeStep set every bar of its step, is cleared through
     * PhraseMgr::ClearPhrase() wherever pPlayer owns it. When anything was cleared, or a phrase is
     * in progress, one of two sounds plays and a ShowEraseEffectMsg goes out. The phrase in
     * progress is discarded either way. The sound is `SND_ERASE_SECTION` for a whole step and
     * `SND_ERASE` otherwise, and the message names mPlayer rather than pPlayer.
     * AutoRiffer::OnErase() is the recovered caller.
     *
     * @param pPlayer The player the erase is for.
     * @param nTick The song position, in MIDI ticks.
     * @param bWholeStep Non-zero to erase the whole step around the position.
     * @ghidraAddress NTSC-U/C: 0x0019bf80
     * @ghidraAddress PAL: 0x001a1ce8
     */
    void Erase(Player *pPlayer, int nTick, int bWholeStep);

    /**
     * A note-on the maker holds until its note-off.
     *
     * Eight bytes. Every build clears the record with memset() before storing the fields.
     */
    struct HeldNote {
        unsigned char mNote;     /*!< The note number. +0x00 */
        unsigned char mVelocity; /*!< The note-on velocity. +0x01 */
        int mTick;               /*!< The song position of the note-on. +0x04 */
    };

private:
    // A note-on starts the phrase for its tick, holds the note, and takes the message's channel
    // as mChannel. A note-off records the first held note of its number as a NoteMsg lasting
    // until the note-off, and drops it. Any other channel message starts the phrase and is
    // recorded as it is.
    // NTSC-U/C: 0x0019b958, PAL: 0x001a16c0
    void OnStdMidi(StdMidiMsg *pMsg);

    // Records one message into mPhrase at its offset in mPhraseBar, with mValue as the value.
    // NTSC-U/C: 0x0019bbd8, PAL: 0x001a1940
    void RecordMuseMsg(MuseMsg *pMsg);

    // Unless mPhrase is already recording the bar of nTick, finishes the phrase in progress and
    // starts a new one for the player there, announcing it with a ClearGemsMsg, a BarStatusMsg,
    // a BeginPhraseCatchMsg, and a PhraseCapturedMsg.
    // NTSC-U/C: 0x0019bce0, PAL: 0x001a1a48
    void StartPhrase(int nTick);

    // Gives every held note a NoteMsg ending one tick after the bar, installs mPhrase in the
    // phrase manager at mPhraseBar, and releases it.
    // NTSC-U/C: 0x0019c118, PAL: 0x001a1e80
    void FinishPhrase();

    // Sends a SeekerMsg that turns mPlayer's seeker off, unless mPlayer is the stand-in. The bar
    // is not read.
    // NTSC-U/C: 0x0019c368, PAL: 0x001a20d0
    void SendSeekerMsg(int nBar) const;

    // The out-of-line copy of the TrackSelectMsg branch DispatchPriv() expands inline.
    // NTSC-U/C: 0x0019d860, PAL: 0x001a35c8
    void OnTrackSelect(TrackSelectMsg *pMsg);

    // The out-of-line copy of the InvalidateSeekerMsg branch DispatchPriv() expands inline.
    // NTSC-U/C: 0x0019d8f0, PAL: 0x001a3658
    void OnMsg(const InvalidateSeekerMsg &msg);

    // The out-of-line copy of the SustainNoteMsg branch DispatchPriv() expands inline.
    // NTSC-U/C: 0x0019d920, PAL: 0x001a3688
    void OnSustainNote(SustainNoteMsg *pMsg);

    PhraseMgr *mPhraseMgr;            // +0x18
    Quantizer *mQuantizer;            // +0x1c, not read by any recovered routine
    int mTrack;                       // +0x20
    unsigned char mChannel;           // +0x24
    Phrase *mPhrase;                  // +0x28, the phrase in progress, or null
    int mPhraseBar;                   // +0x2c, the bar mPhrase records, -1 at first
    Player *mPlayer;                  // +0x30, NullPlayer::sInstance until a TrackSelectMsg
    std::vector<HeldNote> mHeldNotes; // +0x34
    Mid::MBT mBarTicks;               // +0x40
    const TrackData *mTrackData;      // +0x44
    int mSwitchBanks;                 // +0x48, from configuration codes 0x3a4 and 0x3a1
    float mValue;                     // +0x4c, the axis value recorded with every message
};
