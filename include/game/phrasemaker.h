#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

/**
 * Base of the per-instrument phrase makers.
 *
 * `11PhraseMaker` in the RTTI descriptor at `0x008f0300`, over MsgSink at offset 0 and MsgSource at
 * offset 4. AxePhraseMaker derives from it at offset 0.
 *
 * The class is not reconstructed and no member of it is recovered. It is declared so that
 * AxePhraseMaker records the base the RTTI attests rather than omitting it. HandleMessage() is
 * declared because the derived table supplies it and a class that omitted it could not be
 * instantiated.
 */
class PhraseMaker : public MsgSink, public MsgSource {
public:
    virtual ~PhraseMaker();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     */
    virtual void HandleMessage(Message *pMsg);
};
