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
 * Its RTTI descriptor is at `0x009021b0`. It has ToAllOtherGameSystemsPacket as its one base. The
 * object is 0x24 bytes and its vtable is at `0x00814608`. The payload comes from the copy
 * constructor at `0x003f37b8`. Clone() delegates to it. The four words Packet provides are
 * declared there rather than here.
 *
 * PrintExtra() labels `+0x1c` as `tr` and `+0x20` as `b`, and writes the first through the
 * unsigned integer inserter. The player reference at `+0x14` is transferred but not printed.
 *
 * saveGuts() writes the Packet word at `+0x0c` a second time after the payload, and restoreGuts()
 * reads it a second time to match, as GemPacket does.
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
     * @ghidraAddress NTSC-U/C: 0x003e5200
     * @ghidraAddress PAL: 0x0041d498
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003f04b8
     * @ghidraAddress PAL: 0x00428ac0
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCaughtPhrasePacketType.
     * @ghidraAddress NTSC-U/C: 0x003f0530
     * @ghidraAddress PAL: 0x00428b38
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CaughtPhrasePacket`.
     * @ghidraAddress NTSC-U/C: 0x003f0540
     * @ghidraAddress PAL: 0x00428b48
     */
    virtual const char *GetName() const;

    /**
     * Write ` tr:`, the track, ` b:`, the bar, and a space to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f24a8
     * @ghidraAddress PAL: 0x0042a9f0
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the Packet words, the player's identifier, the track, the bar, and the Packet word at
     * `+0x0c` again to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e6db0
     * @ghidraAddress PAL: 0x0041f090
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the fields back in place in the order saveGuts() wrote them.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e6f00
     * @ghidraAddress PAL: 0x0041f1e0
     */
    virtual void restoreGuts(IBStream &stream);

    /**
     * The player the phrase goes to. +0x14
     *
     * PhraseMgr::OnCaughtPhrasePacket() at `0x001ba540` resolves it directly.
     */
    IDablePtr<Player> mPlayer;

    /**
     * The track, labelled `tr` by PrintExtra(). +0x1c
     *
     * PhraseMgr::OnCaughtPhrasePacket() compares it with the track the manager serves.
     */
    unsigned int mTr;

    /**
     * The phrase step, labelled `b` by PrintExtra(). +0x20
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
 * @ghidraAddress NTSC-U/C: 0x006d73dc
 * @ghidraAddress PAL: 0x0071ab7c
 */
extern int g_nCaughtPhrasePacketType;
