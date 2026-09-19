#pragma once

#include <vector>

#include "gs/pitcher.h"
#include "msg/message.h"

/**
 * Pitcher that drives the scratch track.
 *
 * `9Scratcher` in the RTTI descriptor at `0x00901fe0`, with Pitcher as its one base. Its three
 * tables are at `0x007e57b8`, `0x007e5790`, and `0x007e5760`, and it overrides exactly the two
 * slots Pitcher leaves pure.
 *
 * Tick() is what fixes mBarDivisor. It divides the elapsed tick count by that member, sends the
 * result through SendSeekerMsg(), and then, while mUnknown54 is set, tests the object at
 * mUnknown40 against the same bar and dispatches a slot on the synthesiser that
 * Globals::GetSynth() returns.
 *
 * Its destructor releases one vector of four-byte elements at `+0x74`, which is the one member of
 * the class the destructor reveals.
 *
 * Only HandleMessage() and Tick() are written. The five routines they dispatch to and the
 * constructor are declared with their addresses and their bodies are not written.
 */
class Scratcher : public Pitcher {
public:
    /**
     * Construct a scratcher.
     *
     * The body is not written.
     *
     * @ghidraAddress 0x001cf988
     */
    Scratcher();

    /**
     * @ghidraAddress 0x001d1bf0
     */
    virtual ~Scratcher();

    /**
     * Advance to the bar the elapsed tick count falls in.
     *
     * Divides the elapsed count by mBarDivisor and sends that bar through SendSeekerMsg(). While
     * mUnknown54 is set and the test at `0x001d79d0` accepts the bar, it then dispatches slot 0 of
     * whatever Globals::GetSynth() returns, with mUnknown58 as one argument. That second half
     * reads three routines whose verbs are unrecovered, which is why the body records the shape
     * rather than the work.
     *
     * @param nElapsedTicks Ticks since the epoch.
     * @return 1 always.
     * @ghidraAddress 0x001d1d68
     */
    virtual int Tick(int nElapsedTicks);

protected:
    /**
     * React to an AxisRegisterMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001cfd20
     */
    void PostNowBarMsg(Message *pMsg);

    /**
     * React to an EraseMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d0038
     */
    void EraseGemRange(Message *pMsg);

    /**
     * React to a TrackSelectMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d0248
     */
    void OnTrackSelect(Message *pMsg);

    /**
     * React to a PitchRiffMsg. The body is not written.
     *
     * @param nUnknown The message's `+0x04`.
     * @param bUnknown Always zero at the one recovered call site.
     * @param nUnknown2 The message's `+0x0c`.
     * @return The value HandleMessage() stores in mUnknown68.
     * @ghidraAddress 0x001d0358
     */
    int OnPitchRiff(int nUnknown, int bUnknown, int nUnknown2);

    /**
     * Send the seeker message for one bar. The body is not written.
     *
     * @param nBar The bar.
     * @ghidraAddress 0x001d08e0
     */
    void SendSeekerMsg(int nBar);

    /**
     * Act on a message.
     *
     * Primary table slot 3.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d0980
     */
    virtual void HandleMessage(Message *pMsg);

private:
    void *mUnknown38; // +0x38
    void *mUnknown3c; // +0x3c
    // The object Tick() tests the current bar against.
    void *mUnknown40; // +0x40
    // Matched against a PitchRiffMsg's `+0x10` and an InvalidateSeekerMsg's `+0x08`, so it
    // identifies the track this Scratcher serves.
    int mUnknown44; // +0x44
    // Divisor that turns an elapsed tick count into a bar index.
    int mBarDivisor; // +0x48
    int mUnknown4c;  // +0x4c
    int mUnknown50;  // +0x50
    // Tick() performs its second half only while this is set.
    int mUnknown54; // +0x54
    // The argument Tick() hands to the synthesiser slot.
    int mUnknown58; // +0x58
    // Matched against a PitchRiffMsg's `+0x08`.
    int mUnknown5c; // +0x5c
    int mUnknown60; // +0x60
    int mUnknown64; // +0x64
    // Result of the PitchRiffMsg handler.
    int mUnknown68; // +0x68
    int mUnknown6c; // +0x6c
    int mUnknown70; // +0x70
    // One vector of four-byte elements, which the destructor releases. Its element type is
    // unrecovered.
    std::vector<int> mUnknown74; // +0x74
};
