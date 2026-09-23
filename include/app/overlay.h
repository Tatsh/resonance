#pragma once

#include <vector>

#include "app/msgsink.h"
#include "msg/barstatusmsg.h"
#include "os/hxstr.h"

class HudBadge;
class HudPanel;
class HudTrack;
class Message;
class Player;
class Renderer;

/**
 * Head-up display the in-game renderer draws over the tunnel.
 *
 * `7Overlay` in the RTTI descriptor at `0x008efd80`, with MsgSink as its one public base at offset
 * 0. Its type function is at `0x00429580`. The class sits in `app/` beside Renderer, its one
 * constructor caller. An earlier pass called the constructor `HudDisplay__Construct`, and no
 * descriptor among the 574 in the image bears that name. The table the constructor installs has
 * this class's type function in slot 0.
 *
 * The table at `0x008194a8` has four entries, the same length as MsgSink's table at `0x007ccc40`,
 * and the class therefore introduces no virtual. It overrides the destructor at slot 1 and
 * HandleMessage() at slot 3, and inherits MsgSink::Handle() at slot 2.
 *
 * The object is 0x5c bytes, the size Renderer's constructor requests under the MsgSink tag at
 * `0x0042c8cc`. The destructor frees every badge with the scalar free and deletes every track
 * display, then deletes the HudPanel (its implicit destructor inlined, then the scalar free),
 * clears GfxDevice::mFeedbackEnabled, and releases the name and kind vectors.
 */
class Overlay : public MsgSink {
public:
    /**
     * Build the display for the game about to start.
     *
     * Records the renderer, configuration code 0x3a1, the game mode, the play mode, and the last
     * bar. The layout number is the count of world players with a slot, with three players using
     * the four-player layout. The constructor sets g_hudLayoutName from it, swaps `hud<n>.view`
     * into `hud.view` in place of the other layouts, runs the layout at mMsPerTick, and hides
     * every layout child other than `hud.cam` and `hud.env`. It then builds one track display per
     * player with a slot, one badge per player, the panel, and the name and kind of each of the
     * level's eight tracks. A jukebox session shows the jukebox prompt, the song name, and the
     * level caption, and snaps the assembly and letterbox animations. Otherwise the assembly
     * animation starts. Jam mode pulses every badge icon. Records itself in g_pOverlay at the end.
     *
     * @param pRenderer The renderer that constructs this object.
     * @ghidraAddress 0x0041c940
     */
    Overlay(Renderer *pRenderer);

    /**
     * Release every display object and clear g_pOverlay.
     *
     * Each track display goes through its deleting destructor at `0x0042aa18`, and the panel's
     * implicit destructor is inlined.
     *
     * @ghidraAddress 0x0041da00
     */
    virtual ~Overlay();

    /**
     * Act on one message the renderer sends on.
     *
     * Compares the identity against twenty-four registered message identities in a fixed order
     * and runs one handler for each. A PowerupCountMsg is recognised and ignored.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x004206e0
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Advance the display to one song position.
     *
     * Renderer::OnUnknownSlot7() is the caller. The bar is the position divided by 1920 ticks, and
     * a change of bar is recorded in mCurrentBar and relights every track display's effect lamps
     * from the renderer's cell for the new bar. The time the text animations run against is the
     * position scaled by mMsPerTick. While the game manager plays a recording back, the panel
     * message shows the demo prompt. The title is inferred from the caller.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x0041dd20
     */
    void SetFrame(float flFrame);

    /**
     * Draw the display.
     *
     * Draws the win message over the frame feedback while its sequence runs, and does nothing
     * otherwise. The title is inferred.
     *
     * @ghidraAddress 0x0042acb8
     */
    void Draw();

    /**
     * Update the track display for one bar whose state a BarStatusMsg changed.
     *
     * Does nothing unless the bar is the current bar in mCurrentBar. Otherwise sets the effect
     * lamps of every track display on that track from the value. The title is inferred.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param effects The cell's effect mask.
     * @ghidraAddress 0x0042ad10
     */
    void OnBarChanged(int nTrack, int nBar, BarStatusMsg::Effects effects);

