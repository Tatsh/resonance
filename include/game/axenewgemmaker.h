#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/trackdata.h"
#include "msg/message.h"

/**
 * Producer of the gems a guitar or vocal track presents, in its later form.
 *
 * `14AxeNewGemMaker` in the RTTI descriptor at `0x008efc20`, over MsgSink at offset 0 and MsgSource
 * at offset 4. Its primary table is at `0x007dee40` with four entries and its MsgSource subobject
 * table at `0x007dee18` with four. The object is 0x2c bytes, which the tagged allocations in
 * AxingSTG and VoxingSTG both measure.
 *
 * The class is not reconstructed. Only the surface the stage classes use is declared, so that they
 * compile against the real type.
 */
class AxeNewGemMaker : public MsgSink, public MsgSource {
public:
    /**
     * @param pTrackData The track description.
     * @ghidraAddress 0x001a2da0
     */
    AxeNewGemMaker(const TrackData *pTrackData);

    /**
     * @ghidraAddress 0x001a2e48
     */
    virtual ~AxeNewGemMaker();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001a46e0
     */
    virtual void HandleMessage(Message *pMsg);
};
