#pragma once

#include <iostream>

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/idable.h"
#include "os/hxstr.h"

class FreqAppearance;
class PhraseCapturedMsg;
class UpdateScorePacket;

/** What Player::GetInputSlot() reports for a player with no input slot. */
constexpr int kNoInputSlot = -1;

/**
 * One participant in a session, local or remote.
 *
 * `Player` in the RTTI descriptor at `0x009020c0`, over three bases: `IDable<Player>` at offset 0,
 * `MsgSink` at 8, and `MsgSource` at 12. Three classes derive from it, `LocalPlayer`, `NetPlayer`,
 * and `NullPlayer`.
 *
 * Three vtables belong to it, each walked to its all-zero terminator rather than counted from the
 * slot titles, which understate every one of them. The primary at `0x007d2170` has 21 entries, so
 * this class declares 19 virtuals of its own after slot 0 and the destructor. The `MsgSink` table
 * at `0x007d2148` has 4 and overrides only `DispatchPriv`, and the `MsgSource` table at
 * `0x007d2120` has 4 and overrides nothing, leaving `AddSink` and `RemoveSink` as inherited.
 *
 * Almost all of the primary table is inert defaults: slots 2 and 4 return -1, six return 0, two
 * return 1, one returns 0.0f, and five are empty. Only slots 11 and 13 have a body of any size, so
 * this class is an interface with defaults and the three subclasses carry the behaviour.
 *
 * The base subobjects account for `+0x00` through `+0x1f`, which the destructor at `0x00132ae8`
 * confirms by restoring a vptr at `+0x04` for `IDable<Player>`, at `+0x08` for `MsgSink`, and at
 * `+0x1c` for `MsgSource`, whose own `mSinks` vector it tears down at `+0x10`. This class's own
 * members start at `+0x20`, and the only one the destructor releases is the colour name at `+0x24`.
 */
class Player : public IDable<Player>, public MsgSink, public MsgSource {
public:
    /**
     * Register the player under its identifier and start its tallies.
     *
     * The score starts at 0 with a ceiling of 1, the juice at 0 with a ceiling of 1, and the last
     * erase time at 0.
     *
     * @param nId The identifier, also recorded in mPlayerId.
     * @param colorName The player's colour name.
     * @param pAppearance The appearance the player is drawn with.
     * @ghidraAddress NTSC-U/C: 0x0012f5c0
     * @ghidraAddress PAL: 0x0012fd78
     */
    Player(int nId, const HxStr &colorName, const FreqAppearance *pAppearance);

    /**
     * @ghidraAddress NTSC-U/C: 0x00132ae8
     * @ghidraAddress PAL: 0x00133318
     */
    virtual ~Player();

    /**
     * Report the controller slot that drives this player.
     *
     * Slot 2. Returns kNoInputSlot here, and NetPlayer inherits it. LocalPlayer returns the slot
     * its constructor received, and InputMap matches a controller reading against it.
     *
     * @return The input slot, or kNoInputSlot.
     * @ghidraAddress NTSC-U/C: 0x00132c20
     * @ghidraAddress PAL: 0x00133460
     */
    virtual int GetInputSlot();

    /**
     * Test whether this player is the stand-in for an absent one.
     *
     * Returns false here. Only `NullPlayer` returns true, and Print() branches on the answer to
     * write "{player null}", which is what recovers the verb.
     *
     * @return Non-zero when this player is a stand-in.
     * @ghidraAddress NTSC-U/C: 0x00132c60
     * @ghidraAddress PAL: 0x001334a0
     */
    virtual int IsNull();

    /**
     * Report the track this player occupies.
     *
     * Slot 4. Returns -1 here. LocalPlayer and NetPlayer return the track the last TrackSelectMsg
     * for the player chose, and powerups deploy and riffs play on it.
     *
     * @return The track, or -1.
     * @ghidraAddress NTSC-U/C: 0x00132c98
     * @ghidraAddress PAL: 0x001334d8
     */
    virtual int GetTrack();

    /**
     * Report the player's place on its track.
     *
     * Slot 5. Returns zero here. LocalPlayer and NetPlayer return the second word of the last
     * TrackSelectMsg for the player, the place that TrackSelectPacket includes beside the track.
     *
     * @return The place.
     * @ghidraAddress NTSC-U/C: 0x00132ca0
     * @ghidraAddress PAL: 0x001334e0
     */
    virtual int GetPlace();

    /**
     * Report zero.
     *
     * Slot 6. LocalPlayer's override also returns zero, and the image has no caller of the slot.
     * The title records that the slot is unused.
     *
     * @return Always 0.
     * @ghidraAddress NTSC-U/C: 0x00132ca8
     * @ghidraAddress PAL: 0x001334e8
     */
    virtual int UnusedQuery();

