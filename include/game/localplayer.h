#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/player.h"
#include "game/powerupcollectioni.h"
#include "game/powerupplacer.h"
#include "mid/mbt.h"
#include "sch/cmdid.h"

class AxisYPowMsg;
class ButtonPowMsg;
class CaughtPowerbarMsg;
class HxStr;
class LoopToolMsg;
class MultiplierMsg;
class PhraseCapturedMsg;
class PhraseMuffedMsg;
class ToggleGhostMsg;
class TrackSelectMsg;

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Player driven by a controller on this machine.
 *
 * `LocalPlayer` in the RTTI descriptor at `0x008eeff8`, with `Player` as its only base. Its three
 * vtables are at `0x007d0160`, `0x007d0138`, and `0x007d0110`, each walked to its terminator.
 *
 * The primary table has **23 entries against the base's 21**, so this class adds two virtuals past
 * the end of the base table rather than overriding into it. It replaces slots 2 and 4 through 12
 * and 14 through 20, and inherits only slots 3 and 13. In the `MsgSource` table it replaces both
 * `AddSink` and `RemoveSink`, which the base leaves inherited, and in the `MsgSink` table it
 * replaces `HandleMessage`.
 *
 * The constructor gives the player a powerup collection and a placer that fits the play mode, and
 * AddSink() and RemoveSink() fan registration out to both. The player also keeps the capture
 * statistics the solo statistics read back: the streak of consecutive captures and its best, the
 * multiplier, and the counts of captured and muffed phrases. A LocalPlayerCmd posted every bar on
 * mClock ends a multiplier bonus and resets the multiplier when the player stops catching.
 *
 * A slot whose verb is unrecovered keeps its table index as its title, because the index is part
 * of the layout.
 */
class LocalPlayer : public Player {
public:
    /**
     * Build the player and the powerup collection and placer its play mode calls for.
     *
     * In jam the player gets an unlimited PowerupCollection and a JamPowerupPlacer, a freestyle
     * span that never ends, and starts looping. In game modes 1 through 3 it gets a
     * SinglePowerupCollection and a SimplifiedGamePowerupPlacer. In any other mode it gets neither.
     *
     * @param nId The player's identifier.
     * @param nInputSlot The controller slot Slot2() reports.
     * @param colorName The player's colour name.
     * @param pAppearance The appearance the player is drawn with.
     * @param pClock The clock the per-bar command is posted on.
     * @param nTrack The track Slot4() reports.
     * @ghidraAddress 0x0011e000
     */
    LocalPlayer(int nId,
                int nInputSlot,
                const HxStr &colorName,
                const FreqAppearance *pAppearance,
                Sch::TickClock *pClock,
                int nTrack);

    /**
     * Delete the placer and the collection.
     *
     * @ghidraAddress 0x0011e348
     */
    virtual ~LocalPlayer();

    /** @ghidraAddress 0x00121ea0 */
    virtual int Slot2();

    /** @ghidraAddress 0x00121e90 */
    virtual int Slot4();

    /** @ghidraAddress 0x00121e98 */
    virtual int Slot5();

    /** @ghidraAddress 0x00122890 */
    virtual int Slot6();

    /** @ghidraAddress 0x00121ea8 */
    virtual void Slot7();

    /** @ghidraAddress 0x00122898 */
    virtual void Slot8(int first, int second);

    /** @ghidraAddress 0x001228a8 */
    virtual int Slot9(int value);

    /** @ghidraAddress 0x00121eb0 */
    virtual int Slot10();

    /**
     * Announce the player's whole state and start the per-bar command.
     *
     * Runs Player::Slot11(), then sends a TrackSelectMsg for the player's track at the current
     * song position, has the collection announce its state, and sends a PointAmountMsg, a
     * ToggleGhostMsg, and a LoopToggleMsg. It then posts a LocalPlayerCmd at the end of the first
     * bar under mCommand.
     *
     * @ghidraAddress 0x0011e4e0
     */
    virtual void Slot11();

    /**
     * Slot 12. Forwards to slot 5 of the placer.
     *
     * That slot is declared on `PowerupPlacer` and its body is two instructions, so the call
     * reaches an empty routine. Both the `PowerupPlacer` and `JamPowerupPlacer` tables record the
     * same address for it rather than a re-emitted copy each, which is what proves the empty body
     * is inherited rather than a local override, so this dispatch is real and does nothing.
     *
     * @ghidraAddress 0x001228c8
     */
    virtual void Slot12();

    /** @ghidraAddress 0x00121ec0 */
    virtual int Slot14();

    /** @ghidraAddress 0x00121ed0 */
    virtual int Slot15();

    /** @ghidraAddress 0x00122ca0 */
    virtual int Slot16(int value);

    /**
     * Report the best streak of consecutive captures.
     *
     * @ghidraAddress 0x00121ee0
     */
    virtual int Slot17();

    /**
     * Report the proportion of captured phrases among those captured and muffed.
     *
     * Returns zero when nothing was captured, so the division never runs on an empty total.
     *
     * @return A fraction between 0 and 1.
     * @ghidraAddress 0x00122cc8
     */
    virtual float Slot18();

    /** @ghidraAddress 0x00121ee8 */
    virtual int Slot19();

