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
};