    /**
     * Do nothing.
     *
     * Slot 7. LocalPlayer's override is also empty, and the image has no caller of the slot. The
     * title records that the slot is unused.
     *
     * @ghidraAddress NTSC-U/C: 0x00132cb0
     * @ghidraAddress PAL: 0x001334f0
     */
    virtual void UnusedHook();

    /**
     * Give the player a span of bars it plays freely.
     *
     * Slot 8. Empty here. LocalPlayer stores both bars, and IsFreestyleBar() tests a bar against
     * them. Gamer passes the eight-bar span of an enable-freestyle powerup and the span a solo win
     * grants, and a jam player's constructor passes a span that never ends.
     *
     * @param nStartBar The first bar of the span.
     * @param nEndBar The bar after the span.
     * @ghidraAddress NTSC-U/C: 0x00132cb8
     * @ghidraAddress PAL: 0x001334f8
     */
    virtual void SetFreestyleSpan(int nStartBar, int nEndBar);

    /**
     * Test whether a bar lies in the player's freestyle span.
     *
     * Slot 9. Returns zero here, ignoring its argument. NotePitcher plays a bar inside the span
     * without the track's usual requirement.
     *
     * @param nBar The bar.
     * @return Non-zero when the bar is in the span.
     * @ghidraAddress NTSC-U/C: 0x00132cc0
     * @ghidraAddress PAL: 0x00133500
     */
    virtual int IsFreestyleBar(int nBar);

    /**
     * Report whether the player is looping.
     *
     * Slot 10. Returns zero here. LocalPlayer returns the flag SetLooping() and ToggleLoop()
     * maintain.
     *
     * @return Non-zero when looping.
     * @ghidraAddress NTSC-U/C: 0x00132cc8
     * @ghidraAddress PAL: 0x00133508
     */
    virtual int IsLooping();

    /**
     * Announce the player's state.
     *
     * Slot 11. Here, builds a `JuiceAmountMsg` on the stack for this player, with mMaxJuice
     * clamped to a maximum of 800, and sends it through the `MsgSource` subobject. LocalPlayer adds
     * its track, powerups, score ceiling, ghost, and loop state.
     *
     * @ghidraAddress NTSC-U/C: 0x0012f788
     * @ghidraAddress PAL: 0x0012ff40
     */
    virtual void AnnounceState();

    /**
     * Deactivate the powerup placer.
     *
     * Slot 12. Empty here. LocalPlayer runs PowerupPlacer::Deactivate(), the counterpart of the
     * PowerupPlacer::Activate() that AnnounceState() runs. The image has no caller of the slot but
     * StopMF(), itself uncalled. The title is inferred from the forwarding alone.
     *
     * @ghidraAddress NTSC-U/C: 0x00132d30
     * @ghidraAddress PAL: 0x00133570
     */
    virtual void DeactivatePlacer();

    /**
     * Write a description of this player to stream.
     *
     * Writes the literal "{player null}" when IsNull() reports true, and otherwise "{player "
     * followed by the identifier at `+0x20`. Those two literals at `0x007d2048` and `0x007d2058`
     * are what attest both this routine and IsNull().
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x00133110
     * @ghidraAddress PAL: 0x00133960
     */
    virtual void Print(std::ostream &stream);

    /**
     * Count one caught gem.
     *
     * Slot 14. Catcher runs it for every gem the player catches and discards the result. Does not
     * touch the return register here. The value it yields is therefore indeterminate. LocalPlayer
     * increments its count and returns the new value. A base whose default yields an indeterminate
     * value is faithful to the image rather than a reconstruction error.
     *
     * @return The new count.
     * @ghidraAddress NTSC-U/C: 0x00132d70
     * @ghidraAddress PAL: 0x001335b0
     */
    virtual int CountCaughtGem();

    /**
     * Count one missed gem.
     *
     * Slot 15. Catcher runs it when a gem passes uncaught. Does not touch the return register
     * here, as with CountCaughtGem(). LocalPlayer increments its count and returns the new value.
     *
     * @return The new count.
     * @ghidraAddress NTSC-U/C: 0x00132d78
     * @ghidraAddress PAL: 0x001335b8
     */
    virtual int CountMissedGem();

    /**
     * Report the score multiplier a capture starting at a bar earns.
     *
     * Slot 16. Returns 1 here, ignoring its argument. Catcher and Scratcher pass the result in a
     * BeginPhraseCatchMsg.
     *
     * @param nBar The bar.
     * @return The multiplier.
     * @ghidraAddress NTSC-U/C: 0x00132d80
     * @ghidraAddress PAL: 0x001335c0
     */
    virtual int GetMultiplier(int nBar);

