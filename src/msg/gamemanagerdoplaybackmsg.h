#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef790`. It has Message as its one base. The object is 0x4 bytes
 * and its vtable is at `0x007f5e18`. New(), Clone(), and MetLoadGameScreen::OnFadeInDone() store
 * the table. The allocation in New() and the allocation in Clone() report the same size.
 *
 * The deleting destructor at `0x002918c0` is implicitly declared. It restores Message's table and
 * frees the object under the class tag, which is what the compiler generates.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * This class adds no field of its own, and the size is exactly the size of the base, which is what
 * fixes the size of the base.
 */
class GameManagerDoPlaybackMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 436.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7c30
     * @ghidraAddress PAL: 0x0040fb48
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x00291970
     * @ghidraAddress PAL: 0x002ad888
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGameManagerDoPlaybackMsgType.
     * @ghidraAddress NTSC-U/C: 0x002919a8
     * @ghidraAddress PAL: 0x002ad8c0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GameManagerDoPlaybackMsg`.
     * @ghidraAddress NTSC-U/C: 0x002919b8
     * @ghidraAddress PAL: 0x002ad8d0
     */
    virtual const char *Name();
};

/**
 * Identity that GameManagerDoPlaybackMsg::Type() reports.
 *
 * This word belongs to GameManagerDoPlaybackMsg because GameManagerDoPlaybackMsg::Type() at
 * `0x002919a8` returns it, and the registration at `0x003d9818` passes the same value, 436, as the
 * identity of this class's factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d03c4
 * @ghidraAddress PAL: 0x00713b5c
 */
extern int g_nGameManagerDoPlaybackMsgType;
