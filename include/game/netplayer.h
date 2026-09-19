#pragma once

#include "game/player.h"

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
     * network. A `TrackSelectMsg` whose `+0x10` names this player updates the two cached words,
     * and anything else falls through to the base.
     *
     * Not reconstructed. The packet branch calls the helper at `0x00122f78`, whose second half
     * builds a further message and is not recovered.
     *
     * @param message The message.
     * @ghidraAddress 0x00125f70
     */
    virtual void HandleMessage(Message *message);

private:
    // The two payload words of a TrackSelectMsg addressed to this player, copied from the
    // message's +0x04 and +0x08 by HandleMessage and reported by Slot4 and Slot5. The base
    // returns -1 and 0 for the same two slots, so a remote player reports what a message
    // announced where a local one computes it.
    int mUnknown48; // +0x48 returned by Slot4
    int mUnknown4c; // +0x4c returned by Slot5
};
