#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/trackdata.h"
#include "msg/message.h"

/**
 * Chooser of which pitch a guitar track's input resolves to.
 *
 * `11PitchPicker` in the RTTI descriptor at `0x008eef88`, over MsgSink at offset 0 and MsgSource at
 * offset 4. Its primary table is at `0x007e3418` with four entries and its MsgSource subobject
 * table at `0x007e33f0` with four. The object is 0x50 bytes, which AxingSTG's tagged allocation
 * measures.
 *
 * The class is not reconstructed. Only the surface AxingSTG uses is declared, so that it compiles
 * against the real type.
 */
class PitchPicker : public MsgSink, public MsgSource {
public:
    /**
     * @param pTrackData The track description.
     * @ghidraAddress 0x001c29c8
     */
    PitchPicker(const TrackData *pTrackData);

    /**
     * @ghidraAddress 0x001c3e10
     */
    virtual ~PitchPicker();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001c3130
     */
    virtual void HandleMessage(Message *pMsg);
};
