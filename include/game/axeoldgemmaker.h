#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/trackdata.h"
#include "msg/message.h"

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
 * The class is not reconstructed. Only the surface the stage classes use is declared, so that they
 * compile against the real type.
 */
class AxeOldGemMaker : public MsgSink, public MsgSource {
public:
    /**
     * @param pTrackData The track description.
     * @ghidraAddress 0x001a31c8
     */
    AxeOldGemMaker(const TrackData *pTrackData);

    /**
     * @ghidraAddress 0x001a3f90
     */
    virtual ~AxeOldGemMaker();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001a47a8
     */
    virtual void HandleMessage(Message *pMsg);
};
