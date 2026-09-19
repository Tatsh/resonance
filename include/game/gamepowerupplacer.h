#pragma once

#include "app/ticktask.h"
#include "game/powerupplacer.h"

class Application;
class LocalPlayer;
class PowerupCollectionI;

/**
 * Powerup placer that walks a bar cursor forward with the song and deploys at the bar it rests on.
 *
 * `17GamePowerupPlacer` in the RTTI descriptor at `0x008f0130`, over two bases, PowerupPlacer at
 * offset 0 and TickTask at offset 20. It is the one class in this family with two bases, and it
 * therefore has two tables. The primary table at `0x007e4c50` has nine entries, the same length as
 * PowerupPlacer's, so the class adds no virtual on that side. The secondary table at `0x007e4c20`
 * has five entries with a slot 0 delta of -20 and serves the TickTask subobject.
 *
 * The object is 0x44 bytes. PowerupPlacer and its MsgSource base occupy `+0x00` through `+0x13`
 * with the primary vptr at `+0x10`. The TickTask subobject occupies `+0x14` through `+0x33` with
 * its reference count at `+0x14` and its own vptr at `+0x18`. The four members below follow it.
 *
 * Nothing in the image constructs one. The constructor at `0x001ccb70` has no reference of any
 * kind, from code or from data, so the class is unreachable in the shipped game. Its two
 * PowerupPlacer siblings, JamPowerupPlacer and SimplifiedGamePowerupPlacer, are both constructed
 * and share the deployment shape below.
 *
 * Three bodies here are not written yet, and each one is blocked on a routine outside this family
 * that has no recovered name. The constructor and slot 6 both need the accessor at `0x00118e78`,
 * which turns an Application into the Sch::TickClock the task is posted against. Slot 8 needs the
 * accessor at `0x00118da0` and slot 9 of the object it returns, which reports the last bar a
 * placement is allowed in. Slots 4 and 5 both need a TickTask member that `app/ticktask.h` does
 * not declare, the scheduling call at `0x0013a860` and the withdrawal at `0x0013ae10`.
 *
 * The two slots this class overrides on the PowerupPlacer side retain the base spelling,
 * OnUnknownSlot4() and OnUnknownSlot5(), even though the bodies recover the verbs. The base
 * declares those names and JamPowerupPlacer already matches them, and an override that differs by
 * one letter becomes a new virtual instead.
 */
class GamePowerupPlacer : public PowerupPlacer, public TickTask {
public:
    /**
     * Post a task against the song clock and start with the cursor off the map.
     *
     * The body is not written yet. It constructs the PowerupPlacer base, constructs the TickTask
     * subobject against the clock the accessor at `0x00118e78` returns for pApplication with a
     * period of 480 ticks and a third argument of zero, installs both tables, stores the three
     * arguments, and sets the cursor to -1. It also calls IsFiniteMBT(480) and discards the
     * result, which is the shape of an assertion compiled without its report.
     *
     * @param pOwner The player whose placer this is.
     * @param pApplication The application the song clock is reached through.
     * @param pCollection The store the deployment draws from.
     * @ghidraAddress 0x001ccb70
     */
    GamePowerupPlacer(LocalPlayer *pOwner,
                      Application *pApplication,
                      PowerupCollectionI *pCollection);

    /**
     * @ghidraAddress 0x001cd9b0
     */
    virtual ~GamePowerupPlacer();

    /**
     * Move the cursor by the negation of the argument and announce where it rests.
     *
     * The body is not written yet. It negates the argument, reads the song tick through the clock
     * the accessor at `0x00118e78` returns, divides by 1920 for the current bar, and reads
     * Player::Slot4() for the message. A cursor of -1 accepts a step of 1 only, and then only when
     * the store reports a selection through PowerupCollectionI::HasSelection(), and the cursor
     * jumps to the current bar. A cursor already on the map moves by the step, clamped so that it
     * never falls behind the current bar and never runs more than one bar past it. Every accepted
     * move sends a DisplayPointerMsg.
     *
     * @param nStep The step, which the body negates.
     * @ghidraAddress 0x001cccb0
     */
    virtual void OnUnknownSlot6(int nStep);

    /**
     * Announce where the cursor rests, for a listener that has just registered.
     *
     * A cursor of -1 sends nothing.
     *
     * @ghidraAddress 0x001cce90
     */
    virtual void OnUnknownSlot7();

    /**
     * Deploy the selected powerup at the bar the cursor rests on, then take the cursor off the map.
     *
     * The body is not written yet. A cursor of -1 returns. The cursor must also lie at or before
     * the bar slot 9 of the object the accessor at `0x00118da0` returns reports. The deployment
     * dispatches PowerupCollectionI::Deploy() with Player::Slot4() and the cursor bar, then sends
     * a DisplayPointerMsg with a bar of -1 and sets the cursor to -1.
     *
     * @ghidraAddress 0x001ccf30
     */
    virtual void OnUnknownSlot8();

    /**
     * Take the cursor off the map, on the PowerupPlacer side.
     *
     * The body is not written yet. It schedules the TickTask subobject at kMBTInfinity
     * through the call at `0x0013a860`.
     *
     * @ghidraAddress 0x001cde18
     */
    virtual void OnUnknownSlot4();

    /**
     * Withdraw the task from the clock, on the PowerupPlacer side.
     *
     * The body is not written yet. It calls the withdrawal at `0x0013ae10` on the TickTask
     * subobject, which cancels the queued command and resets the handle to -2.
     *
     * @ghidraAddress 0x001cde40
     */
    virtual void OnUnknownSlot5();

    /**
     * Advance the cursor to the current bar and announce it.
     *
     * Slot 4 of the TickTask table. The elapsed count arrives already saturated against Mid::MBT's
     * bounds, and this body adds 480 and saturates again before dividing by 1920, so the bar it
     * computes is half a beat ahead of the run now due. A cursor of -1 or a cursor already at or
     * past the computed bar produces no message. The return value is 1 on every path, so the task
     * runs again every 480 ticks for as long as it is scheduled.
     *
     * @param nElapsedTicks Ticks between the task's epoch and the run now due.
     * @return 1 on every path.
     * @ghidraAddress 0x001cd028
     */
    virtual int Tick(int nElapsedTicks);

private:
    LocalPlayer *mOwner;             // +0x34
    Application *mApplication;       // +0x38
    PowerupCollectionI *mCollection; // +0x3c
    // The bar the placement cursor rests on, or -1 for a cursor off the map.
    int mCursorBar; // +0x40
};
