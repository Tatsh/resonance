#pragma once

#include "msg/message.h"

/**
 * Message with a line of script text for a sink to run.
 *
 * `9ScriptMsg` in the RTTI descriptor at `0x00901cd0`, with Message as its one base. Recovery has
 * barely started, and this declaration exists to satisfy the reference from ScriptSink, which is
 * the one recovered reader of the text. Neither Message virtual is implemented here, because
 * nothing reconstructed so far creates a ScriptMsg.
 *
 * mScript is public because ScriptSink reads it directly and the image has no accessor for it. It
 * follows an unrecovered word, so the members below are grouped by access rather than by offset,
 * and each trailing comment records the real offset.
 */
class ScriptMsg : public Message {
public:
    /** Script text to run, or null to fall back on the default text. `+0x08` */
    const char *mScript;

private:
    int mUnknown04; // +0x04
};

/**
 * Identity that ScriptSink compares a message against before running its text.
 *
 * @ghidraAddress 0x006d024c
 */
extern int g_nScriptMsgType;