    /**
     * Move the leader marker from one player to another.
     *
     * The score pulse moves to the new leader's badge, or hides when no player leads, and the new
     * leader's FreQ icon pulses in place of the old leader's. The title is inferred.
     *
     * @param pOldLeader The previous leader, or null.
     * @param pNewLeader The new leader, or null.
     * @ghidraAddress 0x0042ad98
     */
    void OnLeaderChanged(Player *pOldLeader, Player *pNewLeader);

private:
    // HandleMessage() runs one of the handlers below per message identity. Each handler written
    // "inlined" is expanded in place there, and its out-of-line copy has no caller. Most handlers
    // act on the track display of the message's player, which FindTrack() looks up, and show text
    // through HudTextMessage::Show() at scale 1 for 1500 unless noted.

    // 0x0041fdd8
    // TrackSelectMsg. Shows the track's instrument name on the selecting player's
    // label, records the track, lights the effect lamps from the renderer's cell for the current
    // bar, and banks the player's points unless mUnknown44 is set.
    void OnTrackSelect(Message *pMsg);

    // 0x0042aec8
    // inlined. GameOverMsg. Runs script template 1001 when mUnknown44 is set.
    void OnGameOver();

    // 0x0041e020
    // WinMsg. Resets every multiplier to 1. With the win sequence enabled in
    // kGameModeSolo and a winner, starts the win message. Otherwise shows `YOU WIN`, `GAME OVER`,
    // or `YOU LOSE` on each track display at scale 2 for 3000, over two lines when there are two
    // or more displays, with the freestyle prompt for a solo winner.
    void OnWin(Message *pMsg);

    // 0x0041e9b8
    // ChoosePowerupMsg. Shows the chosen kind on the player's powerup indicator in
    // kPlayModeGame, and selects its effect lamp name otherwise. Runs script template 1016 when
    // mUnknown44 is set.
    void OnChoosePowerup(Message *pMsg);

    // 0x0041eba0
    // CaughtPowerbarMsg. Shows `<kind>\nCAPTURED` in the player's text message for
    // 1500. HandleMessage() ignores a PowerupCountMsg outright.
    void OnCaughtPowerbar(Message *pMsg);

    // 0x0041eda8
    // DeployedPowerupMsg. In kPlayModeGame, shows `<kind>\nDEPLOYED`, sets the
    // display's mUnknownec, and shows `YOU GOT\nBUMPED!` on the target's display for a bumper.
    void OnDeployedPowerup(Message *pMsg);

    // 0x0042aff0
    // inlined. PointAmountMsg. Records the new score in the player's badge, pending an
    // untimed redraw.
    void OnPointAmount(Message *pMsg);

    // 0x0041f310
    // JuiceAmountMsg. In kGameModeSolo and kPlayModeGame, sets the player's energy
    // level to the juice amount and pulses the player's icon while the juice is above 0.85.
    void OnJuiceAmount(Message *pMsg);

    // 0x0042b068
    // inlined. PhraseCapturedMsg. Runs script template 1005 when mUnknown44 is set.
    // Otherwise, in kPlayModeGame before the bar in mLastBar, shows the capturing player's
    // points leaving.
    void OnPhraseCaptured(Message *pMsg);

    // 0x0041f5e8
    // TextMsg. Shows the message's text in the first track display's text message.
    void OnText(Message *pMsg);

    // 0x0041f708
    // LoopToggleMsg. Outside kPlayModeGame, shows the player's loop indicator and,
    // once the song is under way, `LOOP ON` or `LOOP OFF`. Runs script template 1011 when
    // mUnknown44 is set.
    void OnLoopToggle(Message *pMsg);

    // 0x0041f440
    // AdvanceSectionToggleMsg. Without mUnknown44, restyles the section blocks and,
    // outside playback, shows `ADVANCE TO\nNEXT SECTION` or `REPEAT\nSECTION` on every display.
    void OnAdvanceSectionToggle(Message *pMsg);

    // 0x0041fba0
    // ShowEraseEffectMsg. Shows `BAR ERASED` for a range under two bars and
    // `TRACK ERASED` otherwise.
    void OnShowEraseEffect(Message *pMsg);

    // 0x0041f9a8
    // PlaybackToggleMsg. Records the state in mPlaybackOn, shows or hides the edit
    // prompt, runs the assembly and letterbox animations the matching way, hides the FreQ icons
    // during playback, and hides every text message.
    void OnPlaybackToggle(Message *pMsg);

    // 0x0042aef8
    // inlined. ToggleGhostMsg. Outside kPlayModeGame, lights or darkens the player's
    // kHudItemGuides lamp. Runs script template 1021 when mUnknown44 is set.
    void OnToggleGhost(Message *pMsg);

