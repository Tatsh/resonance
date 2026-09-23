#pragma once

#include "game/genericcatcher.h"
#include "game/player.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "msg/message.h"
#include "sch/cmdid.h"
#include "sch/tick.h"
#include "sch/tickclock.h"

/**
 * Catcher that scores the gems one track presents.
 *
 * `7Catcher` in the RTTI descriptor at `0x00902010`, with GenericCatcher as its only base at
 * offset 0. Its primary table is at `0x007e0c38` with eleven entries and its MsgSource subobject
 * table at `0x007e0c10` with four. Two classes derive from it, MultiCatcher and SingleCatcher, and
 * both retain eleven entries, so neither introduces a virtual.
 *
 * The class supplies MsgSink::HandleMessage() and GenericCatcher's slots 4, 5, and 6, and it
 * introduces four virtuals of its own at slots 7 through 10. Slots 9 and 10 address the shared
 * pure-virtual stub at `0x005381a8`, so this class is abstract and the two subclasses exist to
 * supply them. The object is 0x80 bytes, which MultiCatcher's tagged allocation measures;
 * SingleCatcher's is 0x84, so that class adds one word.
 *
 * The constructor's parameter list is attested rather than inferred. The anonymous-namespace marker
 * for the file-local command class `GemCmd` records the enclosing constructor's mangled signature
 * as `__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick`, which is
 * `Catcher(PhraseMgr *, Quantizer *, const TrackData *, Sch::TickClock *, int, Sch::Tick)`. A
 * second marker records `PostGemCmd` in the same translation unit. Both are the commands slot 4
 * schedules.
 *
 * The constructor stores only the low word of its Sch::Tick parameter, with `sw` rather than `sd`.
 * That is recorded here as measured rather than explained, and it belongs with the note in
 * `sch/tick.h` that two measurements of that type's member count disagree.
 *
 * Six bodies are not written here. HandleMessage(), Slot7(), Slot8(), PostCatchMsg(), and
 * PostCaughtBarMsg() each read or build a message as a local object, and the message classes
 * involved declare their payload words private and declare no constructor, so none of the five can
 * be expressed. MultiCatcher::Slot9() depends on seven routines in the 0x001d7xxx range that are
 * not identified. Each is described where it is declared, with what the disassembly establishes.
 *
 * The table diff corrects an earlier attribution. `0x001adb78`, `0x001b15a8`, `0x001b1610`,
 * `0x001b19a0`, `0x001abe50`, and `0x001abfd8` were titled for MultiCatcher and sit in this
 * class's own table at slots 3, 4, 5, 6, 7, and 8. SingleCatcher's table holds the same six
 * addresses at the same indices, which confirms it independently of MultiCatcher's.
 */
class Catcher : public GenericCatcher {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pTrackData The track description.
     * @param pClock The clock the scheduled commands run on.
     * @param nFlag MultiCatcher passes 1. No other caller was inspected.
     * @param tick The scheduler time the constructor retains.
     * @ghidraAddress 0x001aba30
     */
    Catcher(PhraseMgr *pPhraseMgr,
            Quantizer *pQuantizer,
            const TrackData *pTrackData,
            Sch::TickClock *pClock,
            int nFlag,
            Sch::Tick tick);

    /**
     * @ghidraAddress 0x001abc10
     */
    virtual ~Catcher();

    /**
     * Act on a message.
     *
     * Slot 3. The routine dispatches on the message's registered identity over five identities: a
     * CatchMsg and two further messages arrive at their own handlers, one message is acted on only
     * when its player matches this catcher's, and every other message is discarded. The body is
     * not written, for the reason recorded in the class documentation.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001adb78
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Schedule the catcher's two commands on the clock.
     *
     * Slot 4. The post-gem command is always scheduled and the gem command only in game mode 3.
     *
     * @ghidraAddress 0x001b15a8
     */
    virtual void Slot4();

    /**
     * Withdraw the catcher's two commands from the clock.
     *
     * Slot 5. The two withdrawals mirror slot 4, the second being conditional on game mode 3 in
     * the same way.
     *
     * @ghidraAddress 0x001b1610
     */
    virtual void Slot5();

    /**
     * Report whether the catcher has nothing outstanding.
     *
     * Slot 6.
     *
     * @return Non-zero when the counter at `+0x60` is zero.
     * @ghidraAddress 0x001b19a0
     */
    virtual int Slot6();

