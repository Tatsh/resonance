#pragma once

#include "app/msgsink.h"

/**
 * Sink that runs a script line arriving as a message.
 *
 * `ScriptSink` in the RTTI descriptor at `0x00901e60`, single inheritance from `MsgSink`. Its one
 * table at `0x007cee50` has four entries: the compiler-generated accessor, the destructor,
 * `MsgSink::Handle` inherited unchanged, and this class's `HandleMessage`. It adds no virtual of
 * its own and declares no data member, which the destructor confirms by tearing nothing down.
 *
 * The allocation tag on its release path reads `MsgSink` rather than `ScriptSink`, because a tag
 * names the class that declares the operator and this class inherits the pair from its base.
 */
class ScriptSink : public MsgSink {
public:
    /**
     * Release the sink.
     *
     * The compiled routine restores the base table and honours the deleting flag, both of which
     * the compiler generates, so the original body is empty.
     *
     * @ghidraAddress 0x00118a08
     */
    virtual ~ScriptSink();

    /**
     * Run the script line one message names.
     *
     * Ignores every message whose identity is not the one at `0x006d024c`. For that identity it
     * builds an `HxStr` from the message's `+0x08` and passes it to `RunScript`, substituting
     * the program-wide empty string when the pointer is null so the script text is never null.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00118b50
     */
    virtual void HandleMessage(Message *pMsg);
};
