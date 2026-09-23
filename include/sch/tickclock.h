#pragma once

#include "app/watchdogtimer.h"
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
 * The class is not polymorphic and emits no RTTI, so its title comes from the one mangled signature
 * that mentions it. The RTTI records `Catcher`'s constructor as
 * `__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick`, which makes
 * `Sch::TickClock` the original title of a type passed by pointer. That signature identifies this
 * class rather than its base, because Catcher's handler at `0x001ac760` calls SongTick(), and
 * SongTick() reads mTempoMap.
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
 * **The base's title, and the title of the class the base refers to, are both disputed.** This
 * reconstruction derives from `WatchdogTimer` so that the tree continues to compile, and the
 * evidence against both titles is recorded in the Watchdog and WatchdogTimer class
 * documentation. In short, the 0x50-byte object those headers call `Watchdog` is the command
 * scheduler: its `+0x00` is a red-black tree of Sch::TimedCommand pointers ordered by due tick,
 * its `Service()` at `0x004aa848` pops every due wrapper and calls Sch::TimedCommand::Run() on it,
 * and its `+0x0c` selects between recording and playback of the queued stream.
 *
 * Every post below wraps the command in a Sch::TimedCommand, hands the wrapper to one of the
 * scheduler's two queueing paths, and then gives back its own reference, which leaves the queue as
 * the only owner. The member titles are inferred from those bodies. The script command `clock` at
 * `0x00150d88` supplies the words `tempo`, `tick`, and `song_bar`, which is what titles SongTick();
 * the bare title `Tick` would hide the type `Sch::Tick` inside this class, so the song part is
 * spelled out.
 */
class TickClock : public WatchdogTimer {
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
     * @ghidraAddress 0x004a79f8
     */
    TickClock(Watchdog *pWatchdog, TempoMap *pTempoMap);

    /**
     * Give back the reference to the tempo map.
     *
     * @ghidraAddress 0x004a7aa8
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
     * @ghidraAddress 0x004a7be0
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
     * @ghidraAddress 0x004a7af8
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
     * @ghidraAddress 0x004a7b60
     */
    void SetSongTick(Mid::MBT tick);

    /**
     * Queue a command at an absolute scheduler time, discarding the handle.
     *
     * @param pCommand The command to run.
     * @param tick The scheduler time to run it at, in this clock's frame.
     * @ghidraAddress 0x004a5fd0
     */
    void PostAt(Command *pCommand, Tick tick);

    /**
     * Queue a command at a song position, under a handle the caller retains.
     *
     * @param pCommand The command to run.
     * @param nTick The song position, in MIDI ticks at 480 per quarter note.
     * @param id The handle to queue under.
     * @param nUnused Every one of the 17 callers passes 0 in the fifth argument register, and the
     *                body never reads it. The type and the meaning are not recoverable.
     * @ghidraAddress 0x004a6248
     */
    void PostAtSongTick(Command *pCommand, long long nTick, CmdID &id, int nUnused = 0);

    /**
     * Queue a command at a song position, discarding the handle.
     *
     * @param pCommand The command to run.
     * @param nTick The song position, in MIDI ticks at 480 per quarter note.
     * @ghidraAddress 0x004a6330
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
