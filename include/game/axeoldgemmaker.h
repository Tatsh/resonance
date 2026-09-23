#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/trackdata.h"
#include "mid/mbt.h"
#include "msg/message.h"

class NoteMsg;
class Phrase;
class PhraseMsg;
class StdMidiMsg;

/**
 * Counter AxeOldGemMaker::NextStripId() issues sustain strip identities from.
 *
 * Zero at start. Its only readers are NextStripId() and the copy of it that
 * AxeNewGemMaker::PostGemMessages() expands.
 *
 * @ghidraAddress 0x00683f70
 */
extern int g_nNextStripId;

/**
 * Producer of the gems a guitar or vocal track presents, in its earlier form.
 *
 * `14AxeOldGemMaker` in the RTTI descriptor at `0x008ef220`, over MsgSink at offset 0 and MsgSource
 * at offset 4. Its primary table is at `0x007dedf0` with four entries and its MsgSource subobject
 * table at `0x007dedc8` with four. The object is 0x28 bytes, which the tagged allocations in
 * AxingSTG and VoxingSTG both measure.
 *
 * Both classes build one of these and one AxeNewGemMaker, and the pair coexists rather than one
 * replacing the other. Which of the two a given track reaches is not recovered.
 *
 * The maker replays a PhraseMsg's sequence through its own HandleMessage(), advancing mPosition to
 * each entry. Every NoteMsg becomes a DurGemMsg from mPosition, and a sustain-pedal release and
 * press on controller 46 bound a DurGemMsg of their own.
 *
 * The destructor at `0x001a3f90` is implicitly declared. It destroys MsgSource's vector and
 * releases the object under MsgSink's tag.
 *
 * PostDurGemMsg(), OnStdMidi(), and OnPhrase() are declared and not written. The first two set
 * DurGemMsg's private word at `+0x18`, and OnPhrase() reads PhraseMsg's private members.
 */
class AxeOldGemMaker : public MsgSink, public MsgSource {
public:
    /**
     * @param pTrackData The track description. The constructor copies its track.
     * @ghidraAddress 0x001a31c8
     */
    explicit AxeOldGemMaker(const TrackData *pTrackData);

    /**
     * Act on a message.
     *
     * A PhraseMsg goes to OnPhrase(), a NoteMsg to PostDurGemMsg(), and a StdMidiMsg to
     * OnStdMidi(). Every other message is ignored.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001a47a8
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Issue the next sustain strip identity.
     *
     * Inline. AxeNewGemMaker::PostGemMessages() expands it, and Scratcher calls the out-of-line
     * copy. The title is inferred.
     *
     * @return The identity, starting at zero.
     * @ghidraAddress 0x001a4560
     */
    static int NextStripId() {
        return g_nNextStripId++;
    }

    /**
     * Report the gem blend for a pitch step.
     *
     * PhraseMgr::PostDurGemMsg() and Scratcher are the recovered callers. The title is inferred.
     *
     * @param nStep The step, from -3 to 3.
     * @return 0.7, 0.8, 0.9, 0.5, 0.3, 0.2, or 0.1 for steps -3 through 3, and 0 for any other
     *         step.
     * @ghidraAddress 0x001a4578
     */
    static float BlendForStep(int nStep);

    /**
     * Map an axis value between 0 and 1 onto the gem blend, 0.2 to 0.8.
     *
     * Inline, computed in double precision. AxeNewGemMaker::PostGemMessages() and PostDurGemMsg()
     * expand it. The image has no caller of the out-of-line copy. The title is inferred.
     *
     * @param flValue The axis value.
     * @return The blend.
     * @ghidraAddress 0x001a4638
     */
    static float BlendForAxis(float flValue) {
        return static_cast<float>((flValue * kAxisBlendScale) + kAxisBlendBase);
    }

private:
    static constexpr double kAxisBlendScale = 0.6;
    static constexpr double kAxisBlendBase = 0.2;

    // Sends a DurGemMsg from mPosition, lasting the note's length less 60 ticks and at least
    // 60, blended by BlendForAxis() of the phrase's value at the position within its bar.
    // 0x001a32f0
    void PostDurGemMsg(NoteMsg *pMsg);

    // On controller 46, a zero value records mPosition as the sustain start when none is held,
    // and a non-zero value sends a DurGemMsg from the sustain start to mPosition at blend 0.5
    // and clears the start.
    // 0x001a34e8
    void OnStdMidi(StdMidiMsg *pMsg);

    // Replays every entry of the phrase's sequence through HandleMessage() with mPosition
    // advanced to the entry, then clears mPhrase.
    // 0x001a3600
    void OnPhrase(PhraseMsg *pMsg);

    int mTrack;             // +0x18, copied from TrackData::mUnknown04
    Mid::MBT mPosition;     // +0x1c, the position of the entry being replayed
    Phrase *mPhrase;        // +0x20, the phrase being replayed, or null
    Mid::MBT mSustainStart; // +0x24, zero while the pedal is not held
};
