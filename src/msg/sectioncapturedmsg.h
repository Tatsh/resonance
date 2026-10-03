#pragma once

#include <iostream>

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008efcf0`. It has Message as its one base. The object is 0x18 bytes
 * and its vtable is at `0x008126a8`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). PrintExtra() labels a bar
 * range at `+0x04` and `+0x08` and the track at `+0x0c`, and it writes the colour name of the
 * player at `+0x10`. The word at `+0x14` is not printed. The last four members are public because
 * AppTunnel's section handler at `0x00448530` reads them directly with no accessor in the image.
 * It scales mEndBar by the 1920 ticks of a bar.
 *
 * The destructor at `0x003ded28` is compiler-generated and has no declaration here.
 */
class SectionCapturedMsg : public Message {
public:
    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    SectionCapturedMsg() {
    }

    /**
     * Report a captured section.
     *
     * Inline, with no address of its own. SingleCatcher::CapturePhrase() expands it on its stack at
     * `0x001ad79c`. The five arguments are the five members in declaration order.
     *
     * @param nFirstBar The first bar of the section.
     * @param nEndBar The end of the section.
     * @param nTrack The track.
     * @param pPlayer The capturing player.
     * @param nAutoCatch Non-zero when the section was caught automatically.
     */
    SectionCapturedMsg(int nFirstBar, int nEndBar, int nTrack, Player *pPlayer, int nAutoCatch)
        : mFirstBar(nFirstBar), mEndBar(nEndBar), mTrack(nTrack), mPlayer(pPlayer),
          mAutoCatch(nAutoCatch) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7408
     * @ghidraAddress PAL: 0x0040f308
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dee18
     * @ghidraAddress PAL: 0x00417270
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nSectionCapturedMsgType.
     * @ghidraAddress NTSC-U/C: 0x003dee80
     * @ghidraAddress PAL: 0x004172d8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `SectionCapturedMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dee90
     * @ghidraAddress PAL: 0x004172e8
     */
    virtual const char *GetName() const;

    /**
     * Write `b `, the bar range joined by `--`, ` tr# `, the track, a space, and the player's
     * colour name to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003d8578
     * @ghidraAddress PAL: 0x00410950
     */
    virtual void PrintExtra(std::ostream &stream) const;

private:
    int mFirstBar; // +0x04

public:
    int mEndBar;     /*!< The end of the bar range. +0x08 */
    int mTrack;      /*!< The track. +0x0c */
    Player *mPlayer; /*!< The capturing player. +0x10 */
    int mAutoCatch;  /*!< Non-zero for an automatic catch. AppTunnel reduces it to 0 or 1. +0x14 */
};

/**
 * Identity that SectionCapturedMsg::Type() reports.
 *
 * This word belongs to SectionCapturedMsg because SectionCapturedMsg::Type() at `0x003dee80`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d02ac
 * @ghidraAddress PAL: 0x00713a44
 */
extern int g_nSectionCapturedMsgType;