    /**
     * Report the best streak of consecutive captures.
     *
     * Slot 17. Returns zero here. Gamer records it as the solo tally.
     *
     * @return The streak.
     * @ghidraAddress NTSC-U/C: 0x00132d88
     * @ghidraAddress PAL: 0x001335c8
     */
    virtual int GetBestStreak();

    /**
     * Report the proportion of captured phrases among those captured and muffed.
     *
     * Slot 18. Returns 0.0f here. Gamer records it as the solo ratio.
     *
     * @return A fraction between 0 and 1.
     * @ghidraAddress NTSC-U/C: 0x00132d90
     * @ghidraAddress PAL: 0x001335d0
     */
    virtual float GetCaptureRatio();

    /**
     * Report the game mode the player was built in.
     *
     * Slot 19. Returns zero here.
     *
     * @return The game mode.
     * @ghidraAddress NTSC-U/C: 0x00132da0
     * @ghidraAddress PAL: 0x001335e0
     */
    virtual int GetGameMode();

    /**
     * Record that a bar scored, reporting whether it had not scored before.
     *
     * Slot 20. Returns 1 here, ignoring its argument. Scratcher does not award points for a bar
     * the slot reports zero for.
     *
     * @param nBar The bar.
     * @return Non-zero when the bar is later than every bar recorded before.
     * @ghidraAddress NTSC-U/C: 0x00132db0
     * @ghidraAddress PAL: 0x001335f0
     */
    virtual int MarkBarScored(int nBar);

    /**
     * Receive one message.
     *
     * The only `MsgSink` virtual this class overrides, at slot 3 of its `MsgSink` table. An
     * UpdateScorePacket for this player adds its delta without notifying, and a
     * PhraseCapturedMsg is awarded through OnMsg() whichever player it identifies. Every other
     * message is discarded.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x00133240
     * @ghidraAddress PAL: 0x00133a90
     */
    virtual void DispatchPriv(Message *pMsg);

    // Declared in recovered offset order. The base subobjects occupy +0x00 through +0x1f.
    /**
     * Identifier a message addresses this player by.
     *
     * Print() writes it after the "{player " literal and DispatchPriv compares it against a
     * field of an incoming message. Gem reads it directly at `0x001a2608` and `0x001a2d84` from
     * outside the hierarchy, and the image exposes no accessor. The member is therefore public
     * here. A friend declaration for Gem fits the image equally well.
     *
     * +0x20
     */
    int mPlayerId;

    /**
     * The player's colour name, such as `green` or `red`. +0x24
     *
     * ~Player releases its buffer at `+0x28` with the inlined HxStr destructor. HudScore, HudFreq,
     * and HudScorePulse copy-construct it directly at `0x00419848`, `0x00419b40`, and `0x0041c2d0`
     * from outside the hierarchy, and the image has no accessor for it.
     */
    HxStr mColorName;

    /**
     * Report the juice the player has banked.
     *
     * The routine at `0x0012f970` clamps the value to 0 through mMaxJuice, the maximum
     * JuiceAmountMsg announces, and the message accessor at `0x003e4198` divides it by that
     * announced maximum.
     *
     * @return The juice.
     * @ghidraAddress NTSC-U/C: 0x001330f8
     * @ghidraAddress PAL: 0x00133948
     */
    int GetJuice();

    /**
     * Report the player's score.
     *
     * The routine at `0x0012f808` clamps the value to 0 through mMaxScore. Renderer compares the
     * scores of every world player through this accessor to find the leader.
     *
     * @return The score.
     * @ghidraAddress NTSC-U/C: 0x001330e0
     * @ghidraAddress PAL: 0x00133930
     */
    int GetScore();

    /**
     * Set the score and the ceiling it is clamped to.
     *
     * Gamer's constructor sets every player to 0 with a ceiling of 100000. The title is inferred.
     *
     * @param nScore The score.
     * @param nMaxScore The ceiling.
     * @ghidraAddress NTSC-U/C: 0x001330e8
     * @ghidraAddress PAL: 0x00133938
     */
    void SetScore(int nScore, int nMaxScore);

    /**
     * Set the juice and the ceiling it is clamped to.
     *
     * Gamer's constructor passes two configuration values in kGameModeSolo and zeros otherwise.
     * The title is inferred.
     *
     * @param nJuice The juice.
     * @param nMaxJuice The ceiling.
     * @ghidraAddress NTSC-U/C: 0x00133100
     * @ghidraAddress PAL: 0x00133950
     */
    void SetJuice(int nJuice, int nMaxJuice);

    /**
     * Add juice, clamped to 0 through the ceiling, and announce a change.
     *
     * A change sends a JuiceAmountMsg through the MsgSource subobject and, when bNotify is set,
     * an UpdateScorePacket with the amount. The script command that adds juice is the recovered
     * caller. The title is inferred.
     *
     * @param nAmount The juice to add, which may be negative.
     * @param bNotify Non-zero to also send the UpdateScorePacket.
     * @ghidraAddress NTSC-U/C: 0x0012f970
     * @ghidraAddress PAL: 0x00130128
     */
    void AddJuice(int nAmount, int bNotify);

