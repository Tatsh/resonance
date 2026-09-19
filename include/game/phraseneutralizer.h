#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

/**
 * Neutraliser of the phrases a catching track has already captured.
 *
 * `18PhraseNeutralizer` in the RTTI descriptor at `0x008ff3f0`, over MsgSource at offset 0 and
 * MsgSink at offset 20. Its primary table is at `0x007e2e88` with four entries and its MsgSink
 * subobject table at `0x007e2e60`. MsgSource coming first is why CatchingSTG's wiring adjusts by
 * `+0x14` when it registers this object as a sink and guards the adjustment against a null
 * pointer. The object is 0x3c bytes, which CatchingSTG's tagged allocation measures.
 *
 * The class is not reconstructed. Only the surface CatchingSTG uses is declared, so that it
 * compiles against the real type. Its constructor is at `0x001c0918`.
 */
class PhraseNeutralizer : public MsgSource, public MsgSink {
public:
    /**
     * @ghidraAddress 0x001c1478
     */
    virtual ~PhraseNeutralizer();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     */
    virtual void HandleMessage(Message *pMsg);
};
