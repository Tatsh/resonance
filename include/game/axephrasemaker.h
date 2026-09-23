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
 * `14AxePhraseMaker` in the RTTI descriptor at `0x008f2a50`, with PhraseMaker as its only base at
 * offset 0. Its primary table is at `0x007ddc50` with six entries and its MsgSource subobject table
 * at `0x007ddc28` with four, so the class introduces two virtuals of its own. The object is 0x50
 * bytes, which AxingSTG's tagged allocation measures.
 *
 * An earlier pass titled this class's constructor `RndSpotShadowMap__Construct`. No descriptor
 * among the 574 in the image bears that title. Slot 0 of the table at `0x007ddc50` addresses the
 * accessor at `0x0019d390`, which guards on the descriptor at `0x008f2a50`, and that is what
 * settles the name.
 *
 * The maker records what the player plays over a bar into a fresh Phrase and installs it in the
 * phrase manager when the bar ends. A note-on is held until its note-off (or the end of the bar)
 * gives it a length, and every recorded message also records the current axis value.
 *
 * The destructor at `0x0019b7f0` is implicitly declared. It destroys mHeldNotes and MsgSource's
 * vector and releases the object under MsgSink's tag.
 *
 * Five routines are declared but not written, because each builds or reads a message whose
 * payload is not public yet: HandleMessage() (AxisRegisterMsg), OnStdMidi() and FinishPhrase()
 * (NoteMsg), StartPhrase() (ClearGemsMsg, BarStatusMsg, and PhraseCapturedMsg), and Erase()
 * (ShowEraseEffectMsg).
 */
class AxePhraseMaker : public PhraseMaker {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pTrackData The track description. The constructor copies its track and channel.
     * @param pClock Not read.
     * @ghidraAddress 0x0019b5d0
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
     * @ghidraAddress 0x0019c408
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Act on the start of a bar. Slot 4.
     *
     * A bar right after mPhraseBar finishes the phrase in progress, and the seeker is cleared.
     * With mSwitchBanks set, a bar that starts a step selects the synthesiser bank of that step
     * for mChannel.
     *
     * @param nBar The bar.
     * @ghidraAddress 0x0019d990
     */
    virtual void Slot4(int nBar);

    /**
     * Report the song position periods are counted from. Slot 5.
     *
     * @return 6 always, after a discarded finiteness test on the same value.
     * @ghidraAddress 0x0019d438
     */
    virtual int Slot5();

    /**
     * Report whether a bar can be played.
     *
     * AutoRiffer is the recovered caller.
     *
     * @param nBar The bar.
     * @return Non-zero when TrackData::QueryBar() accepts the bar and Player::Slot9() reports
     *         non-zero for it.
     * @ghidraAddress 0x0019da58
     */
    int IsBarPlayable(int nBar);

    /**
     * Erase a player's phrase at a song position.
     *
     * The bar of the position, or with bWholeStep set every bar of its step, is cleared through
     * PhraseMgr::ClearPhrase() wherever pPlayer owns it. When anything was cleared, or a phrase is
     * in progress, one of two sounds plays and a ShowEraseEffectMsg goes out. The phrase in
     * progress is discarded either way. AutoRiffer::OnErase() is the recovered caller. The body is
     * not written.
     *
     * @param pPlayer The player the erase is for.
     * @param nTick The song position, in MIDI ticks.
     * @param bWholeStep Non-zero to erase the whole step around the position.
     * @ghidraAddress 0x0019bf80
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
    // 0x0019b958
    void OnStdMidi(StdMidiMsg *pMsg);

    // Records one message into mPhrase at its offset in mPhraseBar, with mValue as the value.
    // 0x0019bbd8
    void RecordMuseMsg(MuseMsg *pMsg);

    // Unless mPhrase is already recording the bar of nTick, finishes the phrase in progress and
    // starts a new one for the player there, announcing it with a ClearGemsMsg, a BarStatusMsg,
    // a BeginPhraseCatchMsg, and a PhraseCapturedMsg.
    // 0x0019bce0
    void StartPhrase(int nTick);

    // Gives every held note a NoteMsg ending one tick after the bar, installs mPhrase in the
    // phrase manager at mPhraseBar, and releases it.
    // 0x0019c118
    void FinishPhrase();

    // Sends a SeekerMsg that turns mPlayer's seeker off, unless mPlayer is the stand-in. The bar
    // is not read.
    // 0x0019c368
    void PostSeekerMsg(int nBar);

    // The out-of-line copy of the TrackSelectMsg branch HandleMessage() expands inline.
    // 0x0019d860
    void OnTrackSelect(TrackSelectMsg *pMsg);

    // The out-of-line copy of the InvalidateSeekerMsg branch HandleMessage() expands inline.
    // 0x0019d8f0
    void OnInvalidateSeeker(InvalidateSeekerMsg *pMsg);

    // The out-of-line copy of the SustainNoteMsg branch HandleMessage() expands inline.
    // 0x0019d920
    void OnSustainNote(SustainNoteMsg *pMsg);

    PhraseMgr *mPhraseMgr;            // +0x18
    Quantizer *mQuantizer;            // +0x1c, not read by any recovered routine
    int mTrack;                       // +0x20
    unsigned char mChannel;           // +0x24
    Phrase *mPhrase;                  // +0x28, the phrase in progress, or null
    int mPhraseBar;                   // +0x2c, the bar mPhrase records, -1 at first
    Player *mPlayer;                  // +0x30, g_nullPlayer until a TrackSelectMsg
    std::vector<HeldNote> mHeldNotes; // +0x34
    Mid::MBT mBarTicks;               // +0x40
    const TrackData *mTrackData;      // +0x44
    int mSwitchBanks;                 // +0x48, from configuration codes 0x3a4 and 0x3a1
    float mValue;                     // +0x4c, the axis value recorded with every message
};
