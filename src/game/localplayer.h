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
     * @param nInputSlot The controller slot GetInputSlot() reports.
     * @param colorName The player's colour name.
     * @param pAppearance The appearance the player is drawn with.
     * @param pClock The clock the per-bar command is posted on.
     * @param nTrack The track GetTrack() reports.
     * @ghidraAddress NTSC-U/C: 0x0011e000
     * @ghidraAddress PAL: 0x0011e5a0
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
     * @ghidraAddress NTSC-U/C: 0x0011e348
     * @ghidraAddress PAL: 0x0011e8f8
     */
    virtual ~LocalPlayer();

    /**
     * @ghidraAddress NTSC-U/C: 0x00121ea0
     * @ghidraAddress PAL: 0x001224a8
     */
    virtual int GetInputSlot();

    /**
     * @ghidraAddress NTSC-U/C: 0x00121e90
     * @ghidraAddress PAL: 0x00122498
     */
    virtual int GetTrack();

    /**
     * @ghidraAddress NTSC-U/C: 0x00121e98
     * @ghidraAddress PAL: 0x001224a0
     */
    virtual int GetPlace();

    /**
     * @ghidraAddress NTSC-U/C: 0x00122890
     * @ghidraAddress PAL: 0x00122ea8
     */
    virtual int UnusedQuery();

    /**
     * @ghidraAddress NTSC-U/C: 0x00121ea8
     * @ghidraAddress PAL: 0x001224b0
     */
    virtual void UnusedHook();

    /**
     * @ghidraAddress NTSC-U/C: 0x00122898
     * @ghidraAddress PAL: 0x00122eb0
     */
    virtual void SetFreestyleSpan(int nStartBar, int nEndBar);

    /**
     * @ghidraAddress NTSC-U/C: 0x001228a8
     * @ghidraAddress PAL: 0x00122ec0
     */
    virtual int IsFreestyleBar(int nBar);

    /**
     * @ghidraAddress NTSC-U/C: 0x00121eb0
     * @ghidraAddress PAL: 0x001224b8
     */
    virtual int IsLooping();

    /**
     * Announce the player's whole state and start the per-bar command.
     *
     * Runs Player::AnnounceState(), then sends a TrackSelectMsg for the player's track at the
     * current song position, has the collection announce its state, and sends a PointAmountMsg, a
     * ToggleGhostMsg, and a LoopToggleMsg. It then posts a LocalPlayerCmd at the end of the first
     * bar under mCommand.
     *
     * @ghidraAddress NTSC-U/C: 0x0011e4e0
     * @ghidraAddress PAL: 0x0011eaa0
     */
    virtual void AnnounceState();

    /**
     * Run PowerupPlacer::Deactivate() on the placer.
     *
     * That slot's body is two instructions, and the call arrives at an empty routine. Both the
     * `PowerupPlacer` and `JamPowerupPlacer` tables record the same address for it rather than a
     * re-emitted copy each. The shared address proves the empty body is inherited rather than a
     * local override. This dispatch is therefore real and does nothing.
     *
     * @ghidraAddress NTSC-U/C: 0x001228c8
     * @ghidraAddress PAL: 0x00122ee0
     */
    virtual void DeactivatePlacer();

    /**
     * @ghidraAddress NTSC-U/C: 0x00121ec0
     * @ghidraAddress PAL: 0x001224c8
     */
    virtual int CountCaughtGem();

    /**
     * @ghidraAddress NTSC-U/C: 0x00121ed0
     * @ghidraAddress PAL: 0x001224d8
     */
    virtual int CountMissedGem();

    /**
     * Report the multiplier a capture starting at a bar earns.
     *
     * A bar after the end of the last caught run earns only the bonus plus one, and a bar inside
     * it adds the streak multiplier.
     *
     * @ghidraAddress NTSC-U/C: 0x00122ca0
     * @ghidraAddress PAL: 0x001232b8
     */
    virtual int GetMultiplier(int nBar);

    /**
     * Report the best streak of consecutive captures.
     *
     * @ghidraAddress NTSC-U/C: 0x00121ee0
     * @ghidraAddress PAL: 0x001224e8
     */
    virtual int GetBestStreak();

    /**
     * Report the proportion of captured phrases among those captured and muffed.
     *
     * Returns zero when nothing was captured, so the division never runs on an empty total.
     *
     * @return A fraction between 0 and 1.
     * @ghidraAddress NTSC-U/C: 0x00122cc8
     * @ghidraAddress PAL: 0x001232e0
     */
    virtual float GetCaptureRatio();

    /**
     * @ghidraAddress NTSC-U/C: 0x00121ee8
     * @ghidraAddress PAL: 0x001224f0
     */
    virtual int GetGameMode();

    /**
     * @ghidraAddress NTSC-U/C: 0x00122c00
     * @ghidraAddress PAL: 0x00123218
     */
    virtual int MarkBarScored(int nBar);

    /**
     * Start or stop looping.
     *
     * Only in jam. Stores bLooping, invalidates the seeker of the player's track at the position's
     * bar, and sends a LoopToggleMsg. The title is inferred.
     *
     * @param bLooping Non-zero to loop.
     * @param position The song position of the change.
     * @ghidraAddress NTSC-U/C: 0x0011e810
     * @ghidraAddress PAL: 0x0011edd0
     */
    virtual void SetLooping(int bLooping, const Mid::MBT &position);

    /**
     * Show or hide the player's ghost.
     *
     * Slot 22, declared by this class rather than inherited. Returns at once unless the play mode
     * is jam. Otherwise it stores its argument in mGhost and sends a `ToggleGhostMsg` for this
     * player with its argument as ToggleGhostMsg::mOn. The set-ghost-mode script command is the
     * recovered caller.
     *
     * @param bGhost Non-zero to show the ghost.
     * @ghidraAddress NTSC-U/C: 0x0011e908
     * @ghidraAddress PAL: 0x0011eec8
     */
    virtual void SetGhost(int bGhost);

    /**
     * Receive one message.
     *
     * Acts on the controller, capture, and powerup messages that address this player, and passes
     * every message it does not recognise to Player::HandleMessage().
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x0011ed98
     * @ghidraAddress PAL: 0x0011f358
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * @ghidraAddress NTSC-U/C: 0x001228f8
     * @ghidraAddress PAL: 0x00122f10
     */
    virtual void AddSink(MsgSink *pSink);

    /**
     * @ghidraAddress NTSC-U/C: 0x00122968
     * @ghidraAddress PAL: 0x00122f80
     */
    virtual void RemoveSink(MsgSink *pSink);

    /**
     * Run the update of the bar that starts at a tick, then post the next bar's.
     *
     * Ends a multiplier bonus whose last bar has passed, resets the multiplier two bars after the
     * last caught bar, and announces the multiplier when either changes. LocalPlayerCmd::Execute()
     * is the caller. The title is inferred.
     *
     * @param nTick The song position.
     * @ghidraAddress NTSC-U/C: 0x0011ec00
     * @ghidraAddress PAL: 0x0011f1c0
     */
    void OnBarTick(int nTick);

    /**
     * Toggle looping.
     *
     * Only in jam. Invalidates the seeker of the player's track at the position's bar and sends a
     * LoopToggleMsg. The title is inferred.
     *
     * @param position The song position of the change.
     * @ghidraAddress NTSC-U/C: 0x0011e700
     * @ghidraAddress PAL: 0x0011ecc0
     */
    void ToggleLoop(const Mid::MBT &position);

