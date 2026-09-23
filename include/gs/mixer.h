#pragma once

#include <vector>

#include "app/msgsink.h"
#include "msg/message.h"
#include "msg/stdmidimsg.h"

class MsgSource;
class Player;
class TrackSelectMsg;
class TracksOnMsg;

/**
 * Per-track MIDI mixer that sits between a track graph and the synthesiser.
 *
 * `5Mixer` in the RTTI descriptor at `0x008f0770`, with MsgSink as its one base, so the inherited
 * vptr sits at offset 0. The object is 0x58 bytes, which the allocations in
 * BGTrackGraph::CreateMixer() and ScoreTrackGraph's constructor both measure. Its vtable is at
 * `0x007df900` and runs four entries.
 *
 * It is a MIDI filter on one channel. Every message it emits is a StdMidiMsg built on the stack
 * and handed to mOutput, with the channel taken from mChannel. A StdMidiMsg arriving with a
 * control-change status goes through ApplyControlChange().
 *
 * SetGainFactor() is what fixes the four factors. It multiplies all four together and divides by
 * 127 cubed, which normalises a product of four 7-bit values back into a 7-bit value, and sends
 * that as controller 11. Four independent gains therefore scale one channel's expression.
 *
 * This class is not a light manager, and the three members previously titled
 * EmitSectionBrightnessPulse(), SetColorByte(), and UpdateColorFromGameState() are the pan
 * emitter, the gain setter, and the gain recomputation. The name comes from the RTTI descriptor
 * and is not the invented `Rnd::MidiLight` that the type-function harvest recorded for the
 * accessor.
 */
class Mixer : public MsgSink {
public:
    /**
     * Construct a mixer on one channel.
     *
     * The routine reads mOwnsPan from configuration code 0x398, mUnknown10 from code 0x399, and
     * mTrackLevels from code 0x39f, sets mLevel and all four gain factors to 127, mUnknown50 to
     * -1, and mSelection to g_nullPlayer, and zeroes mUnknown28 twice over. mUnknown54 takes
     * PlayMap::Slot9() of Globals::GetPlayMap(). It does not write mOutput.
     *
     * @param nTrack The track this mixer serves. BGTrackGraph passes -1, which matches no track.
     * @param nChannel The MIDI channel every message it emits is sent on.
     * @ghidraAddress 0x001a7110
     */
    Mixer(int nTrack, unsigned char nChannel);

    /**
     * @ghidraAddress 0x001a8130
     */
    virtual ~Mixer();

    /**
     * Set one of the four gain factors and send the combined level.
     *
     * Recomputes the product of all four factors divided by 127 cubed. When the result differs
     * from the level already sent, and the channel is not muted, it stores the result and sends
     * it as controller 11 on mChannel.
     *
     * The compiler emits its divide-by-zero check against the constant divisor, so the trap it
     * guards can never fire.
     *
     * @param nIndex Which factor, 0 through 3.
     * @param nFactor The factor, 0 through 127.
     * @ghidraAddress 0x001a73a8
     */
    void SetGainFactor(int nIndex, unsigned char nFactor);

    /**
     * Mute or unmute the channel.
     *
     * A change to unmuted sends the level already computed as controller 11, and a change to
     * muted sends zero. A call that does not change the flag sends nothing.
     *
     * @param bMuted Non-zero to mute.
     * @ghidraAddress 0x001a7490
     */
    void SetMuted(int bMuted);

    /**
     * Act on one control-change message.
     *
     * A 41-entry jump table over controller numbers 0x0a through 0x32 decides what happens.
     * Controllers 0x2f through 0x32 set gain factors 0 through 3, controller 0x2e drives
     * SetMuted(), controller 0x0b is discarded because the mixer sends its own expression,
     * controller 0x0a is discarded while mOwnsPan is set and forwarded otherwise, and everything
     * else including a controller outside the table's range is forwarded to mOutput unchanged.
     *
     * The member is inline. HandleMessage() has its own emission of the whole body with its own
     * copy of the jump table, which is why two addresses exist for one member.
     *
     * @param pMsg The control-change message.
     * @ghidraAddress 0x001a8390
     */
    void ApplyControlChange(StdMidiMsg *pMsg);

