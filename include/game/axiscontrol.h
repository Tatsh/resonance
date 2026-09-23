#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/trackdata.h"
#include "mid/mbt.h"
#include "msg/axisregistermsg.h"
#include "msg/message.h"
#include "msg/trackselectmsg.h"

class AllNotesOffMsg;
class Player;
class StdMidiMsg;

/**
 * Translator of analogue stick movement into track control.
 *
 * `11AxisControl` in the RTTI descriptor at `0x00901ba0`, over MsgSink at offset 0 and MsgSource at
 * offset 4. Its primary table is at `0x007de2b8` with four entries and its MsgSource subobject
 * table at `0x007de290` with four. The object is 0x38 bytes, which AxingSTG's tagged allocation
 * measures.
 *
 * An earlier pass titled this class's constructor `RndSpotShadowCam__Construct`. No descriptor
 * among the 574 in the image bears that title. Slot 0 of the table at `0x007de2b8` addresses the
 * accessor at `0x0019f868`, which guards on the descriptor at `0x00901ba0`, and that is what
 * settles the name.
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
     * @ghidraAddress 0x0019e940
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
     * @ghidraAddress 0x0019ed80
     */
    virtual void HandleMessage(Message *pMsg);

private:
    // Records the stick position. A new coarse position (the value times 1024, divided by 8)
    // sends a NowBarMsg for this track, and during a bend the stick drives the pitch bend.
    // 0x0019ea80
    void OnAxisRegister(AxisRegisterMsg *pMsg);

    // Takes the player a TrackSelectMsg for this track names and, for a real player, sends a
    // NowBarMsg with the current lane. The message's second word is not tested.
    // 0x0019ec10
    void OnTrackSelect(TrackSelectMsg *pMsg);

    // Sends a pitch bend on mChannel whose coarse byte is (nValue + 512) / 8 and whose fine byte
    // is zero.
    // 0x0019ecf0
    void SendPitchBend(int nTick, int nValue);

    // The out-of-line copy of the StdMidiMsg branch HandleMessage() expands inline. A note-on at
    // mSustainTick starts a bend from the stick position, snapped to the centre within 50, and
    // any other note-on ends a bend in progress with a centred pitch bend. The image has no
    // caller of this copy.
    // 0x0019fab0
    void OnStdMidi(StdMidiMsg *pMsg);

    // The out-of-line copy of the AllNotesOffMsg branch HandleMessage() expands inline. A bend in
    // progress ends with a centred pitch bend at the message's tick.
    // 0x0019fb40
    void OnAllNotesOff(AllNotesOffMsg *pMsg);

    int mTrack;            // +0x18
    int mChannel;          // +0x1c, the track's channel byte widened to a word
    int mLane;             // +0x20, the coarse stick position, 64 at first
    int mAxis;             // +0x24, the stick position times 1024, -1 at first
    int mBending;          // +0x28
    int mBendOrigin;       // +0x2c, the stick position the bend started from
    Mid::MBT mSustainTick; // +0x30
    Player *mPlayer;       // +0x34, g_nullPlayer until a TrackSelectMsg
};
