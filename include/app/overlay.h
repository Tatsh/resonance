#pragma once

#include <vector>

#include "app/msgsink.h"
#include "os/hxstr.h"

class HudBadge;
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
 * `0x0042c8cc`. The destructor releases the display object at `+0x04` (0x164 bytes, torn down
 * through `0x0041ac18` and then the scalar free), deletes every element of the vector at `+0x08`
 * through `0x0042aa18`, frees every element of the vector at `+0x14` with the scalar free, and
 * releases the string vector at `+0x20` and the vector at `+0x2c`. The panel class is not declared
 * yet, and the panel pointer is recorded by size.
 */
class Overlay : public MsgSink {
public:
    /**
     * Build the display for the game about to start.
     *
     * Records the renderer, reads configuration code 0x3a1, and records the game mode and the play
     * mode before loading the layout `HUD layout%d`. Records itself in g_pOverlay at the end. The
     * body is not written.
     *
     * @param pRenderer The renderer that constructs this object.
     * @ghidraAddress 0x0041c940
     */
    Overlay(Renderer *pRenderer);

    /**
     * Release every display object and clear g_pOverlay.
     *
     * The body is not written. It needs the destructors at `0x0041ac18` and `0x0042aa18`.
     *
     * @ghidraAddress 0x0041da00
     */
    virtual ~Overlay();

    /**
     * Act on one message the renderer sends on.
     *
     * Compares the identity against about twenty registered message identities, among them
     * TrackSelectMsg, GameOverMsg, ChoosePowerupMsg, PointAmountMsg, PhraseCapturedMsg, and
     * FadeGameMsg, and runs one handler for each. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x004206e0
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Advance the display to one song position.
     *
     * Renderer::OnUnknownSlot7() is the caller. The bar is the position divided by 1920 ticks, and
     * a change of bar is recorded in mUnknown4c. The title is inferred from the caller. The body is
     * not written.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x0041dd20
     */
    void SetFrame(float flFrame);

    /**
     * Draw the display.
     *
     * Does nothing until the display object at `+0x04` reports itself ready. The title is
     * inferred. The body is not written.
     *
     * @ghidraAddress 0x0042acb8
     */
    void Draw();

    /**
     * Update the track display for one bar whose state a BarStatusMsg changed.
     *
     * Does nothing unless the bar is the current bar in mUnknown4c. Otherwise sets the effect lamps
     * of every track display on that track from the value. The title is inferred.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param llValue The cell's `mUnknown10`.
     * @ghidraAddress 0x0042ad10
     */
    void OnBarChanged(int nTrack, int nBar, long long llValue);

    /**
     * Move the leader marker from one player to another.
     *
     * The title is inferred. The body is not written.
     *
     * @param pOldLeader The previous leader, or null.
     * @param pNewLeader The new leader, or null.
     * @ghidraAddress 0x0042ad98
     */
    void OnLeaderChanged(Player *pOldLeader, Player *pNewLeader);

private:
    // HandleMessage() runs one of the handlers below per message identity. Each handler written
    // "inlined" is expanded in place there, and its out-of-line copy has no caller. The bodies not
    // written read message payloads their message headers declare private.

    // 0x0041fdd8. TrackSelectMsg. Shows the track's instrument name on the selecting player's
    // label, records the track, lights the effect lamps from the renderer's cell for the current
    // bar, and banks the player's points unless mUnknown44 is set.
    void OnTrackSelect(Message *pMsg);

    // 0x0042aec8, inlined. GameOverMsg. Runs script template 1001 when mUnknown44 is set.
    void OnGameOver();

    // 0x0041e020. WinMsg.
    void OnWin(Message *pMsg);

    // 0x0041e9b8. ChoosePowerupMsg. Shows the chosen kind on the player's powerup indicator in
    // kPlayModeGame, and selects its effect lamp name otherwise. Runs script template 1016 when
    // mUnknown44 is set.
    void OnChoosePowerup(Message *pMsg);

    // 0x0041eba0. PowerupCountMsg and CaughtPowerbarMsg. Shows `<kind>\nCAPTURED` in the player's
    // text message for 1500.
    void OnPowerupCount(Message *pMsg);

    // 0x0041eda8. DeployedPowerupMsg.
    void OnDeployedPowerup(Message *pMsg);

