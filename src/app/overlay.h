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
 * Its RTTI descriptor is at `0x008efd80`. It has MsgSink as its one public base at offset 0. Its
 * type function is at `0x00429580`. The class sits in `app/` beside Renderer, its one constructor
 * caller. The table the constructor installs has this class's type function in slot 0.
 *
 * The table at `0x008194a8` has four entries, the same length as MsgSink's table at `0x007ccc40`,
 * and the class therefore introduces no virtual. It overrides the destructor at slot 1 and
 * DispatchPriv() at slot 3, and inherits MsgSink::Dispatch() at slot 2.
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
     * @ghidraAddress NTSC-U/C: 0x0041c940
     * @ghidraAddress PAL: 0x004573a0
     */
    Overlay(Renderer *pRenderer);

    /**
     * Release every display object and clear g_pOverlay.
     *
     * Each track display goes through its deleting destructor at `0x0042aa18`, and the panel's
     * implicit destructor is inlined.
     *
     * @ghidraAddress NTSC-U/C: 0x0041da00
     * @ghidraAddress PAL: 0x004588b0
     */
    virtual ~Overlay();

    /**
     * Act on one message the renderer sends on.
     *
     * Compares the identity against twenty-four registered message identities in a fixed order
     * and runs one handler for each. A PowerupCountMsg is recognised and ignored.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x004206e0
     * @ghidraAddress PAL: 0x0045ba00
     */
    virtual void DispatchPriv(Message *pMsg);

    /**
     * Advance the display to one song position.
     *
     * Renderer::Update() is the caller. The bar is the position divided by 1920 ticks, and
     * a change of bar is recorded in mCurrentBar and relights every track display's effect lamps
     * from the renderer's cell for the new bar. The time the text animations run against is the
     * position scaled by mMsPerTick. While the game manager plays a recording back, the panel
     * message shows the demo prompt. The title is inferred from the caller.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress NTSC-U/C: 0x0041dd20
     * @ghidraAddress PAL: 0x00458bf0
     */
    void SetFrame(float flFrame);

    /**
     * Draw the display.
     *
     * Draws the win message over the frame feedback while its sequence runs, and does nothing
     * otherwise. The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x0042acb8
     * @ghidraAddress PAL: 0x004660d0
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
     * @ghidraAddress NTSC-U/C: 0x0042ad10
     * @ghidraAddress PAL: 0x00466128
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
     * @ghidraAddress NTSC-U/C: 0x0042ad98
     * @ghidraAddress PAL: 0x004661b0
     */
    void OnLeaderChanged(Player *pOldLeader, Player *pNewLeader);

    // The parts that belong to the whole screen, deleted by the destructor. Public because
    // the heads-up script commands drive them with no accessor in the image.
    HudPanel *mPanel;

