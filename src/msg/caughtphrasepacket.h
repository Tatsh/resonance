#pragma once

#include <iostream>

#include "game/idableptr.h"
#include "msg/toallothergamesystemspacket.h"

class IBStream;
class OBStream;
class Player;

/**
 * Network packet the game sends between game systems.
 *
 * `18CaughtPhrasePacket` in the RTTI descriptor at `0x009021b0`, with ToAllOtherGameSystemsPacket
 * as its one base. The object is 0x24 bytes and its vtable is at `0x00814608`. The payload comes
 * from the copy constructor at `0x003f37b8`, which Clone() delegates to. The four words Packet
 * provides are declared there rather than here.
 *
 * Print() labels `+0x1c` as `tr` and `+0x20` as `b`, and writes the first through the unsigned
 * integer inserter. The player reference at `+0x14` is transferred but not printed.
 *
 * Save() writes the Packet word at `+0x0c` a second time after the payload, and Load() reads it a
 * second time to match, as GemPacket does.
 *
 * The destructor at `0x003f0440` is compiler-generated and has no declaration here.
 */
class CaughtPhrasePacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Construct a packet with only the player reference and the Packet words set.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    CaughtPhrasePacket() {
    }

    /**
     * Report a phrase caught by a player.
     *
     * Inline, with no address of its own. PhraseMgr::SetPhraseOwner() at `0x001bafa8` expands it
     * on its stack with the manager's word at `+0x30` as the track and the mapped bar.
     *
     * @param pPlayer The player who caught the phrase, or null.
     * @param nTr The track.
     * @param nB The bar.
     */
    CaughtPhrasePacket(Player *pPlayer, unsigned int nTr, int nB)
        : mPlayer(pPlayer), mTr(nTr), mB(nB) {
    }

    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nCaughtPhrasePacketType. Only the player reference is initialised beyond the Packet words.
     *
     * @return The packet.
     * @ghidraAddress 0x003e5200
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f04b8
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCaughtPhrasePacketType.
     * @ghidraAddress 0x003f0530
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CaughtPhrasePacket`.
     * @ghidraAddress 0x003f0540
     */
    virtual const char *Name();

    /**
     * Write ` tr:`, the track, ` b:`, the bar, and a space to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f24a8
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, the player's identifier, the track, the bar, and the Packet word at
     * `+0x0c` again to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e6db0
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the fields back in place in the order Save() wrote them.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e6f00
     */
    virtual void Load(IBStream &stream);

    /**
     * The player the phrase goes to. +0x14
     *
     * PhraseMgr::OnCaughtPhrasePacket() at `0x001ba540` resolves it directly.
     */
    IDablePtr<Player> mPlayer;

    /**
     * The track, labelled `tr` by Print(). +0x1c
     *
     * PhraseMgr::OnCaughtPhrasePacket() compares it with the track the manager serves.
     */
    unsigned int mTr;

    /**
     * The phrase step, labelled `b` by Print(). +0x20
     *
     * PhraseMgr::OnCaughtPhrasePacket() starts its walk of the chained steps there.
     */
    int mB;
};

/**
 * Identity that CaughtPhrasePacket::Type() reports.
 *
 * This word belongs to CaughtPhrasePacket because CaughtPhrasePacket::Type() at `0x003f0530`
 * returns it.
 *
 * @ghidraAddress 0x006d73dc
 */
extern int g_nCaughtPhrasePacketType;