    /**
     * Add to the score, clamped to 0 through the ceiling, and announce a change.
     *
     * The score counterpart of AddJuice(). A change sends a PointAmountMsg with the ceiling capped
     * at 800 and, when bNotify is set, an UpdateScorePacket with the delta. PhraseNeutralizer is a
     * recovered caller. The title is inferred.
     *
     * @param nDelta The points to add, which may be negative.
     * @param bNotify Non-zero to also send the UpdateScorePacket.
     * @ghidraAddress NTSC-U/C: 0x0012f808
     * @ghidraAddress PAL: 0x0012ffc0
     */
    void AddScore(int nDelta, int bNotify);

    /**
     * Add a captured phrase's score and juice, announcing both.
     *
     * LocalPlayer::DispatchPriv() and the routine at `0x00122be8` are the callers.
     *
     * @param msg The capture.
     * @ghidraAddress NTSC-U/C: 0x001331c8
     * @ghidraAddress PAL: 0x00133a18
     */
    void OnMsg(const PhraseCapturedMsg &msg);

    /**
     * Report whether the player has an input slot.
     *
     * Inline, and TrackSelector::DispatchPriv() expands it. The address is its uncalled
     * out-of-line copy.
     *
     * @return Non-zero when GetInputSlot() reports a value other than -1.
     * @ghidraAddress NTSC-U/C: 0x00132c30
     * @ghidraAddress PAL: 0x00133470
     */
    int IsLocal() {
        return GetInputSlot() != kNoInputSlot;
    }

    /**
     * Delete a player through its virtual destructor, doing nothing for null.
     *
     * Inline. GrooveWorld::DeletePlayers() passes it to `std::for_each`, which is what gives it
     * the out-of-line copy at this address. The title is inferred.
     *
     * @param pPlayer The player to delete, or null.
     * @ghidraAddress NTSC-U/C: 0x00132d38
     * @ghidraAddress PAL: 0x00133578
     */
    static void Delete(Player *pPlayer) {
        delete pPlayer;
    }

    /**
     * Copy the player's colour name.
     *
     * Inline. The address is its uncalled out-of-line copy. The title is inferred.
     *
     * @return mColorName, by value.
     * @ghidraAddress NTSC-U/C: 0x00132c68
     * @ghidraAddress PAL: 0x001334a8
     */
    HxStr GetColorName();

    /**
     * Copy the username of the player's appearance.
     *
     * Inline. The address is its uncalled out-of-line copy. The title is inferred.
     *
     * @return FreqAppearance::mUserName of mAppearance, by value.
     * @ghidraAddress NTSC-U/C: 0x001330a8
     * @ghidraAddress PAL: 0x001338f8
     */
    HxStr GetUsername();

    /**
     * Run AnnounceState() and report zero.
     *
     * Inline. The address is its uncalled out-of-line copy.
     *
     * @return Always 0.
     * @ghidraAddress NTSC-U/C: 0x00132cd0
     * @ghidraAddress PAL: 0x00133510
     */
    int StartMF();

    /**
     * Run DeactivatePlacer() and report zero.
     *
     * Inline. The address is its uncalled out-of-line copy.
     *
     * @return Always 0.
     * @ghidraAddress NTSC-U/C: 0x00132d00
     * @ghidraAddress PAL: 0x00133540
     */
    int StopMF();

private:
    // NTSC-U/C: 0x00133210, PAL: 0x00133a60
    // Inline, and DispatchPriv() expands it. Adds the packet's delta without notifying when the
    // packet names this player.
    void OnUpdateScore(UpdateScorePacket *pPacket);

    // The persona's appearance. GrooveWorld::AddLocalPlayer() passes MetPersonaData::mAppearance.
    const FreqAppearance *mAppearance; // +0x2c
    int mJuice;                        // +0x30

protected:
    // The ceiling AddJuice() clamps mJuice to. AnnounceState() clamps it to kJuiceMaximum before
    // announcing it.
    int mMaxJuice; // +0x34

private:
    int mScore; // +0x38

protected:
    // The ceiling AddScore() clamps mScore to. LocalPlayer::AnnounceState() announces it.
    int mMaxScore; // +0x3c

public:
    /**
     * Scheduler time of this player's last erase press, in nanoseconds.
     *
     * The constructor at `0x0012f5c0` zeroes it. InputMap::OnControllerReading() reads and writes
     * it directly at `0x00119a6c` and `0x00119adc` to detect a double tap, and the image has no
     * accessor for it. +0x40
     */
    long long mLastEraseTime;
};