    // 0x0042aff0, inlined. PointAmountMsg. Records the new score in the player's badge, pending an
    // untimed redraw.
    void OnPointAmount(Message *pMsg);

    // 0x0041f310. JuiceAmountMsg. In kGameModeSolo and kPlayModeGame, sets the player's energy
    // level to the juice amount and pulses the player's icon while the juice is above 0.85.
    void OnJuiceAmount(Message *pMsg);

    // 0x0042b068, inlined. PhraseCapturedMsg. Runs script template 1005 when mUnknown44 is set.
    // Otherwise, in kPlayModeGame before the bar in mUnknown54, shows the capturing player's
    // points leaving.
    void OnPhraseCaptured(Message *pMsg);

    // 0x0041f5e8. TextMsg. Shows the message's text in the first track display's text message.
    void OnText(Message *pMsg);

    // 0x0041f708. LoopToggleMsg.
    void OnLoopToggle(Message *pMsg);

    // 0x0041f440. AdvanceSectionToggleMsg.
    void OnAdvanceSectionToggle(Message *pMsg);

    // 0x0041fba0. ShowEraseEffectMsg.
    void OnShowEraseEffect(Message *pMsg);

    // 0x0041f9a8. PlaybackToggleMsg.
    void OnPlaybackToggle(Message *pMsg);

    // 0x0042aef8, inlined. ToggleGhostMsg. Outside kPlayModeGame, lights or darkens the player's
    // kHudItemGuides lamp. Runs script template 1021 when mUnknown44 is set.
    void OnToggleGhost(Message *pMsg);

    // 0x0042b130, inlined. JamEffectMsg. Runs script template 1017 in kPlayModeJam when mUnknown44
    // is set. The message is not read.
    void OnJamEffect();

    // 0x0041fed8. CatchMsg.
    void OnCatch(Message *pMsg);

    // 0x0042b178, inlined. PhraseMuffedMsg. In kPlayModeGame without mUnknown44, banks the
    // player's points.
    void OnPhraseMuffed(Message *pMsg);

    // 0x004201c0. BeginPhraseCatchMsg.
    void OnBeginPhraseCatch(Message *pMsg);

    // 0x0042b1f8, inlined. FadeGameMsg. Starts the screen flash over the message's duration, and
    // hides the panel's message text when the message's second word is zero.
    void OnFadeGame(Message *pMsg);

    // 0x00420588. PlayersTrackNeutralizedMsg.
    void OnPlayersTrackNeutralized(Message *pMsg);

    // 0x00420408. MultiplierStateMsg.
    void OnMultiplierState(Message *pMsg);

    // 0x0041f138. PowerupFailedMsg.
    void OnPowerupFailed(Message *pMsg);

    // 0x0042ae88. The badge whose mPlayer is pPlayer, or null.
    HudBadge *FindBadge(Player *pPlayer);

    // Sets the layout prefix g_hudLayoutName to `HUD<n>`. The constructor inlines the body, and
    // this copy at 0x00429938 has no caller.
    static void SetLayoutName(int nLayout);

    // The 0x164-byte panel, deleted by the destructor. +0x04
    unsigned char mUnknown04[0x04];
    // One track display per world player that has a slot, deleted by the destructor.
    std::vector<HudTrack *> mUnknown08; // +0x08
    // One badge per world player.
    std::vector<HudBadge *> mUnknown14; // +0x14
    // One instrument name per track, which a TrackSelectMsg shows on the selecting player's label.
    std::vector<HxStr> mUnknown20; // +0x20
    // One word per track, read from the track description's `+0x0c`.
    std::vector<int> mUnknown2c; // +0x2c
    Renderer *mUnknown38;        // +0x38
    // Globals::GetGameMode() at construction.
    int mUnknown3c; // +0x3c
    // Globals::GetPlayMode() at construction.
    int mUnknown40; // +0x40
    // Configuration code 0x3a1.
    int mUnknown44; // +0x44
    int mUnknown48; // +0x48
    // The bar SetFrame() last saw. The constructor starts it at -123123.
    int mUnknown4c;                  // +0x4c
    float mUnknown50;                // +0x50
    int mUnknown54;                  // +0x54
    unsigned char mReserved58[0x04]; // +0x58
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
