#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/trackdata.h"
#include "msg/message.h"

class Player;
class StdMidiMsg;

/**
 * Producer of the gems a guitar or vocal track presents, in its later form.
 *
 * `14AxeNewGemMaker` in the RTTI descriptor at `0x008efc20`, over MsgSink at offset 0 and MsgSource
 * at offset 4. Its primary table is at `0x007dee40` with four entries and its MsgSource subobject
 * table at `0x007dee18` with four. The object is 0x2c bytes, which the tagged allocations in
 * AxingSTG and VoxingSTG both measure.
 *
 * The maker turns the live MIDI of mPlayer into gems as it arrives. A note-on becomes a DurGemMsg
 * 80 ticks long, and the sustain pedal on controller 46 opens and closes a SusGemMsg strip.
 *
 * The destructor at `0x001a2e48` is implicitly declared. It destroys MsgSource's vector and
 * releases the object under MsgSink's tag.
 */
class AxeNewGemMaker : public MsgSink, public MsgSource {
public:
    /**
     * @param pTrackData The track description. The constructor copies its track and retains it.
     * @ghidraAddress 0x001a2da0
     */
    explicit AxeNewGemMaker(const TrackData *pTrackData);

    /**
     * Act on a message.
     *
     * A TrackSelectMsg for this track with a zero second word installs its player. An
     * AxisRegisterMsg from mPlayer stores its value, whatever its track. An AxisFXMsg is ignored,
     * and a StdMidiMsg goes to PostGemMessages().
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001a46e0
     */
    virtual void HandleMessage(Message *pMsg);

private:
    // A note-on sends a DurGemMsg for mPlayer from the note to 80 ticks after it, blended by
    // AxeOldGemMaker::BlendForAxis() of mValue. On controller 46, a zero value with no strip open
    // opens one under AxeOldGemMaker::NextStripId(), and a non-zero value closes the open strip
    // with a SusGemMsg whose mStop is 2.
    // 0x001a2f18
    void PostGemMessages(StdMidiMsg *pMsg);

    int mTrack;                  // +0x18, copied from TrackData::mUnknown04
    const TrackData *mTrackData; // +0x1c, not read by any recovered routine
    int mStripId;                // +0x20, the open sustain strip, or zero
    float mValue;                // +0x24, the axis value, 0.5 at first
    Player *mPlayer;             // +0x28, g_nullPlayer until a TrackSelectMsg
};
