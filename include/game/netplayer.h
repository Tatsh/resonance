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
 * and it replaces `HandleMessage` in its `MsgSink` table. It inherits both `MsgSource` virtuals
 * and every other primary slot, which makes it a thin specialisation rather than a parallel
 * implementation.
 *
 * Two of its own members are recovered, the values slots 4 and 5 return, where the base returns
 * -1 and 0 instead. Nothing between `+0x2c` and `+0x47` is recovered. Its destructor restores the
 * base tables and releases the pointer the base declares at `+0x28`, all of which is the inlined
 * base destructor, so its own body is empty.
 *
 * A slot whose verb is unrecovered keeps its table index as its title, because the index is part
 * of the layout.
 */
class NetPlayer : public Player {
public:
    /**
     * Construct a player that a remote machine drives.
     *
     * Runs Player's constructor with nId, name, and pAppearance, then stores nUnknown48 in
     * mUnknown48 and clears mUnknown4c. GrooveWorld::AddNetPlayer() is the recovered caller,
     * and it passes its own identifier as both nId and nUnknown48.
     *
     * @param nId The player's identifier.
     * @param nUnknown48 The value Slot4() reports until a message replaces it.
     * @param name The player's name.
     * @param pAppearance The appearance the player is drawn with.
     * @ghidraAddress 0x00122f10
     */
    NetPlayer(int nId, int nUnknown48, const HxStr &name, const FreqAppearance *pAppearance);

    /** @ghidraAddress 0x00125a98 */
    virtual ~NetPlayer();

    /**
     * Slot 4. Replaces the base implementation, which returns -1.
     *
     * @ghidraAddress 0x00125c48
     */
    virtual int Slot4();

    /**
     * Slot 5. Replaces the base implementation, which returns zero.
     *
     * @ghidraAddress 0x00125c50
     */
    virtual int Slot5();

    /**
     * Receive one message.
     *
     * Dispatches on `Message::Type()` against two identities, `TrackSelectPacket` and
     * `TrackSelectMsg`, so one handler covers track selection arriving locally and over the
     * network. A packet goes to OnTrackSelectPacket(). A `TrackSelectMsg` whose `+0x10` names
     * this player updates the two cached words, and anything else falls through to the base.
     *
     * @param message The message.
     * @ghidraAddress 0x00125f70
     */
    virtual void HandleMessage(Message *message);

private:
    // 0x00122f78
    // Passes a selection packet naming this player on to the sinks as a RemoteTrackSelectMsg.
    void OnTrackSelectPacket(TrackSelectPacket *pPacket);

    // The two payload words of a TrackSelectMsg addressed to this player, copied from the
    // message's +0x04 and +0x08 by HandleMessage and reported by Slot4 and Slot5. The base
    // returns -1 and 0 for the same two slots, so a remote player reports what a message
    // announced where a local one computes it.
    int mUnknown48; // +0x48 returned by Slot4
    int mUnknown4c; // +0x4c returned by Slot5
};
