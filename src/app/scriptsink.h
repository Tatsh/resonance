#pragma once

#include "app/msgsink.h"

// Globals is declared in `app/globals.h`. That header includes this one for its own member, and the
// two headers cannot include each other.
class Globals;

/**
 * Message sink that runs the script text a message supplies.
 *
 * Its RTTI descriptor is at `0x00901e60`. It has MsgSink as its one base. The object is eight
 * bytes. The compiler-generated vptr lands at offset 0, and the Globals back pointer occupies
 * `+0x04`. Globals::InitServices() at `0x001170d0` allocates eight bytes, stores the Globals
 * pointer at `+0x04`, and writes the vptr, and CreateInstance() does the same. The table at
 * `0x007cee50` runs GetTypeInfo, the destructor, the inherited MsgSink::Dispatch(), then the
 * override below.
 *
 * Globals creates the single instance in Init() and destroys it in Shutdown(). The sink accepts
 * every message, acts only on one identity, and is registered broadly.
 * The allocation tag on both construction paths reads `MsgSink` rather than `ScriptSink`, because
 * a tag names the class that declares the operator and this class inherits the pair from its base.
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
     * Build a sink on the heap.
     *
     * The body is the same allocation and construction Globals::InitServices() inlines. No caller
     * is recovered. The name is inferred.
     *
     * @param pOwner The globals the sink belongs to.
     * @return The new sink.
     * @ghidraAddress NTSC-U/C: 0x00118c00
     * @ghidraAddress PAL: 0x00119150
     */
    static ScriptSink *CreateInstance(Globals *pOwner);

protected:
    /**
     * Run the message's script text when the message is a ScriptMsg.
     *
     * Ignores every message whose identity is not g_nScriptMsgType. For a ScriptMsg it builds an
     * HxStr from ScriptMsg::mScript, substituting the program-wide empty string for a null buffer,
     * and passes it to RunScript().
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x00118b50
     * @ghidraAddress PAL: 0x00119088
     */
    virtual void DispatchPriv(Message *pMsg);

private:
    /**
     * Run the message's script text without testing its identity first.
     *
     * The receiver is unused, and DispatchPriv() inlines a copy of this body rather than calling
     * it.
     *
     * @ghidraAddress NTSC-U/C: 0x00118ad0
     * @ghidraAddress PAL: 0x00118fe8
     */
    void RunMessageScript(Message *pMsg);

    Globals *mGlobals; // +0x04
};
