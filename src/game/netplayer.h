#pragma once

#include "game/player.h"

class TrackSelectPacket;

/**
 * Player driven by a remote machine.
 *
 * `NetPlayer` in the RTTI descriptor at `0x008eef18`, with `Player` as its only base. Its three
 * vtables are at `0x007d0ab0`, `0x007d0a88`, and `0x007d0a60`, each walked to its terminator.
 *
 * The primary table has 21 entries, the same as the base, so this class adds no virtual and only
 * replaces. It replaces exactly two of the base's slots, 4 and 5, where the base returns -1 and 0,
 * and it replaces `DispatchPriv` in its `MsgSink` table. It inherits both `MsgSource` virtuals
 * and every other primary slot, which makes it a thin specialisation rather than a parallel
 * implementation.
 *
 * Two members are recovered, the track and place slots 4 and 5 return, where the base returns -1
 * and 0 instead. The bytes from `+0x2c` to `+0x47` are unrecovered. Its destructor restores the
 * base tables and releases the pointer the base declares at `+0x28`. That work is the inlined base
 * destructor, and the destructor body is empty.
 */
class NetPlayer : public Player {
public:
    /**
     * Construct a player that a remote machine drives.
     *
     * Runs Player's constructor with nId, name, and pAppearance, then stores nTrack in mTrack and
     * clears mPlace. GrooveWorld::AddNetPlayer() is the recovered caller, and it passes its own
     * identifier as both nId and nTrack.
     *
     * @param nId The player's identifier.
     * @param nTrack The track GetTrack() reports until a message replaces it.
     * @param name The player's name.
     * @param pAppearance The appearance the player is drawn with.
     * @ghidraAddress NTSC-U/C: 0x00122f10
     * @ghidraAddress PAL: 0x00123540
     */
    NetPlayer(int nId, int nTrack, const HxStr &name, const FreqAppearance *pAppearance);

    /**
     * @ghidraAddress NTSC-U/C: 0x00125a98
     * @ghidraAddress PAL: 0x00126110
     */
    virtual ~NetPlayer();

    /**
     * Report the track the last selection chose. The base implementation returns -1 instead.
     *
     * @ghidraAddress NTSC-U/C: 0x00125c48
     * @ghidraAddress PAL: 0x001262d0
     */
    virtual int GetTrack();

    /**
     * Report the place the last selection chose. The base implementation returns zero instead.
     *
     * @ghidraAddress NTSC-U/C: 0x00125c50
     * @ghidraAddress PAL: 0x001262d8
     */
    virtual int GetPlace();

    /**
     * Receive one message.
     *
     * Dispatches on `Message::Type()` against two identities, `TrackSelectPacket` and
     * `TrackSelectMsg`, so one handler covers track selection arriving locally and over the
     * network. A packet goes to OnTrackSelectPacket(). A `TrackSelectMsg` whose `+0x10` names
     * this player updates the two cached words, and anything else falls through to the base.
     *
     * @param message The message.
     * @ghidraAddress NTSC-U/C: 0x00125f70
     * @ghidraAddress PAL: 0x00126608
     */
    virtual void DispatchPriv(Message *message);

private:
    /**
     * Passes a selection packet naming this player on to the sinks as a RemoteTrackSelectMsg.
     *
     * @ghidraAddress NTSC-U/C: 0x00122f78
     * @ghidraAddress PAL: 0x001235a8
     */
    void OnTrackSelectPacket(TrackSelectPacket *pPacket);

    // The track and place of a TrackSelectMsg addressed to this player, copied from the message's
    // +0x04 and +0x08 by DispatchPriv.
    int mTrack; // +0x48
    int mPlace; // +0x4c
};
