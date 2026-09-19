#pragma once

#include "app/msgsink.h"
#include "msg/message.h"
#include "synth/museparent.h"

/**
 * Synthesiser that turns MUSE notes into voices.
 *
 * `9MuseSynth` in the RTTI descriptor at `0x008f0190`, over MsgSink at offset 0 and MuseParent at
 * offset 4. The primary table is at `0x007e0008` with four entries and the MuseParent subobject
 * table at `0x007dffe8` with three and a `-4` adjustment. The object is 0x30 bytes, which the
 * tagged allocations in `ScoreTrackGraph` and `AxingSTG` both measure.
 *
 * The class is not reconstructed. Only the surface the stage classes use is declared, so that they
 * compile against the real type. Its constructor is at `0x001aa4d8`.
 */
class MuseSynth : public MsgSink, public MuseParent {
public:
    /**
     * @ghidraAddress 0x001aa5d8
     */
    virtual ~MuseSynth();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001ab170
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Register a sink with the source this synthesiser stores at `+0x10`.
     *
     * The routine is a forwarder. It adjusts the receiver by `+0x10` and calls
     * MsgSource::AddSink() directly, which is what places a MsgSource inside the MuseParent
     * subobject rather than at the head of the object. Naming it after the forwarding it performs
     * is an inference: no literal in the image attests the original title.
     *
     * @param pSink The sink to register.
     * @ghidraAddress 0x001ab038
     */
    void AddMuseSink(MsgSink *pSink);

    /**
     * Build the sustain filter this synthesiser feeds through.
     *
     * The routine allocates a SynthSustainer under the tag `MsgSink`, stores it at both `+0x24`
     * and `+0x28`, and points its downstream sink at this object's `+0x0c`. AxingSTG is the only
     * caller. The title is inferred from that body; no literal in the image attests the original.
     *
     * @ghidraAddress 0x001aafb0
     */
    void CreateSustainer();
};
