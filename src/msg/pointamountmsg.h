#pragma once

#include <iostream>

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008eecc8`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x00812198`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). PrintExtra() dispatches
 * Player::Print() through the word at `+0x04`, which types it. Player::AddScore() fills the word
 * at `+0x08` with the capped score ceiling. mPlayer is public because Overlay::OnPointAmount() at
 * `0x0042aff0` reads it directly with no accessor in the image, comparing it with
 * HudBadge::mPlayer.
 *
 * The destructor at `0x003e0958` is compiler-generated and has no declaration here.
 */
class PointAmountMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7850
     * @ghidraAddress PAL: 0x0040f750
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e0a48
     * @ghidraAddress PAL: 0x00418ea0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPointAmountMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e0a98
     * @ghidraAddress PAL: 0x00418ef0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PointAmountMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e0aa8
     * @ghidraAddress PAL: 0x00418f00
     */
    virtual const char *GetName() const;

    /**
     * Write the player to a diagnostic stream through Player::Print().
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e4138
     * @ghidraAddress PAL: 0x0041c368
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Report the player's score.
     *
     * Forwards to Player::GetScore(). Overlay::HandleMessage() at `0x00420848` and
     * Overlay::OnPointAmount() at `0x0042b040` call it. The name is inferred from the accessor it
     * forwards to.
     *
     * @return The score.
     * @ghidraAddress NTSC-U/C: 0x003e40d8
     * @ghidraAddress PAL: 0x0041c308
     */
    int GetScore();

    /**
     * Report the player's score as a fraction of mMaxScore.
     *
     * Both values are converted to float before the division. The image lists no caller. The name
     * is inferred.
     *
     * @return The score divided by mMaxScore.
     * @ghidraAddress NTSC-U/C: 0x003e40f8
     * @ghidraAddress PAL: 0x0041c328
     */
    float GetScoreFraction();

    Player *mPlayer; /*!< The player whose score changed. +0x04 */

    /**
     * The score ceiling, capped at 800.
     *
     * Public because Player::AddScore() writes it directly at `0x0012f89c`, and the image has no
     * accessor. +0x08
     */
    int mMaxScore;
};

/**
 * Identity that PointAmountMsg::Type() reports.
 *
 * This word belongs to PointAmountMsg because PointAmountMsg::Type() at `0x003e0a98` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d033c
 * @ghidraAddress PAL: 0x00713ad4
 */
extern int g_nPointAmountMsgType;
