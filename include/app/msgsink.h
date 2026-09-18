#pragma once

#include "app/message.h"

/**
 * Receiver of engine messages.
 *
 * `7MsgSink` in the RTTI descriptor at `0x0086f780`, with no base and no data members, so the
 * compiler-generated vptr lands at offset 0 and the whole subobject is four bytes. The destructor
 * is declared ahead of both message virtuals, so the table at `0x007ccc40` runs GetTypeInfo, the
 * destructor, Handle(), then HandleMessage(). Around forty classes derive from the class, among
 * them GameManager, GrooveWorld, Player, Synth, MsgQueue, and ScriptSink.
 *
 * The class is entirely inline, so g++ emitted its vtable and both out-of-line bodies into the
 * translation unit that first needed them, and the linker retained a copy in around forty
 * further objects. The canonical vtable is the one at `0x007ccc40`.
 *
 * A sender calls Handle(), whose default implementation passes the message to HandleMessage().
 * Every derived class in the image overrides HandleMessage() and inherits Handle() unchanged.
 */
class MsgSink {
public:
    /**
     * @ghidraAddress 0x00105120
     */
    virtual ~MsgSink();

    /**
     * Accept a message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00105158
     */
    virtual void Handle(Message *pMsg);

protected:
    /**
     * Act on a message.
     *
     * Only Handle() dispatches this, so it is protected rather than public.
     *
     * @param pMsg The message.
     */
    virtual void HandleMessage(Message *pMsg) = 0;
};