    // 0x0042b130
    // inlined. JamEffectMsg. Runs script template 1017 in kPlayModeJam when mUnknown44
    // is set. The message is not read.
    void OnJamEffect();

    // 0x0041fed8
    // CatchMsg. In kPlayModeGame before the last bar, pulses the points readout to
    // the share of the phrase caught. In an easy solo game without mUnknown44, counts catches on
    // bars that cannot be captured and shows `ROTATE TO\nNEW TRACK` at the third.
    void OnCatch(Message *pMsg);

    // 0x0042b178
    // inlined. PhraseMuffedMsg. In kPlayModeGame without mUnknown44, banks the
    // player's points.
    void OnPhraseMuffed(Message *pMsg);

    // 0x004201c0
    // BeginPhraseCatchMsg. In kPlayModeGame before the last bar and without
    // mUnknown44, shows the phrase's points and multiplier on the player's readout.
    void OnBeginPhraseCatch(Message *pMsg);

    // 0x0042b1f8
    // inlined. FadeGameMsg. Starts the screen flash over the message's duration, and
    // hides the win message's prompt when the game fades out.
    void OnFadeGame(Message *pMsg);

    // 0x00420588
    // PlayersTrackNeutralizedMsg. Shows `NEUTRALIZED!\n<points> POINTS`.
    void OnPlayersTrackNeutralized(Message *pMsg);

    // 0x00420408
    // MultiplierStateMsg. Without mUnknown44 and before the last bar, shows the base
    // plus the bonus multiplier and selects the hot material while a bonus applies.
    void OnMultiplierState(Message *pMsg);

    // 0x0041f138
    // PowerupFailedMsg. Shows the failure text for the powerup kind at scale 0.8.
    void OnPowerupFailed(Message *pMsg);

    // 0x0042ae88
    // The badge whose mPlayer is pPlayer, or null.
    HudBadge *FindBadge(Player *pPlayer);

    // The track display whose mPlayer is pPlayer, or null. Every handler inlines the search, and
    // no out-of-line copy is recovered.
    HudTrack *FindTrack(Player *pPlayer);

    // Sets the layout prefix g_hudLayoutName to `HUD<n>`. The constructor inlines the body, and
    // this copy at 0x00429938 has no caller.
    static void SetLayoutName(int nLayout);

    // The parts that belong to the whole screen, deleted by the destructor.
    HudPanel *mPanel;
    // One track display per world player that has a slot, deleted by the destructor.
    std::vector<HudTrack *> mTracks;
    // One badge per world player.
    std::vector<HudBadge *> mBadges;
    // One instrument name per track, which a TrackSelectMsg shows on the selecting player's label.
    std::vector<HxStr> mInstrumentNames;
    // One TrackData::mKind per track.
    std::vector<int> mTrackKinds;
    Renderer *mRenderer;
    // Globals::GetGameMode() at construction.
    int mGameMode;
    // Globals::GetPlayMode() at construction.
    int mPlayMode;
    // Configuration code 0x3a1.
    int mUnknown44; // +0x44
    // The state of the last PlaybackToggleMsg. The constructor starts it at 0.
    int mPlaybackOn;
    // The bar SetFrame() last saw. The constructor starts it at -123123.
    int mCurrentBar;
    // Milliseconds per MIDI tick at the tempo in force at construction. SetFrame() times the text
    // animations with it.
    float mMsPerTick;
    // The last bar of the level, PlayMap::Slot9().
    int mLastBar;
    // The session difficulty, GameManagerImpl::GetDifficulty() at construction.
    int mDifficulty;
};

/**
 * The display that exists, or null.
 *
 * The constructor stores the object and the destructor clears the word.
 *
 * @ghidraAddress 0x006dfdf8
 */
extern Overlay *g_pOverlay;

/**
 * Prefix of every per-layout head-up display object name, `HUD<n>` for layout n.
 *
 * Overlay's constructor sets it before any HUD class resolves an object, and every HUD class
 * formats it into the names it resolves. The translation unit's static initialiser at `0x00429348`
 * constructs it. The Ghidra program labels it `g_abHudLayoutName`, the prefix its naming check
 * requires for an aggregate. It also labels the string pointer inside it, at `0x006dfdf4`, as
 * `g_szPlayerName`, and that label is wrong.
 *
 * @ghidraAddress 0x006dfdf0
 */
extern HxStr g_hudLayoutName;