    /**
     * Send the pan for the current section.
     *
     * Derives a three-bit index from mTrack less mLastSection and maps it through a six-entry jump
     * table to one of 0, 0, 0x20, 0x40, 0x60, and 0x7f, which it sends as controller 10. An index
     * the table does not reach sends zero.
     *
     * @ghidraAddress 0x001a72b8
     */
    void SendPan();

protected:
    /**
     * React to a TrackSelectMsg.
     *
     * A message for mTrack installs its player as mSelection and recomputes the gain. Then, for
     * any track, a selection whose Player::Slot2() reports zero takes the message's track as
     * mLastSection and, when mOwnsPan is set, sends the pan.
     *
     * @param pMsg The TrackSelectMsg.
     * @ghidraAddress 0x001a75d8
     */
    void OnTrackSelect(TrackSelectMsg *pMsg);

    /**
     * React to a TracksOnMsg.
     *
     * Stores the message's bar in mUnknown50 and its track count in mLevelIndex, and recomputes
     * the gain.
     *
     * @param pMsg The TracksOnMsg.
     * @ghidraAddress 0x001a76d0
     */
    void OnTracksOn(TracksOnMsg *pMsg);

    /**
     * Recompute the gain from the game state and send it as gain factor 3.
     *
     * While GameStats::mCompleted is set the factor is 115. Otherwise a real mSelection gets
     * 127, and the stand-in gets 127 less the low byte of mTrackLevels[mLevelIndex].
     *
     * Inline. OnTrackSelect() and OnTracksOn() expand it, and the image keeps this out-of-line
     * copy.
     *
     * @ghidraAddress 0x001a82f0
     */
    void RecomputeGain();

    /**
     * Act on a message.
     *
     * Table slot 3.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001a7780
     */
    virtual void HandleMessage(Message *pMsg);

public:
    /**
     * The sink every message this mixer emits is sent to.
     *
     * The constructor does not write it. Public because BGTrackGraph::AttachMixerToSynth() stores
     * it directly at `0x001404c4`, and the image has no accessor. +0x04
     */
    MsgSink *mOutput;

private:
    unsigned char mChannel; // +0x08
    int mTrack;             // +0x0c the constructor's first argument
    // Read from configuration code 0x399 as one byte. No recovered routine reads it back.
    unsigned char mUnknown10; // +0x10
    // Whether the mixer generates the pan itself, read from configuration code 0x398. Two readers
    // agree on the sense. OnTrackSelect() sends the pan only when it is set, and an incoming pan
    // controller is discarded only when it is set.
    int mOwnsPan; // +0x14
    // Section the pan index is measured against.
    int mLastSection; // +0x18
    // The player of the selected track, g_nullPlayer until OnTrackSelect() installs one.
    Player *mSelection; // +0x1c
    // Combined gain most recently sent as controller 11. The constructor sets it to 127.
    unsigned char mLevel; // +0x20
    // Sixteen bytes the constructor zeroes twice over, once before the configuration reads and
    // once after. No recovered routine reads any of them.
    unsigned char mUnknown28[16]; // +0x28
    // Whether the channel is muted. SetGainFactor() computes the level but does not send it while
    // this is set, and SetMuted() sends zero on the way in and mLevel on the way out.
    int mMuted; // +0x38
    // The four gain factors, each 0 through 127 and each set to 127 by the constructor.
    // SetGainFactor() multiplies all four and divides by 127 cubed to produce mLevel.
    unsigned char mGainFactors[4]; // +0x3c
    int mLevelIndex;               // +0x40 index into mTrackLevels, taken from a TracksOnMsg
    // Per-track levels, read from configuration code 0x39f by the constructor.
    std::vector<int> mTrackLevels; // +0x44
    int mUnknown50;                // +0x50 set to -1 by the constructor, then a TracksOnMsg's bar
    // PlayMap::Slot9() of the play map, read once by the constructor. No recovered routine reads
    // it back.
    int mUnknown54; // +0x54
};
