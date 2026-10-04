#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/trackdata.h"
#include "mid/tick.h"
#include "msg/axisregistermsg.h"
#include "msg/message.h"
#include "msg/trackselectmsg.h"

class AllNotesOffMsg;
class Player;
class StdMidiMsg;

/**
 * Translator of analogue stick movement into track control.
 *
 * Its RTTI descriptor is at `0x00901ba0`. It is built over MsgSink at offset 0 and MsgSource at
 * offset 4. Its primary table is at `0x007de2b8` with four entries and its MsgSource subobject
 * table at `0x007de290` with four. The object is 0x38 bytes, which AxingSTG's tagged allocation
 * measures.
 *
 * Slot 0 of the table at `0x007de2b8` addresses the accessor at `0x0019f868`. The accessor guards
 * on the descriptor at `0x00901ba0`, and the descriptor settles the class name.
 *
 * The stick's position reaches the class as an AxisRegisterMsg. The coarse position becomes a
 * NowBarMsg lane for the display, and a note played on a sustained tick bends the pitch with the
 * stick until the next note or an AllNotesOffMsg.
 *
 * The destructor at `0x0019f6a8` is implicitly declared.
 */
class AxisControl : public MsgSink, public MsgSource {
public:
    /**
     * @param pTrackData The track description. The constructor copies its track and channel.
     * @ghidraAddress NTSC-U/C: 0x0019e940
     * @ghidraAddress PAL: 0x001a46a8
     */
    AxisControl(const TrackData *pTrackData);

    /**
     * Act on a message.
     *
     * Slot 3. An AxisRegisterMsg goes to OnAxisRegister() and a TrackSelectMsg to OnTrackSelect().
     * An AxisFXMsg is ignored. A SustainNoteMsg records its tick in mSustainTick. A note-on at that
     * tick starts a bend from the current stick position, snapped to the centre within 50, and any
     * other note-on, like an AllNotesOffMsg, ends a bend in progress with a centred pitch bend.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x0019ed80
     * @ghidraAddress PAL: 0x001a4ae8
     */
    virtual void DispatchPriv(Message *pMsg);

private:
    /**
     * Records the stick position.
     *
     * A new coarse position (the value times 1024, divided by 8) sends a NowBarMsg for this track,
     * and during a bend the stick drives the pitch bend.
     *
     * @ghidraAddress NTSC-U/C: 0x0019ea80
     * @ghidraAddress PAL: 0x001a47e8
     */
    void OnAxisRegister(AxisRegisterMsg *pMsg);

    /**
     * Takes the player a TrackSelectMsg for this track names and, for a real player, sends a
     * NowBarMsg with the current lane.
     *
     * The message's second word is not tested.
     *
     * @ghidraAddress NTSC-U/C: 0x0019ec10
     * @ghidraAddress PAL: 0x001a4978
     */
    void OnTrackSelect(TrackSelectMsg *pMsg);

    /**
     * Sends a pitch bend on mChannel whose coarse byte is (nValue + 512) / 8 and whose fine byte is
     * zero.
     *
     * @ghidraAddress NTSC-U/C: 0x0019ecf0
     * @ghidraAddress PAL: 0x001a4a58
     */
    void SendPitchBend(int nTick, int nValue);

    /**
     * The out-of-line copy of the StdMidiMsg branch DispatchPriv() expands inline.
     *
     * A note-on at mSustainTick starts a bend from the stick position, snapped to the centre within
     * 50, and any other note-on ends a bend in progress with a centred pitch bend. The image has no
     * caller of this copy.
     *
     * @ghidraAddress NTSC-U/C: 0x0019fab0
     * @ghidraAddress PAL: 0x001a5818
     */
    void OnStdMidi(StdMidiMsg *pMsg);

    /**
     * The out-of-line copy of the AllNotesOffMsg branch DispatchPriv() expands inline.
     *
     * A bend in progress ends with a centred pitch bend at the message's tick.
     *
     * @ghidraAddress NTSC-U/C: 0x0019fb40
     * @ghidraAddress PAL: 0x001a58a8
     */
    void OnAllNotesOff(AllNotesOffMsg *pMsg);

    int mTrack;             // +0x18
    int mChannel;           // +0x1c, the track's channel byte widened to a word
    int mLane;              // +0x20, the coarse stick position, 64 at first
    int mAxis;              // +0x24, the stick position times 1024, -1 at first
    int mBending;           // +0x28
    int mBendOrigin;        // +0x2c, the stick position the bend started from
    Sch::Tick mSustainTick; // +0x30
    Player *mPlayer;        // +0x34, NullPlayer::sInstance until a TrackSelectMsg
};