    /**
     * Record a missed gem.
     *
     * Slot 7. The routine plays the miss sound for the player's own slot, `SND_MISS_PLAYER1`
     * through `SND_MISS_PLAYER4` by Player::Slot2(), and plays nothing for any other slot. It then
     * sends a CatchMsg whose payload is the tick, the word at `+0x4c`, the second argument, zero,
     * this catcher's player, zero, and zero. When the tick's bar matches the bar at `+0x5c`, or
     * the counter at `+0x60` is positive, it records the tick, increments the counter at `+0x54`,
     * clears the counter at `+0x60`, and reports the muffed phrase. The body is not written, for
     * the reason recorded in the class documentation.
     *
     * @param nTick The scheduler time of the miss.
     * @param nValue A word the CatchMsg payload includes.
     * @ghidraAddress 0x001abe50
     */
    virtual void Slot7(int nTick, int nValue);

    /**
     * Advance the catcher to a tick.
     *
     * Slot 8. The routine records the tick, increments the counter at `+0x50`, resolves the tick's
     * bar, sends a GemMsg and then two further messages, and finally posts the caught-bar message
     * when the bar changed. The body is not written, for the reason recorded in the class
     * documentation.
     *
     * @param nTick The scheduler time to advance to.
     * @param nValue A word the payloads include.
     * @ghidraAddress 0x001abfd8
     */
    virtual void Slot8(int nTick, int nValue);

    /**
     * Slot 9, pure. MultiCatcher scores a range of bars and SingleCatcher scores one.
     *
     * @param nFirst The first word.
     * @param nSecond The second word.
     * @param nThird The third word. Both subclasses branch on whether it is zero.
     */
    virtual void Slot9(int nFirst, int nSecond, int nThird) = 0;

    /**
     * Slot 10, pure. MultiCatcher's override is empty and SingleCatcher's sends a
     * CaughtPowerbarMsg to its player when the phrase manager reports a value other than -1.
     */
    virtual void Slot10() = 0;

protected:
    // Report a muffed phrase.
    // The body is not written, for the reason recorded in the class documentation.
    // 0x001ad3b0
    void PostPhraseMuffedMsg(int nBar, int nTick);

    // Report a caught gem.
    // The routine ignores the call unless the two counters at `+0x54` and `+0x58` are both zero,
    // and it is reached from HandleMessage() on a CatchMsg whose player matches this catcher's.
    // The body is not written, for the reason recorded in the class documentation.
    // 0x001ac370
    void PostCatchMsg(Message *pMsg);

    // Report a caught bar.
    // The body is not written, for the reason recorded in the class documentation.
    // 0x001ac7e8
    void PostCaughtBarMsg(int nBar, int nValue);

    // Resolve a scheduler time, substituting the clock's current reading for -1. The title is
    // inferred from the two callers below, both of which pass their own tick argument through
    // unchanged and use the answer as the time to post at.
    // 0x001aca48
    Sch::Tick ResolveTick(Sch::Tick tick);

    // Build a PostGemCmd and queue it under the handle at `+0x2c`.
    // 0x001acba0
    void SchedulePostGemCommand(Sch::Tick tick);

    // Build a GemCmd and queue it at a song position under the handle at `+0x30`.
    // 0x001acca0
    void ScheduleGemCommand(Sch::Tick tick);

    Quantizer *mQuantizer;       // +0x18
    PhraseMgr *mPhraseMgr;       // +0x1c
    const TrackData *mTrackData; // +0x20
    Player *mPlayer;             // +0x24, the file-scope NullPlayer until one is assigned
    Sch::TickClock *mClock;      // +0x28
    CmdID mPostGemCommand;       // +0x2c
    CmdID mGemCommand;           // +0x30
    int mTicksPerBar;            // +0x34, copied from the phrase manager and used as a divisor
    int mUnknown38;              // +0x38, the constructor's int parameter
    int mUnknown3c;              // +0x3c, the low word of the constructor's Sch::Tick parameter
    int mUnknown40;              // +0x40, always 1
    int mUnknown44;              // +0x44, starts -1
    int mUnknown48;              // +0x48, starts -1, slot 7 stores the missed tick
    int mUnknown4c;              // +0x4c, copied from TrackData::mUnknown04
    int mUnknown50;              // +0x50, slot 8 increments it
    int mUnknown54;              // +0x54, slot 7 increments it
    int mUnknown58;              // +0x58
    int mUnknown5c;              // +0x5c, the bar slot 7 compares against
    int mUnknown60;              // +0x60, slot 6 reports whether it is zero
    int mUnknown64;              // +0x64, starts -1
    int mUnknown68;              // +0x68, the constructor does not write it
    int mUnknown6c;              // +0x6c, the constructor does not write it
    int mUnknown70;              // +0x70
    Player *mUnknown74;          // +0x74, the file-scope NullPlayer until one is assigned
    float mUnknown78;            // +0x78, HandleMessage stores a float here
    int mUnknown7c;              // +0x7c
};
