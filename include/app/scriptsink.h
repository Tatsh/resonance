#pragma once

#include "app/msgsink.h"

// Globals is declared in `app/globals.h`, which includes this header for its own member, so the
// two headers cannot include each other.
class Globals;

/**
 * Message sink that runs the script text a message supplies.
 *
 * `10ScriptSink` in the RTTI descriptor at `0x00901e60`, with MsgSink as its one base. The object
 * is eight bytes: the compiler-generated vptr lands at offset 0 and the Globals back pointer
 * occupies `+0x04`. The table at `0x007cee50` runs GetTypeInfo, the destructor, the inherited
 * MsgSink::Handle(), then the override below.
 *
 * Globals creates the single instance in Init() and destroys it in Shutdown(). The sink accepts
 * every message and acts only on one identity, so it is registered broadly and filters itself.
 *
 * The Globals back pointer is private, because no recovered routine reads it. The constructor
 * stores it and nothing else recovered so far uses it.
 */
class ScriptSink : public MsgSink {
public:
    /**
     * @param pOwner The globals this sink belongs to.
     */
    ScriptSink(Globals *pOwner);

    /**
     * @ghidraAddress 0x00118a08
     */
    virtual ~ScriptSink();

protected:
    /**
     * Run the message's script text when the message is a ScriptMsg.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00118b50
     */
    virtual void HandleMessage(Message *pMsg);

private:
    // Run the message's script text without testing its identity first. The receiver is unused,
    // and HandleMessage() inlines a copy of this body rather than calling it. 0x00118ad0
    void RunMessageScript(Message *pMsg);

    Globals *mGlobals; // +0x04
};
