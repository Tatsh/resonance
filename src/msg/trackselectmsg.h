#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00902a20`. It has Message as its one base. The object is 0x14 bytes
 * and its vtable is at `0x00812d68`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). Every member is public, and
 * each member's documentation identifies the reader outside the class that accesses it directly.
 *
 * PrintExtra() hands mPosition to Mid::MBT::Print(), and New() initialises it to kMBTInfinity.
 *
 * The destructor at `0x003dc840` is compiler-generated and has no declaration here.
 */
class TrackSelectMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6e90
     * @ghidraAddress PAL: 0x0040ed80
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dc930
     * @ghidraAddress PAL: 0x00414d68
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_dwTrackSelectMsgType.
     * @ghidraAddress NTSC-U/C: 0x003dc990
     * @ghidraAddress PAL: 0x00414dc8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `TrackSelectMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dc9a0
     * @ghidraAddress PAL: 0x00414dd8
     */
    virtual const char *GetName() const;

    /**
     * Write the player's colour name, ` tr#`, the two words at `+0x04` and `+0x08` joined by `/`,
     * a space, and the position to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3ae8
     * @ghidraAddress PAL: 0x00410358
     */
    virtual void PrintExtra(std::ostream &stream) const;

public:
    /**
     * The selected track. Copied into NetPlayer `+0x48` by its handler at `0x00125f70`. +0x04
     *
     * GrooveWorld::CreateRenderer() at `0x0018caa8` writes the player's GetTrack() result here,
     * and LocalPlayer's handler copies it into the player's track.
     */
    int mTrack;

    /**
     * The place on the track. Copied into NetPlayer `+0x4c` by the same handler. +0x08
     *
     * GrooveWorld::CreateRenderer() writes zero here, and the song position and player below.
     * LocalPlayer's handler copies it into the player's place.
     */
    int mPlace;

    /**
     * The song position of the selection. +0x0c
     *
     * Catcher::OnTrackSelect() at `0x001ac550` reads it directly with no accessor in the image,
     * dividing its tick by the catcher's ticks per bar to find the selected bar.
     */
    Mid::MBT mPosition;

    /**
     * Player the message is addressed to.
     *
     * NetPlayer::HandleMessage() compares this member against the receiving player and acts only on
     * a match, which is what types it as a player rather than as a payload word.
     *
     * +0x10
     */
    Player *mPlayer;
};

/**
 * Identity that TrackSelectMsg::Type() reports.
 *
 * This word belongs to TrackSelectMsg because TrackSelectMsg::Type() at `0x003dc990` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d01ec
 * @ghidraAddress PAL: 0x00713984
 */
extern unsigned int g_dwTrackSelectMsgType;
