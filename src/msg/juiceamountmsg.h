#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef840`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x008121e0`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x003e07b8` is compiler-generated and has no declaration here.
 */
class JuiceAmountMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7818
     * @ghidraAddress PAL: 0x0040f718
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e08a8
     * @ghidraAddress PAL: 0x00418d00
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nJuiceAmountMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e08f8
     * @ghidraAddress PAL: 0x00418d50
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `JuiceAmountMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e0908
     * @ghidraAddress PAL: 0x00418d60
     */
    virtual const char *GetName() const;

    /**
     * Write the player to a diagnostic stream through Player::Print().
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e41d8
     * @ghidraAddress PAL: 0x0041c408
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Report the player's juice.
     *
     * Forwards to Player::GetJuice(). Overlay::OnJuiceAmount() calls it at `0x0041f3ac`. The name
     * is inferred from the accessor it forwards to.
     *
     * @return The juice.
     * @ghidraAddress NTSC-U/C: 0x003e4178
     * @ghidraAddress PAL: 0x0041c3a8
     */
    int GetJuice();

    /**
     * Report the player's juice as a fraction of mMaxJuice.
     *
     * Both values are converted to float before the division. Six sites call it, among them
     * Overlay::OnJuiceAmount() twice, TnlArena::DispatchPriv(), and AppTunnel::DispatchPriv().
     * The name is inferred.
     *
     * @return The juice divided by mMaxJuice.
     * @ghidraAddress NTSC-U/C: 0x003e4198
     * @ghidraAddress PAL: 0x0041c3c8
     */
    float GetJuiceFraction();

public:
    /**
     * Player the message is about.
     *
     * Player::AnnounceState() at `0x0012f788` writes the player here before sending. That write
     * types this member as a player rather than as a payload word.
     *
     * +0x04
     */
    Player *mPlayer;

    /** The player's mMaxJuice, clamped by the same site to a maximum of 800. +0x08 */
    int mMaxJuice;
};

/**
 * Identity that JuiceAmountMsg::Type() reports.
 *
 * This word belongs to JuiceAmountMsg because JuiceAmountMsg::Type() at `0x003e08f8` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0334
 * @ghidraAddress PAL: 0x00713acc
 */
extern int g_nJuiceAmountMsgType;
