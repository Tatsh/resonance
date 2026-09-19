#pragma once

#include "gs/pitcher.h"
#include "msg/message.h"

/**
 * Pitcher that drives a note track.
 *
 * `11NotePitcher` in the RTTI descriptor at `0x008eef68`, with Pitcher as its one base. Its three
 * tables are at `0x007e14a0`, `0x007e1478`, and `0x007e1448`, and it overrides exactly the two
 * slots Pitcher leaves pure.
 *
 * It ignores an EraseOffMsg rather than forwarding it, which is the one message the three Pitcher
 * subclasses treat differently from each other.
 *
 * Only HandleMessage() and Tick() are written. The five routines they dispatch to and the
 * constructor are declared with their addresses and their bodies are not written. The five titles
 * come from the message each routine posts rather than from the message it receives, which is the
 * naming the program already had and is retained here.
 */
class NotePitcher : public Pitcher {
public:
    /**
     * Construct a note pitcher.
     *
     * The body is not written.
     *
     * @ghidraAddress 0x001b1ce0
     */
    NotePitcher();

    /**
     * @ghidraAddress 0x001b39c0
     */
    virtual ~NotePitcher();

    /**
     * Advance to the bar the elapsed tick count falls in.
     *
     * Divides the elapsed count by mBarDivisor and hands the result to PostSeekerMsgSecond() with
     * a second argument of zero.
     *
     * @param nElapsedTicks Ticks since the epoch.
     * @return 1 always.
     * @ghidraAddress 0x001b3b98
     */
    virtual int Tick(int nElapsedTicks);

protected:
    /**
     * React to a PitchRiffMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b1f10
     */
    void PostPitchMsg(Message *pMsg);

    /**
     * React to an EraseMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b20b0
     */
    void PostAllNotesOffMsg(Message *pMsg);

    /**
     * React to a TrackSelectMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b22f0
     */
    void PostSeekerMsg(Message *pMsg);

    /**
     * Post the phrase-captured message. The body is not written.
     *
     * No caller is recovered. It is declared because the program already titles it and the address
     * belongs to this class.
     *
     * @ghidraAddress 0x001b2400
     */
    void PostPhraseCapturedMsg();

    /**
     * Post the seeker message for one bar. The body is not written.
     *
     * Both an InvalidateSeekerMsg and Tick() reach it, the first with the message's `+0x04` and
     * the second with the bar it computed. The second argument is zero at both call sites.
     *
     * @param nBar The bar.
     * @param bUnknown Zero at both recovered call sites.
     * @ghidraAddress 0x001b2710
     */
    void PostSeekerMsgSecond(int nBar, int bUnknown);

    /**
     * Act on a message.
     *
     * Primary table slot 3.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b3bd0
     */
    virtual void HandleMessage(Message *pMsg);

private:
    // Matched against an InvalidateSeekerMsg's `+0x08`, so it identifies the track this pitcher
    // serves.
    int mUnknown40; // +0x40
    // Divisor that turns an elapsed tick count into a bar index.
    int mBarDivisor; // +0x50
};
