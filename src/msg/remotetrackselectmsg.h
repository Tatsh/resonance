#pragma once

#include <iostream>

#include "mid/tick.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901d00`. It has Message as its one base. The object is 0x14 bytes
 * and its vtable is at `0x00812d20`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered. NetPlayer builds the message field by field on the stack, and TrackSelector reads
 * the fields back, so all four are public.
 *
 * The layout matches TrackSelectMsg's. PrintExtra() hands `+0x0c` to Sch::Tick::Print() and writes
 * the colour name of the player at `+0x10`, which types both.
 *
 * The destructor at `0x003dca18` is compiler-generated and has no declaration here.
 */
class RemoteTrackSelectMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6ed0
     * @ghidraAddress PAL: 0x0040edc0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dcb08
     * @ghidraAddress PAL: 0x00414f40
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nRemoteTrackSelectMsgType.
     * @ghidraAddress NTSC-U/C: 0x003dcb68
     * @ghidraAddress PAL: 0x00414fa0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `RemoteTrackSelectMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dcb78
     * @ghidraAddress PAL: 0x00414fb0
     */
    virtual const char *GetName() const;

    /**
     * Write the player's colour name, ` tr#`, the two words at `+0x04` and `+0x08` joined by `/`,
     * a space, and the position to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3bd0
     * @ghidraAddress PAL: 0x00410460
     */
    virtual void PrintExtra(std::ostream &stream) const;

public:
    /**
     * The selected track. NetPlayer's packet handler at `0x00122f78` writes it from the packet's
     * track, and TrackSelector::DispatchPriv() reads it. +0x04
     */
    int mTrack;

    /**
     * The place on the track. NetPlayer's packet handler at `0x00122f78` writes it from the
     * packet's place, and TrackSelector::DispatchPriv() reads it. +0x08
     */
    int mPlace;

    /**
     * The song position of the selection. NetPlayer's packet handler at `0x00122f78` writes it,
     * and TrackSelector::DispatchPriv() reads it. +0x0c
     */
    Sch::Tick mPosition;

    /**
     * The player that selected. NetPlayer's packet handler at `0x00122f78` writes it, and
     * TrackSelector::DispatchPriv() reads it. +0x10
     */
    Player *mPlayer;
};

/**
 * Identity that RemoteTrackSelectMsg::Type() reports.
 *
 * This word belongs to RemoteTrackSelectMsg because RemoteTrackSelectMsg::Type() at `0x003dcb68`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d01f4
 * @ghidraAddress PAL: 0x0071398c
 */
extern int g_nRemoteTrackSelectMsgType;