    /** @ghidraAddress 0x00122c00 */
    virtual int Slot20(int value);

    /**
     * Start or stop looping.
     *
     * Only in jam. Stores bLooping, invalidates the seeker of the player's track at the position's
     * bar, and sends a LoopToggleMsg. The title is inferred.
     *
     * @param bLooping Non-zero to loop.
     * @param position The song position of the change.
     * @ghidraAddress 0x0011e810
     */
    virtual void SetLooping(int bLooping, const Mid::MBT &position);

    /**
     * Slot 22. Declared by this class rather than inherited.
     *
     * Returns at once unless the play mode is jam. Otherwise it stores its argument in mGhost and
     * sends a `ToggleGhostMsg` naming this player with its argument as ToggleGhostMsg::mOn.
     *
     * @ghidraAddress 0x0011e908
     */
    virtual void Slot22(int value);

    /**
     * Receive one message.
     *
     * Acts on the controller, capture, and powerup messages that address this player, and passes
     * every message it does not recognise to Player::HandleMessage().
     *
     * @param pMsg The message.
     * @ghidraAddress 0x0011ed98
     */
    virtual void HandleMessage(Message *pMsg);

    /** @ghidraAddress 0x001228f8 */
    virtual void AddSink(MsgSink *pSink);

    /** @ghidraAddress 0x00122968 */
    virtual void RemoveSink(MsgSink *pSink);

    /**
     * Run the update of the bar that starts at a tick, then post the next bar's.
     *
     * Ends a multiplier bonus whose last bar has passed, resets the multiplier two bars after the
     * last caught bar, and announces the multiplier when either changes. LocalPlayerCmd::Execute()
     * is the caller. The title is inferred.
     *
     * @param nTick The song position.
     * @ghidraAddress 0x0011ec00
     */
    void OnBarTick(int nTick);

    /**
     * Toggle looping.
     *
     * Only in jam. Invalidates the seeker of the player's track at the position's bar and sends a
     * LoopToggleMsg. The title is inferred.
     *
     * @param position The song position of the change.
     * @ghidraAddress 0x0011e700
     */
    void ToggleLoop(const Mid::MBT &position);

private:
    // 0x0011e980
    // Records a selection of this player's track and place, and tells the other game
    // systems unless the player stayed on the same track and dropped back.
    void OnTrackSelect(TrackSelectMsg *pMsg);

    // 0x0011eaa8
    // Toggles the ghost display of this player and announces it.
    void OnToggleGhost(ToggleGhostMsg *pMsg);

    // 0x0011eb20
    // Starts a multiplier bonus of 2 for eight bars from the message's bar.
    void OnMultiplier(MultiplierMsg *pMsg);

    // The six handlers below are inline, and HandleMessage() expands each. The addresses are
    // their uncalled out-of-line copies.

    // 0x00122a20
    // Moves the collection's selection when the message addresses this player.
    void OnAxisYPow(AxisYPowMsg *pMsg);

    // 0x00122ae0
    // Toggles looping at the message's position when it addresses this player.
    void OnLoopTool(LoopToolMsg *pMsg);

    // 0x00122b38
    // Updates the streak, multiplier, and capture counts, then awards the capture.
    void OnPhraseCaptured(PhraseCapturedMsg *pMsg);

    // 0x00122c20
    // Counts a tried muff once per bar.
    void OnPhraseMuffed(PhraseMuffedMsg *pMsg);

    // 0x001229d8
    // Forwards a button press to the placer.
    void OnButtonPow(ButtonPowMsg *pMsg);

    // 0x00122a68
    // Adds the caught powerup to the collection, plays its sounds, and passes the message on.
    void OnCaughtPowerbar(CaughtPowerbarMsg *pMsg);

    Sch::TickClock *mClock; // +0x48
    CmdID mCommand;         // +0x4c
    int mInputSlot;         // +0x50 returned by Slot2
    int mTrack;             // +0x58 returned by Slot4
    int mPlace;             // +0x5c returned by Slot5
    int mLooping;           // +0x60 returned by Slot10
    int mGhost;             // +0x64 the ghost display, set by Slot22
    int mPlayMode;          // +0x68
    int mGameMode;          // +0x6c returned by Slot19
    int mUnknown70;         // +0x70 written by Slot8, compared by Slot9
    int mUnknown74;         // +0x74 written by Slot8
    int mUnknown78;         // +0x78 compared by Slot20
    int mRunEndBar;         // +0x7c the end of the last caught run, compared by Slot16
    int mLastCaughtBar;     // +0x80
    int mStreak;            // +0x84
    int mBestStreak;        // +0x88 returned by Slot17
    int mMultiplier;        // +0x8c
    int mBonus;             // +0x90
    int mBonusEndBar;       // +0x94 not written by the constructor
    int mLastMuffedBar;     // +0x98
    int mCaptures;          // +0x9c
    int mMisses;            // +0xa0

public:
    // Public because the select-powerup script command drives it with no accessor in the image.
    PowerupCollectionI *mCollection; // +0xa4

private:
    PowerupPlacer *mPlacer; // +0xa8
    int mUnknownac;         // +0xac returned plus one by Slot15
    int mUnknownb0;         // +0xb0 returned plus one by Slot14
};
