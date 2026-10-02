#pragma once

#include "msg/message.h"
#include "os/hxstr.h"

/**
 * Message with a line of script text for a sink to run.
 *
 * `9ScriptMsg` in the RTTI descriptor at `0x00901cd0`, with Message as its one base. The object
 * is 0xc bytes and its vtable is at `0x007d6730`. It overrides Clone(), Type(), and Name(), and
 * retains Message's Print(), Save(), and Load().
 *
 * The payload is one HxStr. Clone() copy-constructs `+0x04` through HxStr::HxStr(const HxStr &),
 * the destructor at `0x0015a5f0` releases the buffer at `+0x08` with the inlined HxStr
 * destructor, and New() zeroes both words, so the text ScriptSink reads at `+0x08` is the
 * string's buffer. That destructor is compiler-generated and has no declaration here, and the
 * registration unit's copies of it and of Clone() are marked as duplicates in the program.
 */
class ScriptMsg : public Message {
public:
    /** Wrap an empty script. */
    ScriptMsg() = default;

    /**
     * Wrap a line of script text.
     *
     * Inline. ScriptCmd's Execute() at `0x0015a408` expands it on the stack.
     *
     * @param script The text.
     */
    explicit ScriptMsg(const HxStr &script) : mScript(script) {
    }

    /**
     * Produce a message with an empty script on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress 0x003d7158
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0015a6c8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nScriptMsgType.
     * @ghidraAddress 0x0015a768
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ScriptMsg`.
     * @ghidraAddress 0x0015a778
     */
    virtual const char *Name();

    /**
     * Script text to run. An empty string makes the reader fall back on the default text.
     *
     * Public because ScriptSink reads the buffer directly and the image has no accessor for it.
     *
     * +0x04
     */
    HxStr mScript;
};

/**
 * Identity that ScriptSink compares a message against before running its text.
 *
 * ScriptMsg::Type() at `0x0015a768` returns it.
 *
 * @ghidraAddress 0x006d024c
 */
extern int g_nScriptMsgType;
