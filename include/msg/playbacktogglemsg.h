#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `17PlaybackToggleMsg` in the RTTI descriptor at `0x00901db0`, with Message as its one base. The
 * object is 0x8 bytes and its vtable is at `0x007ce6d8`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). mOn is public because
 * Overlay::OnPlaybackToggle() at `0x0041f9a8` reads it directly with no accessor in the image. It
 * copies the flag into its own state and, when the flag is set, shows `Press the SELECT button to
 * edit`.
 */
class PlaybackToggleMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 316.
     *
     * @return The message.
     * @ghidraAddress 0x003d7248
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001161d0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPlaybackToggleMsgType.
     * @ghidraAddress 0x00116218
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PlaybackToggleMsg`.
     * @ghidraAddress 0x00116228
     */
    virtual const char *Name();

    int mOn; /*!< Non-zero when playback starts, zero when it stops. +0x04 */
};

/**
 * Identity that PlaybackToggleMsg::Type() reports.
 *
 * This word belongs to PlaybackToggleMsg because PlaybackToggleMsg::Type() at `0x00116218` returns
 * it, and the registration at `0x003d9818` passes the same value, 316, as the identity of this
 * class's factory.
 *
 * @ghidraAddress 0x006d026c
 */
extern int g_nPlaybackToggleMsgType;