private:
    // NTSC-U/C: 0x0011e980, PAL: 0x0011ef40
    // Records a selection of this player's track and place, and tells the other game
    // systems unless the player stayed on the same track and dropped back.
    void OnTrackSelect(TrackSelectMsg *pMsg);

    // NTSC-U/C: 0x0011eaa8, PAL: 0x0011f068
    // Toggles the ghost display of this player and announces it.
    void OnToggleGhost(ToggleGhostMsg *pMsg);

    // NTSC-U/C: 0x0011eb20, PAL: 0x0011f0e0
    // Starts a multiplier bonus of 2 for eight bars from the message's bar.
    void OnMultiplier(MultiplierMsg *pMsg);

    // The six handlers below are inline, and HandleMessage() expands each. The addresses are
    // their uncalled out-of-line copies.

    // NTSC-U/C: 0x00122a20, PAL: 0x00123038
    // Moves the collection's selection when the message addresses this player.
    void OnAxisYPow(AxisYPowMsg *pMsg);

    // NTSC-U/C: 0x00122ae0, PAL: 0x001230f8
    // Toggles looping at the message's position when it addresses this player.
    void OnLoopTool(LoopToolMsg *pMsg);

    // NTSC-U/C: 0x00122b38, PAL: 0x00123150
    // Updates the streak, multiplier, and capture counts, then awards the capture.
    void OnPhraseCaptured(PhraseCapturedMsg *pMsg);

    // NTSC-U/C: 0x00122c20, PAL: 0x00123238
    // Counts a tried muff once per bar.
    void OnPhraseMuffed(PhraseMuffedMsg *pMsg);

    // NTSC-U/C: 0x001229d8, PAL: 0x00122ff0
    // Forwards a button press to the placer.
    void OnButtonPow(ButtonPowMsg *pMsg);

    // NTSC-U/C: 0x00122a68, PAL: 0x00123080
    // Adds the caught powerup to the collection, plays its sounds, and passes the message on.
    void OnCaughtPowerbar(CaughtPowerbarMsg *pMsg);

    Sch::TickClock *mClock; // +0x48
    CmdID mCommand;         // +0x4c
    int mInputSlot;         // +0x50
    int mTrack;             // +0x58
    int mPlace;             // +0x5c
    int mLooping;           // +0x60
    int mGhost;             // +0x64 the ghost display
    int mPlayMode;          // +0x68
    int mGameMode;          // +0x6c
    int mFreestyleStartBar; // +0x70
    int mFreestyleEndBar;   // +0x74
    int mLastScoredBar;     // +0x78
    int mRunEndBar;         // +0x7c the end of the last caught run
    int mLastCaughtBar;     // +0x80
    int mStreak;            // +0x84
    int mBestStreak;        // +0x88
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
    int mMissedGems;        // +0xac
    int mCaughtGems;        // +0xb0
};
