#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

/**
 * Manager of the jam effects a track applies to its voices.
 *
 * `13JamEffectsMgr` in the RTTI descriptor at `0x008f0490`, over MsgSink at offset 0 and MsgSource
 * at offset 4. The object is 0x34 bytes, which the tagged allocations in PitchingSTG and VoxingSTG
 * both measure. Both build one only when the game manager reports play mode 2, and both then hand
 * it to the phrase player.
 *
 * The class is not reconstructed. Only the surface the stage classes use is declared, so that they
 * compile against the real type. Its constructor is at `0x001a5020` and takes five arguments, the
 * last two arriving in t0 and t1.
 */
class JamEffectsMgr : public MsgSink, public MsgSource {
public:
    virtual ~JamEffectsMgr();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Switch every effect on or off from one bit of a mask.
     *
     * PhrasePlayer::PlayBar() passes the step value of each bar. For each effect of the vector at
     * `+0x28`, the effect's slot 4 reports a bit index, which is taken modulo 64, and the effect's
     * slot 5 receives whether that bit of the mask is set. The bit test goes through a
     * `std::bitset` reference built on the stack. The body is not written, and the title is
     * inferred.
     *
     * @param nMask The step's mask, one bit per effect.
     * @ghidraAddress 0x001a56d8
     */
    void ApplyStepMask(long nMask);
};
