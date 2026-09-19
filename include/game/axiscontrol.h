#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/trackdata.h"
#include "msg/message.h"

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
 * The class is not reconstructed. Only the surface AxingSTG uses is declared, so that it compiles
 * against the real type.
 */
class AxisControl : public MsgSink, public MsgSource {
public:
    /**
     * @param pTrackData The track description.
     * @ghidraAddress 0x0019e940
     */
    AxisControl(const TrackData *pTrackData);

    /**
     * @ghidraAddress 0x0019f6a8
     */
    virtual ~AxisControl();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x0019ed80
     */
    virtual void HandleMessage(Message *pMsg);
};
