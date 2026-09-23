#pragma once

#include <bitset>

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12BarStatusMsg` in the RTTI descriptor at `0x008ef420`, with Message as its one base. The
 * object is 0x30 bytes and its vtable is at `0x00812660`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). The purpose of each field comes
 * from the Print() override at `0x003d8670`, which streams `b#<bar> tr#<track>` and then one
 * labelled part per flag set in mFlags: the player's colour name for kFieldPlayer, ` enabled` or
 * ` disabled` for kFieldEnabled, ` pow:<n>` for kFieldPowerup, and ` effect:<names>` for
 * kFieldEffects.
 *
 * A flagged field is read through an inline getter that runs Has() for its flag and discards the
 * result before the load. Renderer::OnBarStatus() at `0x0042d068` and Print() both show that shape
 * at every read. Has() is the out-of-line member the getters call, and each getter also has one
 * uncalled out-of-line copy, recorded on its declaration. The bar, the track, mUnknown14, and
 * mFlags are read with no call at all, and the image has no accessor for them. Those four are
 * public.
 *
 * The destructor at `0x003deee0` is compiler-generated and has no declaration here.
 *
 * Clone() copies only as far as `0x2c` of the 0x30 bytes it allocates, so the remaining 4 are
 * either alignment padding or a field the copy omits.
 */
class BarStatusMsg : public Message {
public:
    /** Effect kinds the effect mask records, one bit each. */
    enum { kEffectCount = 13 };

    /**
     * One bit per effect kind.
     *
     * Print() streams the mask through std::operator<<() for a thirteen-bit set at `0x003d9690`,
     * and on this target the set is one eight-byte word.
     */
    typedef std::bitset<kEffectCount> Effects;

    /**
     * Bits of mFlags, one per optional field.
     */
    enum Field {
        kFieldPlayer = 1,  /*!< The player field is set. */
        kFieldEnabled = 2, /*!< The enabled field is set. */
        kFieldPowerup = 4, /*!< The powerup field is set. */
        kFieldEffects = 8, /*!< The effect mask is set. */
    };

    /**
     * Report a bar's player, with no other optional field set.
     *
     * Inline, with no address of its own. AxePhraseMaker::StartPhrase() expands it on its stack at
     * `0x0019bdc4`. mUnknown14 and the effect mask start clear, and mEnabled and mPowerup are
     * left unset.
     *
     * @param nBar The bar.
     * @param nTrack The track.
     * @param pPlayer The player whose track the bar belongs to.
     */
    BarStatusMsg(int nBar, int nTrack, Player *pPlayer)
        : mBar(nBar), mTrack(nTrack), mPlayer(pPlayer), mUnknown14(0), mFlags(kFieldPlayer) {
    }

    /**
     * Start with no optional field set and the effect mask clear.
     *
     * Inline, with no address of its own. New() expands it.
     */
    BarStatusMsg() : mFlags(0) {
    }

    /**
     * Build an empty message on the heap for the message registry.
     *
     * Allocates 0x30 bytes under the tag `MSG`. The unit's static initialiser registers it.
     *
     * @return The new message.
     * @ghidraAddress 0x003d7440
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003defd0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nBarStatusMsgType.
     * @ghidraAddress 0x003df050
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `BarStatusMsg`.
     * @ghidraAddress 0x003df060
     */
    virtual const char *Name();

    /**
     * Write the bar, the track, and each set optional field.
     *
     * Streams `b#<bar> tr#<track>`, then the player's colour name, ` enabled` or ` disabled`,
     * ` pow:<powerup>`, and ` effect:<mask>` for each Field bit set in mFlags.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003d8670
     */
    virtual void Print(std::ostream &stream);

    /**
     * Report whether one optional field is set.
     *
     * @param nField The Field bit to test.
     * @return 1 when the bit is set in mFlags, and 0 otherwise.
     * @ghidraAddress 0x003df1f8
     */
    int Has(int nField);

    /**
     * Report the player field.
     *
     * @return The player whose track the bar belongs to.
     * @ghidraAddress 0x003df130
     */
    Player *GetPlayer() {
        Has(kFieldPlayer); // Yes, the binary discards this result.
        return mPlayer;
    }

    /**
     * Report the enabled field.
     *
     * @return Non-zero when the bar is enabled.
     * @ghidraAddress 0x003df160
     */
    int GetEnabled() {
        Has(kFieldEnabled); // Yes, the binary discards this result.
        return mEnabled;
    }

    /**
     * Report the powerup field.
     *
     * @return The powerup kind on the bar.
     * @ghidraAddress 0x003df190
     */
    int GetPowerup() {
        Has(kFieldPowerup); // Yes, the binary discards this result.
        return mPowerup;
    }

    /**
     * Report the effect mask.
     *
     * @return One bit per effect kind.
     * @ghidraAddress 0x003df1c0
     */
    Effects GetEffects() {
        Has(kFieldEffects); // Yes, the binary discards this result.
        return mEffects;
    }

    int mBar;   /*!< The bar the status describes. +0x04 */
    int mTrack; /*!< The track the status describes. +0x08 */

private:
    Player *mPlayer; // +0x0c
    int mEnabled;    // +0x10

public:
    /** Word Renderer::OnBarStatus() passes to AppTunnel::OnBarChanged(). +0x14 */
    int mUnknown14;

private:
    int mPowerup;     // +0x18
    Effects mEffects; // +0x20

public:
    int mFlags; /*!< The Field bits of the fields that are set. +0x28 */
};

/**
 * Identity that BarStatusMsg::Type() reports.
 *
 * This word belongs to BarStatusMsg because BarStatusMsg::Type() at `0x003df050` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d02b4
 */
extern int g_nBarStatusMsgType;
