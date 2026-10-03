#pragma once

#include "app/activefilter.h"
#include "app/filterlover.h"
#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

class AxisFXMsg;
class Globals;
class TrackData;

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Stick-driven controller effect for a guitar track.
 *
 * Its RTTI descriptor is at `0x00902a40`. It is built over MsgSink at offset 0, MsgSource at offset
 * 4, and FilterLover at offset 0x18. The primary table is at `0x007dd820`, the MsgSource table at
 * `0x007dd7f8`, and the FilterLover table at `0x007dd7d8`. No routine in the image calls the
 * constructor, so the class is compiled but never built. The last member is at `+0x50`.
 *
 * The stick position of an AxisFXMsg feeds the embedded ActiveFilter, whose smoothed output
 * arrives back through OnFilterValue() as a controller value. While a riff is playing the
 * controller change goes to the sinks at every new value.
 *
 * The destructor at `0x0019b138` is implicitly declared. It destroys mFilter, frees MsgSource's
 * vector, and releases the object under MsgSink's tag.
 */
class AxeFX : public MsgSink, public MsgSource, public FilterLover {
public:
    /**
     * @param pGlobals The globals whose song clock drives the filter and dates each message.
     * @param pTrackData The track description. The constructor copies its channel.
     * @ghidraAddress NTSC-U/C: 0x0019aa20
     * @ghidraAddress PAL: 0x001a0788
     */
    AxeFX(Globals *pGlobals, const TrackData *pTrackData);

    /**
     * Take a new smoothed stick value.
     *
     * Slot 2 of the FilterLover table. The value maps to a controller value of 127 minus the
     * value times 128. A new controller value is stored, and sent while mPlaying is set.
     *
     * @param flValue The filter's output, from 0 to 1.
     * @ghidraAddress NTSC-U/C: 0x0019b4e0
     * @ghidraAddress PAL: 0x001a1248
     */
    virtual void OnFilterValue(float flValue);

    /**
     * Act on a message.
     *
     * Slot 3. An AxisFXMsg sets the filter's target to its value, a MultiMuseMsg starts a riff
     * (mPlaying set and the controller sent), and an AllNotesOffMsg ends it.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x0019b538
     * @ghidraAddress PAL: 0x001a12a0
     */
    virtual void HandleMessage(Message *pMsg);

private:
    // Sends controller mController on mChannel with mValue, dated at the song clock's tick.
    // NTSC-U/C: 0x0019abd8, PAL: 0x001a0940
    void SendController();

    // The out-of-line copy of the AxisFXMsg branch HandleMessage() expands inline.
    // NTSC-U/C: 0x0019b498, PAL: 0x001a1200
    void OnAxisFX(AxisFXMsg *pMsg);

    // The out-of-line copy of the MultiMuseMsg branch HandleMessage() expands inline.
    // NTSC-U/C: 0x0019b4b8, PAL: 0x001a1220
    void OnMultiMuse();

    unsigned char mChannel;    // +0x1c
    unsigned char mController; // +0x1d, from configuration code 0x393
    ActiveFilter mFilter;      // +0x20
    int mPlaying;              // +0x48
    int mValue;                // +0x4c, the controller value, 64 at first
    Sch::TickClock *mClock;    // +0x50
};
