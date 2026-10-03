#pragma once

#include <vector>

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/trackdata.h"
#include "mid/mbt.h"
#include "msg/message.h"
#include "msg/multimusemsg.h"
#include "msg/stdmidimsg.h"
#include "msg/sustainnotemsg.h"

class Player;

/**
 * Chooser of which pitch a guitar track's input resolves to.
 *
 * Its RTTI descriptor is at `0x008eef88`. It is built over MsgSink at offset 0 and MsgSource at
 * offset 4. Its primary table is at `0x007e3418` with four entries and its MsgSource subobject
 * table at `0x007e33f0` with four, which adjusts `this` by `-4`. The object is 0x50 bytes, which
 * AxingSTG's tagged allocation measures.
 *
 * A note-on picks a pitch from the harmony in force, offset by the axis position across the range
 * of the current riff, and records the pairing so the matching note-off releases the same pitch.
 * Notes at the tick of the last SustainNoteMsg reuse the pitch a sustained note already received.
 *
 * The destructor at `0x001c3e10` is implicitly declared. It restores the base tables, destroys the
 * two note vectors and MsgSource's vector, and releases the object under MsgSink's tag.
 */
class PitchPicker : public MsgSink, public MsgSource {
public:
    /**
     * @param pTrackData The track description. The constructor copies its track number.
     * @ghidraAddress NTSC-U/C: 0x001c29c8
     * @ghidraAddress PAL: 0x001c8810
     */
    PitchPicker(const TrackData *pTrackData);

    /**
     * Act on a message.
     *
     * Slot 3. A MultiMuseMsg goes to OnMsg(), a StdMidiMsg to the branch OnStdMidi()
     * copies, and a SustainNoteMsg to PostSustainNoteMsg(). An AxisRegisterMsg from mPlayer sets
     * mAxis to its value times 1024, and a TrackSelectMsg for this track with a zero second word
     * sets mPlayer. Every other message is discarded.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001c3130
     * @ghidraAddress PAL: 0x001c8f78
     */
    virtual void DispatchPriv(Message *pMsg);

    /**
     * A note number paired with the pitch it was played as.
     *
     * The element type of both note vectors, eight bytes with the pitch in the low byte of the
     * second word. Every build clears the whole record with memset() before storing the two fields.
     */
    struct NoteMapping {
        int mNote;            /*!< The note number the input carried. +0x00 */
        unsigned char mPitch; /*!< The pitch it was played as. +0x04 */
    };

private:
    // Records the lowest and highest note of the riff the message carries in mRiffLow and
    // mRiffHigh, through a stack RiffRangeFinder that visits every message of the sequence.
    // NTSC-U/C: 0x001c2c60, PAL: 0x001c8aa8
    void OnMsg(const MultiMuseMsg &msg);

    // Records the message's tick in mSustainTick and sends a SustainNoteMsg at that tick for the
    // pitch GetSustainPitch() reports for its note.
    // NTSC-U/C: 0x001c2d40, PAL: 0x001c8b88
    void PostSustainNoteMsg(SustainNoteMsg *pMsg);

    // Picks the pitch of a note-on (reusing the sustained pitch at mSustainTick), records the
    // pairing in mHeldNotes, and sends the note-on at that pitch.
    // NTSC-U/C: 0x001c2dd0, PAL: 0x001c8c18
    void PostNoteOn(int nTick, unsigned char nStatus, unsigned char nNote, unsigned char nVelocity);

    // Clears mSustainNotes unless nTick is mSustainTick, then sends a note-off with zero velocity
    // for the first held pairing of nNote and removes that pairing. A note without a pairing
    // sends nothing.
    // NTSC-U/C: 0x001c2f08, PAL: 0x001c8d50
    void PostNoteOff(int nTick, unsigned char nStatus, unsigned char nNote);

    // Returns the pitch mSustainNotes pairs with nNote, or picks one and records the pairing.
    // NTSC-U/C: 0x001c3060, PAL: 0x001c8ea8
    unsigned char GetSustainPitch(int nTick, unsigned char nNote);

    // Routes a note-off to PostNoteOff() and a note-on to PostNoteOn(), and sends every other
    // channel message on unchanged. The out-of-line copy of the branch DispatchPriv() expands
    // inline.
    // NTSC-U/C: 0x001c43b0, PAL: 0x001ca1f8
    void OnStdMidi(StdMidiMsg *pMsg);

    // Returns nNote unchanged when no harmony is in force at nTick. Otherwise the axis position
    // maps linearly from 0..1023 onto the offsets that move the riff's range onto the harmony's,
    // and the offset note snaps to the nearer harmony note.
    // NTSC-U/C: 0x001c4488, PAL: 0x001ca2d0
    unsigned char PickPitch(int nTick, unsigned char nNote);

    const TrackData *mTrackData;            // +0x18
    int mAxis;                              // +0x1c, the axis position scaled to 0..1024
    int mReserved;                          // +0x20, never read or written
    std::vector<NoteMapping> mHeldNotes;    // +0x24
    std::vector<NoteMapping> mSustainNotes; // +0x30
    Mid::MBT mSustainTick;                  // +0x3c
    int mRiffLow;                           // +0x40
    int mRiffHigh;                          // +0x44
    int mTrack;                             // +0x48, copied from TrackData::mIndex
    Player *mPlayer;                        // +0x4c, NullPlayer::sInstance until a TrackSelectMsg
};
