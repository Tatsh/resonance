#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008eed38`. It has Message as its one base. The object is 0x8 bytes
 * and its vtable is at `0x00811b68`. The members below are the whole of the class.
 *
 * The payload layout comes from the run of field copies in Clone().
 *
 * The destructor at `0x003e2b30` is compiler-generated and has no declaration here.
 */
class IsRecordingMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7d48
     * @ghidraAddress PAL: 0x0040fc60
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e2c20
     * @ghidraAddress PAL: 0x0041b0c0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nIsRecordingMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e2c68
     * @ghidraAddress PAL: 0x0041b108
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `IsRecordingMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e2c78
     * @ghidraAddress PAL: 0x0041b118
     */
    virtual const char *Name();

    /**
     * Write `IsRecordingMsg ` and the word at `+0x04` to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e44b0
     * @ghidraAddress PAL: 0x0041c6e0
     */
    virtual void Print(std::ostream &stream);

    /**
     * Non-zero when the game is being restored from a recording.
     *
     * GameManagerImpl::Load() sends 1 and GameManagerImpl::OnBeginGameLocal() sends 0, each to
     * the front end's renderer. Public because both write it directly. +0x04
     */
    int mIsRecording;
};

/**
 * Identity that IsRecordingMsg::Type() reports.
 *
 * This word belongs to IsRecordingMsg because IsRecordingMsg::Type() at `0x003e2c68` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d03ec
 * @ghidraAddress PAL: 0x00713b84
 */
extern int g_nIsRecordingMsgType;