private:
    // DispatchPriv() runs one of the handlers below per message identity. Each handler written
    // "inlined" is expanded in place there, and its out-of-line copy has no caller. Most handlers
    // act on the track display of the message's player, which FindTrack() looks up, and show text
    // through HudTextMessage::Show() at scale 1 for 1500 unless noted.

    /**
     * Handle a TrackSelectMsg.
     *
     * Shows the track's instrument name on the selecting player's label, records the track, lights
     * the effect lamps from the renderer's cell for the current bar, and banks the player's points
     * unless mTutorial is set.
     *
     * @ghidraAddress NTSC-U/C: 0x0041fdd8
     * @ghidraAddress PAL: 0x0045b408
     */
    void OnTrackSelect(Message *pMsg);

    /**
     * Handle a GameOverMsg.
     *
     * Runs script template 1001 when mTutorial is set. Its caller inlines this handler.
     *
     * @ghidraAddress NTSC-U/C: 0x0042aec8
     * @ghidraAddress PAL: 0x004662e0
     */
    void OnGameOver();

    /**
     * Handle a WinMsg.
     *
     * Resets every multiplier to 1. With the win sequence enabled in kGameModeSolo and a winner,
     * starts the win message. Otherwise shows `YOU WIN`, `GAME OVER`, or `YOU LOSE` on each track
     * display at scale 2 for 3000, over two lines when there are two or more displays, with the
     * freestyle prompt for a solo winner. The European release also uses two lines in French.
     *
     * @ghidraAddress NTSC-U/C: 0x0041e020
     * @ghidraAddress PAL: 0x00458fb0
     */
    void OnWin(Message *pMsg);

    /**
     * Handle a ChoosePowerupMsg.
     *
     * Shows the chosen kind on the player's powerup indicator in kPlayModeGame, and selects its
     * effect lamp name otherwise. Runs script template 1016 when mTutorial is set.
     *
     * @ghidraAddress NTSC-U/C: 0x0041e9b8
     * @ghidraAddress PAL: 0x00459b58
     */
    void OnChoosePowerup(Message *pMsg);

    /**
     * Handle a CaughtPowerbarMsg.
     *
     * Shows `<kind>\nCAPTURED` in the player's text message for 1500. DispatchPriv() ignores a
     * PowerupCountMsg outright.
     *
     * @ghidraAddress NTSC-U/C: 0x0041eba0
     * @ghidraAddress PAL: 0x00459d40
     */
    void OnCaughtPowerbar(Message *pMsg);

    /**
     * Handle a DeployedPowerupMsg.
     *
     * In kPlayModeGame, shows `<kind>\nDEPLOYED`, sets the display's mDeployedPowerup, and shows
     * `YOU GOT\nBUMPED!` on the target's display for a bumper.
     *
     * @ghidraAddress NTSC-U/C: 0x0041eda8
     * @ghidraAddress PAL: 0x00459ff8
     */
    void OnDeployedPowerup(Message *pMsg);

    /**
     * Handle a PointAmountMsg.
     *
     * Records the new score in the player's badge, pending an untimed redraw. Its caller inlines
     * this handler.
     *
     * @ghidraAddress NTSC-U/C: 0x0042aff0
     * @ghidraAddress PAL: 0x00466408
     */
    void OnPointAmount(Message *pMsg);

    /**
     * Handle a JuiceAmountMsg.
     *
     * In kGameModeSolo and kPlayModeGame, sets the player's energy level to the juice amount and
     * pulses the player's icon while the juice is above 0.85.
     *
     * @ghidraAddress NTSC-U/C: 0x0041f310
     * @ghidraAddress PAL: 0x0045a7e8
     */
    void OnJuiceAmount(Message *pMsg);

    /**
     * Handle a PhraseCapturedMsg.
     *
     * Runs script template 1005 when mTutorial is set. Otherwise, in kPlayModeGame before the bar
     * in mLastBar, shows the capturing player's points leaving. Its caller inlines this handler.
     *
     * @ghidraAddress NTSC-U/C: 0x0042b068
     * @ghidraAddress PAL: 0x00466480
     */
    void OnPhraseCaptured(Message *pMsg);

    /**
     * Handle a TextMsg.
     *
     * Shows the message's text in the first track display's text message.
     *
     * @ghidraAddress NTSC-U/C: 0x0041f5e8
     * @ghidraAddress PAL: 0x0045ab90
     */
    void OnText(Message *pMsg);

    /**
     * Handle a LoopToggleMsg.
     *
     * Outside kPlayModeGame, shows the player's loop indicator and, once the song is under way,
     * `LOOP ON` or `LOOP OFF`. Runs script template 1011 when mTutorial is set.
     *
     * @ghidraAddress NTSC-U/C: 0x0041f708
     * @ghidraAddress PAL: 0x0045acd0
     */
    void OnLoopToggle(Message *pMsg);

    /**
     * Handle an AdvanceSectionToggleMsg.
     *
     * Without mTutorial, restyles the section blocks and, outside playback, shows `ADVANCE TO\nNEXT
     * SECTION` or `REPEAT\nSECTION` on every display.
     *
     * @ghidraAddress NTSC-U/C: 0x0041f440
     * @ghidraAddress PAL: 0x0045a918
     */
    void OnAdvanceSectionToggle(Message *pMsg);

    /**
     * Handle a ShowEraseEffectMsg.
     *
     * Shows `BAR ERASED` for a range under two bars and `TRACK ERASED` otherwise.
     *
     * @ghidraAddress NTSC-U/C: 0x0041fba0
     * @ghidraAddress PAL: 0x0045b1a8
     */
    void OnShowEraseEffect(Message *pMsg);

    /**
     * Handle a PlaybackToggleMsg.
     *
     * Records the state in mPlaybackOn, shows or hides the edit prompt, runs the assembly and
     * letterbox animations the matching way, hides the FreQ icons during playback, and hides every
     * text message.
     *
     * @ghidraAddress NTSC-U/C: 0x0041f9a8
     * @ghidraAddress PAL: 0x0045af98
     */
    void OnPlaybackToggle(Message *pMsg);

    /**
     * Handle a ToggleGhostMsg.
     *
     * Outside kPlayModeGame, lights or darkens the player's kHudItemGuides lamp. Runs script
     * template 1021 when mTutorial is set. Its caller inlines this handler.
     *
     * @ghidraAddress NTSC-U/C: 0x0042aef8
     * @ghidraAddress PAL: 0x00466310
     */
    void OnToggleGhost(Message *pMsg);

    /**
     * Handle a JamEffectMsg.
     *
     * Runs script template 1017 in kPlayModeJam when mTutorial is set. The message is not read. Its
     * caller inlines this handler.
     *
     * @ghidraAddress NTSC-U/C: 0x0042b130
     * @ghidraAddress PAL: 0x00466548
     */
    void OnJamEffect();

    /**
     * Handle a CatchMsg.
     *
     * In kPlayModeGame before the last bar, pulses the points readout to the share of the phrase
     * caught. In an easy solo game without mTutorial, counts catches on bars that cannot be
     * captured and shows `ROTATE TO\nNEW TRACK` at the third.
     *
     * @ghidraAddress NTSC-U/C: 0x0041fed8
     * @ghidraAddress PAL: 0x0045b508
     */
    void OnCatch(Message *pMsg);

    /**
     * Handle a PhraseMuffedMsg.
     *
     * In kPlayModeGame without mTutorial, banks the player's points. Its caller inlines this
     * handler.
     *
     * @ghidraAddress NTSC-U/C: 0x0042b178
     * @ghidraAddress PAL: 0x00466590
     */
    void OnPhraseMuffed(Message *pMsg);

    /**
     * Handle a BeginPhraseCatchMsg.
     *
     * In kPlayModeGame before the last bar and without mTutorial, shows the phrase's points and
     * multiplier on the player's readout.
     *
     * @ghidraAddress NTSC-U/C: 0x004201c0
     * @ghidraAddress PAL: 0x00466610
     */
    void OnBeginPhraseCatch(Message *pMsg);

    /**
     * Handle a FadeGameMsg.
     *
     * Starts the screen flash over the message's duration, and hides the win message's prompt when
     * the game fades out. Its caller inlines this handler.
     *
     * @ghidraAddress NTSC-U/C: 0x0042b1f8
     * @ghidraAddress PAL: 0x004667a0
     */
    void OnFadeGame(Message *pMsg);

    /**
     * Handle a PlayersTrackNeutralizedMsg.
     *
     * Shows `NEUTRALIZED!\n<points> POINTS`.
     *
     * @ghidraAddress NTSC-U/C: 0x00420588
     * @ghidraAddress PAL: 0x0045b808
     */
    void OnPlayersTrackNeutralized(Message *pMsg);

    /**
     * Handle a MultiplierStateMsg.
     *
     * Without mTutorial and before the last bar, shows the base plus the bonus multiplier and
     * selects the hot material while a bonus applies.
     *
     * @ghidraAddress NTSC-U/C: 0x00420408
     * @ghidraAddress PAL: 0x004666e0
     */
    void OnMultiplierState(Message *pMsg);

    /**
     * Handle a PowerupFailedMsg.
     *
     * Shows the failure text for the powerup kind at scale 0.8.
     *
     * @ghidraAddress NTSC-U/C: 0x0041f138
     * @ghidraAddress PAL: 0x0045a470
     */
    void OnPowerupFailed(Message *pMsg);

    /**
     * The badge whose mPlayer is pPlayer, or null.
     *
     * @ghidraAddress NTSC-U/C: 0x0042ae88
     * @ghidraAddress PAL: 0x004662a0
     */
    HudBadge *FindBadge(Player *pPlayer);

    // The track display whose mPlayer is pPlayer, or null. Every handler inlines the search, and
    // no out-of-line copy is recovered.
    HudTrack *FindTrack(Player *pPlayer);

    /**
     * Sets the layout prefix g_hudLayoutName to `HUD<n>`.
     *
     * The constructor inlines the body, and the out-of-line copy has no caller.
     *
     * @ghidraAddress NTSC-U/C: 0x00429938
     * @ghidraAddress PAL: 0x00464f78
     */
    static void SetLayoutName(int nLayout);

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
    // Non-zero in a tutorial level, from configuration code 0x3a1.
    int mTutorial;
    // The state of the last PlaybackToggleMsg. The constructor starts it at 0.
    int mPlaybackOn;
    // The bar SetFrame() last saw. The constructor starts it at -123123.
    int mCurrentBar;
    // Milliseconds per MIDI tick at the tempo in force at construction. SetFrame() times the text
    // animations with it.
    float mMsPerTick;
    // The last bar of the level, PlayMap::GetEndBar().
    int mLastBar;
    // The session difficulty, GameManagerImpl::GetDifficulty() at construction.
    int mDifficulty;
};

/**
 * The display that exists, or null.
 *
 * The constructor stores the object and the destructor clears the word.
 *
 * @ghidraAddress NTSC-U/C: 0x006dfdf8
 * @ghidraAddress PAL: 0x00723628
 */
extern Overlay *g_pOverlay;

/**
 * Prefix of every per-layout head-up display object name, `HUD<n>` for layout n.
 *
 * Overlay's constructor sets it before any HUD class resolves an object, and every HUD class
 * formats it into the names it resolves. The translation unit's static initialiser at `0x00429348`
 * constructs it. Its string pointer is at `0x006dfdf4`.
 *
 * @ghidraAddress NTSC-U/C: 0x006dfdf0
 * @ghidraAddress PAL: 0x00723620
 */
extern HxStr g_hudLayoutName;
