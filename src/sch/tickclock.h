#pragma once

#include "app/timeclock.h"
#include "mid/mbt.h"
#include "sch/tick.h"

class CmdID;
namespace Sch {
class Command;
class TempoMap;
} // namespace Sch

namespace Sch {

/**
 * Pausable view of scheduler time, together with the tempo that maps it to a song position.
 *
 * The class is not polymorphic and does not emit RTTI. Its name comes from the one recorded
 * signature that mentions it. `Catcher`'s constructor takes a `Sch::TickClock *`. That signature
 * identifies this class rather than its base, because Catcher's handler at `0x001ac760` calls
 * SongTick(), and SongTick() reads mTempoMap.
 *
 * An instance is 0x1c bytes. The base subobject occupies `+0x00` through `+0x17` and this class
 * adds one pointer at `+0x18`, which the constructor at `0x004a79f8` is the last write of and
 * which the destructor at `0x004a7aa8` releases.
 *
 * The base is the 0x18-byte class that `Globals::InitServices()` instantiates directly at
 * `0x00117118`, with its own constructor at `0x004a7780` and its own destructor at `0x004a7798`
 * that releases no tempo map. Two measurements prove the base and derived classes are distinct. The
 * two destructors differ in behaviour, and a class has exactly one destructor. Start(), Resume(),
 * Pause(), and Now() exist as single bodies that both the Globals object and GrooveWorld's object
 * are passed to, and none of the four touches `+0x18`.
 *
 * Every post below wraps the command in a Sch::TimedCommand, hands the wrapper to one of the
 * scheduler's two queueing paths, and then gives back its reference. The queue is then the only
 * owner. The member titles are inferred from those bodies. The script command `clock` at
 * `0x00150d88` supplies the words `tempo`, `tick`, and `song_bar`, the source of the name
 * SongTick(). The bare name `Tick` would hide the type `Sch::Tick` inside this class.
 */
class TickClock : public TimeClock {
public:
    /**
     * Start a clock against a scheduler, with a tempo map of its own or a shared one.
     *
     * A null tempo map produces a private one at the default tempo of 500000 microseconds per
     * quarter note. A tempo map the caller supplies is shared, and its reference count is
     * incremented directly at `0x004a7a28`.
     *
     * @param pWatchdog The scheduler this clock posts to.
     * @param pTempoMap The tempo map to share, or null for a private one.
     * @ghidraAddress NTSC-U/C: 0x004a79f8
     * @ghidraAddress PAL: 0x004e5b08
     */
    TickClock(Scheduler *pWatchdog, TempoMap *pTempoMap);

    /**
     * Give back the reference to the tempo map.
     *
     * @ghidraAddress NTSC-U/C: 0x004a7aa8
     * @ghidraAddress PAL: 0x004e5bb8
     */
    ~TickClock();

    /**
     * Share a different tempo map, giving back the reference to the current one.
     *
     * The new map's reference count is incremented before the old map is released, and a null
     * map is stored without a reference. GrooveWorld::FinishLoad() is the recovered caller, with
     * the tempo map a converted level reports.
     *
     * @param pTempoMap The tempo map to share, or null.
     * @ghidraAddress NTSC-U/C: 0x004a7be0
     * @ghidraAddress PAL: 0x004e5cf0
     */
    void SetTempoMap(TempoMap *pTempoMap);

    /**
     * Report the song position the clock has arrived at.
     *
     * The body divides the current time, biased by TempoMap::mCeilingBias, by
     * TempoMap::mNanosecondsPerTick, which rounds up to the next whole MIDI tick. The result is
     * then wrapped in a Mid::MBT, whose constructor runs the discarded IsFiniteMBT() check at
     * `0x00100ab8`.
     *
     * @return The song position, in MIDI ticks at 480 per quarter note.
     * @ghidraAddress NTSC-U/C: 0x004a7af8
     * @ghidraAddress PAL: 0x004e5c08
     */
    int SongTick();

    /**
     * Move the clock to a song position.
     *
     * The body converts the position to scheduler time through mTempoMap and stores it as the
     * paused reading, but only when it differs from the current reading. The store happens even
     * while the clock runs, when Now() does not report the paused reading.
     *
     * The position is a Mid::MBT passed by value in one register. Both callers, GrooveWorld's
     * PrepareLevel() at `0x0018ddd8` and StopLevel() at `0x0018ed6c`, construct it through
     * Mid::MBT(int) immediately before the call.
     *
     * @param tick The song position.
     * @ghidraAddress NTSC-U/C: 0x004a7b60
     * @ghidraAddress PAL: 0x004e5c70
     */
    void SetSongTick(Mid::MBT tick);

    /**
     * Queue a command at an absolute scheduler time, discarding the handle.
     *
     * @param pCommand The command to run.
     * @param tick The scheduler time to run it at, in this clock's frame.
     * @ghidraAddress NTSC-U/C: 0x004a5fd0
     * @ghidraAddress PAL: 0x004e4070
     */
    void PostAt(Command *pCommand, Tick tick);

    /**
     * Queue a command at a song position, under a handle the caller retains.
     *
     * @param pCommand The command to run.
     * @param nTick The song position, in MIDI ticks at 480 per quarter note. Callers widen a
     *              32-bit position into the 64-bit register, and the body multiplies the full
     *              width through its 64-bit helper at `0x00600270`.
     * @param id The handle to queue under.
     * @param nUnused All 17 call sites pass 0 as the fifth argument, and the body never reads
     *                it. The type and the meaning are not recoverable.
     * @ghidraAddress NTSC-U/C: 0x004a6248
     * @ghidraAddress PAL: 0x004e42e8
     */
    void PostAtSongTick(Command *pCommand, long long nTick, CmdID &id, int nUnused = 0);

    /**
     * Queue a command at a song position, discarding the handle.
     *
     * @param pCommand The command to run.
     * @param nTick The song position, in MIDI ticks at 480 per quarter note.
     * @ghidraAddress NTSC-U/C: 0x004a6330
     * @ghidraAddress PAL: 0x004e43d0
     */
    void PostAtSongTick(Command *pCommand, long long nTick);

    /**
     * Map between song position and scheduler time.
     *
     * Public because the `clock tempo` script command reads it directly at `0x001511ec`,
     * the undeclared Globals accessor at `0x00118ce8` reads it inline, and the image has no
     * out-of-line accessor.
     *
     * +0x18
     */
    TempoMap *mTempoMap;
};

} // namespace Sch
