#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901db0`. It has Message as its one base. The object is 0x8 bytes
 * and its vtable is at `0x007ce6d8`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). mOn is public because
 * Overlay::OnPlaybackToggle() at `0x0041f9a8` reads it directly with no accessor in the image. It
 * copies the flag into its own state and, when the flag is set, shows `Press the SELECT button to
 * edit`.
 *
 * The destructor at `0x00116120` is compiler-generated and has no declaration here. The routine
 * at `0x00116158` is a further emission of the type-information accessor.
 */
class PlaybackToggleMsg : public Message {
public:
    /**
     * Construct a message with the flag unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    PlaybackToggleMsg() {
    }

    /**
     * Report a playback toggle.
     *
     * Inline, with no address of its own. Gamer's build at `0x00110ff4` expands it on its stack
     * with its own word at `+0x4c`.
     *
     * @param nOn Non-zero when playback starts.
     */
    explicit PlaybackToggleMsg(int nOn) : mOn(nOn) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 316.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7248
     * @ghidraAddress PAL: 0x0040f148
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x001161d0
     * @ghidraAddress PAL: 0x00116678
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPlaybackToggleMsgType.
     * @ghidraAddress NTSC-U/C: 0x00116218
     * @ghidraAddress PAL: 0x001166c0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PlaybackToggleMsg`.
     * @ghidraAddress NTSC-U/C: 0x00116228
     * @ghidraAddress PAL: 0x001166d0
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
 * @ghidraAddress NTSC-U/C: 0x006d026c
 * @ghidraAddress PAL: 0x00713a04
 */
extern int g_nPlaybackToggleMsgType;
