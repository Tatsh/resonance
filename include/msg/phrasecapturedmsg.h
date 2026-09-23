#pragma once

#include <iostream>

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `17PhraseCapturedMsg` in the RTTI descriptor at `0x008ef830`, with Message as its one base. The
 * object is 0x28 bytes and its vtable is at `0x008126f0`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). Print() labels five of the
 * nine words: a bar range at `+0x04` and `+0x08`, the track at `+0x14`, the score at `+0x1c`, and
 * the juice at `+0x20`, and it writes the colour name of the player at `+0x18`. The words at
 * `+0x0c`, `+0x10`, and `+0x24` are not printed.
 *
 * mTrack, mPlayer, and mScore are public because Overlay::OnPhraseCaptured() at `0x0042b068`
 * reads them directly with no accessor in the image. It passes mTrack to script template 1005,
 * compares mPlayer with HudTrack::mPlayer, and hands a non-zero mScore to the track display.
 *
 * The destructor at `0x003deaf0` is compiler-generated and has no declaration here.
 */
class PhraseCapturedMsg : public Message {
public:
    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    PhraseCapturedMsg() {
    }

    /**
     * Report a captured phrase.
     *
     * Inline, with no address of its own. SingleCatcher::Slot9() expands it on its stack at
     * `0x001ad750`. The nine arguments are the nine members in declaration order.
     *
     * @param nFirstBar The first bar of the phrase.
     * @param nEndBar The end of the phrase.
     * @param nRunFirstBar The first bar of the caught run.
     * @param nRunEndBar One past the last bar of the run.
     * @param nTrack The track.
     * @param pPlayer The capturing player.
     * @param nScore The score the capture earned.
     * @param nJuice The value SingleCatcher reads from its TrackData.
     * @param bExtendsStreak Non-zero when the capture extends the streak.
     */
    PhraseCapturedMsg(int nFirstBar,
                      int nEndBar,
                      int nRunFirstBar,
                      int nRunEndBar,
                      int nTrack,
                      Player *pPlayer,
                      int nScore,
                      int nJuice,
                      int bExtendsStreak)
        : mFirstBar(nFirstBar), mEndBar(nEndBar), mRunFirstBar(nRunFirstBar),
          mRunEndBar(nRunEndBar), mTrack(nTrack), mPlayer(pPlayer), mScore(nScore), mJuice(nJuice),
          mExtendsStreak(bExtendsStreak) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress 0x003d73d0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003debe0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPhraseCapturedMsgType.
     * @ghidraAddress 0x003dec68
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PhraseCapturedMsg`.
     * @ghidraAddress 0x003dec78
     */
    virtual const char *Name();

    /**
     * Write `b `, the bar range joined by `--`, ` tr# `, the track, ` score `, the score,
     * ` juice `, the juice, a space, and the player's colour name to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003d8448
     */
    virtual void Print(std::ostream &stream);

    /**
     * The first bar of the captured phrase, which Print() writes ahead of `--`. +0x04
     *
     * Public because Gamer's HandleMessage() at `0x00112a80` reads it directly with no accessor in
     * the image.
     */
    int mFirstBar;

private:
    int mEndBar; // +0x08, written after `--` by Print()

public:
    // Public because LocalPlayer::HandleMessage() reads both directly at `0x0011ee88` and
    // `0x0011ef0c`, and the image has no accessor.
    int mRunFirstBar; /*!< The first bar of the caught run. +0x0c */
    int mRunEndBar;   /*!< One past the last bar of the run. +0x10 */
    int mTrack;       /*!< The track the phrase lies on. +0x14 */
    Player *mPlayer;  /*!< The capturing player. +0x18 */
    int mScore;       /*!< The score the capture earned. +0x1c */

    /**
     * The juice the capture earned, a value SingleCatcher reads from its TrackData.
     *
     * Public because Player::AwardCapture() reads it directly at `0x001331ec`, and the image has
     * no accessor. +0x20
     */
    int mJuice;

    /**
     * Non-zero when the capture extends the player's streak of consecutive captures.
     *
     * The inverse of SingleCatcher::Slot9's argument. Public because LocalPlayer::HandleMessage()
     * reads it directly at `0x0011ee9c` and lengthens the streak only when it is set, and the
     * image has no accessor. +0x24
     */
    int mExtendsStreak;
};

/**
 * Identity that PhraseCapturedMsg::Type() reports.
 *
 * This word belongs to PhraseCapturedMsg because PhraseCapturedMsg::Type() at `0x003dec68`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d02a4
 */
extern int g_nPhraseCapturedMsgType;
