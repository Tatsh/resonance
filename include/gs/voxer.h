#pragma once

#include "gs/pitcher.h"
#include "msg/message.h"

/**
 * Pitcher that drives the vocal track.
 *
 * `5Voxer` in the RTTI descriptor at `0x008f0830`, with Pitcher as its one base. Its three tables
 * are at `0x007e61a8`, `0x007e6180`, and `0x007e6150`, and it overrides exactly the two slots
 * Pitcher leaves pure: HandleMessage() in the primary table and Tick() in the TickTask table.
 *
 * The constructor takes four arguments, the fourth in `t0`, and fixes the layout of the derived
 * part. It constructs the TickTask base with a period of 1920 ticks, which is one measure at 480
 * ticks per quarter note.
 *
 * Only HandleMessage() is written. The five routines it dispatches to and the Tick() override are
 * declared with their addresses and their bodies are not written.
 */
class Voxer : public Pitcher {
public:
    /**
     * @param pTrack The first argument, whose `+0x34` is copied into mUnknown48.
     * @param nUnknown3c The second argument.
     * @param pClock The clock the TickTask base is posted against.
     * @param pOwner The fourth argument, which arrives in `t0`. Its `+0x04` is copied into
     *               mUnknown44 and its `+0x08` byte into mChannel.
     * @ghidraAddress 0x001d81b8
     */
    Voxer(void *pTrack, int nUnknown3c, Sch::TickClock *pClock, void *pOwner);

    /**
     * @ghidraAddress 0x001d9dd0
     */
    virtual ~Voxer();

    /**
     * @ghidraAddress 0x001d8dc8
     */
    virtual int Tick(int nElapsedTicks);

protected:
    /**
     * React to a PitchRiffMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d8370
     */
    void OnPitchRiff(Message *pMsg);

    /**
     * React to a StopRiffMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d8430
     */
    void OnStopRiff(Message *pMsg);

    /**
     * React to a TrackSelectMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d8840
     */
    void OnTrackSelect(Message *pMsg);

    /**
     * React to an EraseMsg. The body is not written.
     *
     * @param nBar The message's `+0x08` divided by mUnknown48.
     * @param nUnknown The message's `+0x10`.
     * @param bUnknown Always 1 at the one recovered call site.
     * @ghidraAddress 0x001d8638
     */
    void OnErase(int nBar, int nUnknown, int bUnknown);

    /**
     * React to an InvalidateSeekerMsg. The body is not written.
     *
     * @param nUnknown The message's `+0x04`.
     * @ghidraAddress 0x001d8fb0
     */
    void OnInvalidateSeeker(int nUnknown);

    /**
     * Act on a message.
     *
     * Primary table slot 3.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001d9050
     */
    virtual void HandleMessage(Message *pMsg);

private:
    void *mUnknown38; // +0x38 the constructor's first argument
    int mUnknown3c;   // +0x3c the constructor's second argument
    void *mUnknown40; // +0x40 the constructor's fourth argument
    // Matched against an EraseMsg's `+0x0c` and an InvalidateSeekerMsg's `+0x08`, so it identifies
    // the track this Voxer serves.
    int mUnknown44; // +0x44 copied from mUnknown40's `+0x04`
    // Divisor that turns an elapsed tick count into a bar index.
    int mUnknown48;         // +0x48 copied from mUnknown38's `+0x34`
    unsigned char mChannel; // +0x4c copied from mUnknown40's `+0x08`
    // Matched against an EraseMsg's `+0x04`, and defaulted to the static instance at `0x0066f930`
    // that Mixer also defaults its own selection to.
    void *mUnknown50; // +0x50
    int mUnknown54;   // +0x54 cleared on construction
    int mUnknown58;   // +0x58 cleared on construction
    int mUnknown5c;   // +0x5c set to -1 on construction
    int mUnknown60;   // +0x60 set to -1 on construction
    int mUnknown68;   // +0x68 cleared on construction as one doubleword with mUnknown6c
    int mUnknown6c;   // +0x6c
};
