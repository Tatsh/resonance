#pragma once

#include <list>
#include <vector>

#include "app/msgsink.h"
#include "app/tnlpendingtrigger.h"
#include "game/trackdata.h"

class AdvanceSectionToggleMsg;
class AxeButtonMsg;
class CatchMsg;
class ClearGemMsg;
class ClearGemsMsg;
class CripplePacket;
class DeployedPowerupMsg;
class DurGemMsg;
class DurGemTrails;
class FreestyleFXMsg;
class GemMsg;
class HxStr;
class JuiceAmountMsg;
class Message;
class MultiplierStateMsg;
class NowBarMsg;
class PhraseMuffedMsg;
class PitchMsg;
class PlayMap;
class PlaybackToggleMsg;
class Player;
class PlayersTrackNeutralizedMsg;
class PowerupFailedMsg;
class Renderer;
class SectionCapturedMsg;
class SeekerMsg;
class ShowEraseEffectMsg;
class SusGemMsg;
class TnlArms;
class TnlArrow;
class TnlBoundary;
class TnlBumpFX;
class TnlCameraRig;
class TnlCrippleFX;
class TnlFireFX;
class TnlGemManager;
class TnlLattice;
class TnlMultFX;
class TnlNowRing;
class TnlPanel;
class TnlPanelFX;
class TnlPlayer;
class TnlSnake;
class TnlTrigger;
class ToggleGhostMsg;
class TrackSelectMsg;
class WinMsg;
struct Color;
struct Vector3;
namespace Rnd {
class Cam;
class Drawable;
class Light;
class Mat;
class ParticleSys;
class View;
struct Particle;
} // namespace Rnd

/**
 * Game-side driver of the tunnel the in-game renderer draws.
 *
 * `9AppTunnel` in the RTTI descriptor at `0x008f0860`, with MsgSink as its one public base at
 * offset 0. Its type function is at `0x00453ff8`. The class sits in `app/` beside Renderer, its one
 * constructor caller, and it is a game-side class rather than a Rnd one.
 *
 * The table at `0x0081ba38` has four entries, the same length as MsgSink's table at `0x007ccc40`,
 * and the class therefore introduces no virtual. It overrides the destructor at slot 1 and
 * HandleMessage() at slot 3, and inherits MsgSink::Handle() at slot 2.
 *
 * The object is 0x170 bytes, the size Renderer's constructor requests under the MsgSink tag at
 * `0x0042c89c`. No routine of the unit reads or writes the words from `+0x14c` to `+0x15c` or from
 * `+0x164` to the end.
 *
 * The unit ends with the out-of-line copies of the inline routines of the tunnel helper classes,
 * and of the members below that HandleMessage() and the helpers inline. Those copies have no
 * caller.
 */
class AppTunnel : public MsgSink {
public:
    /**
     * Build the tunnel for the game about to start.
     *
     * Records itself in g_pAppTunnel, sets the tunnel's rates and levels of detail for the local
     * player count, and splits the screen between "tnl cam1" to "tnl cam4" and their outer
     * cameras, saving a copy of each main camera first. It then registers the gem kinds, builds
     * the gem trails, one TnlPlayer per player, and every effect, and runs "hx.nowring(1)",
     * "hx.sections(1)", and "hx.fade_activator(1)". A jam in jukebox mode starts in the playing
     * camera pose with every activator suppressed and the now ring hidden.
     *
     * @param pRenderer The renderer that constructs this object.
     * @ghidraAddress 0x00442020
     */
    AppTunnel(Renderer *pRenderer);

    /**
     * Release every tunnel object and give each main camera back its saved state.
     *
     * Clears g_pAppTunnel first. The pending triggers are not deleted.
     *
     * @ghidraAddress 0x00445740
     */
    virtual ~AppTunnel();

    /**
     * Act on one message the renderer sends on.
     *
     * The message type selects one handler. DisplayPointerMsg, ChoosePowerupMsg, StdMidiMsg, and
     * every other type are ignored, and nothing is passed on to MsgSink.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00449688
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Advance the tunnel to one song position.
     *
     * Fades the string flares, shrinks the gem flashes, retires finished panels and fired
     * triggers, and advances every helper and effect. The fire effects' views, the arrows, and the
     * players also receive the position scaled by the tempo rate. Renderer::OnUnknownSlot7() is the
     * caller. The title is inferred from that caller, which hands the same value to
     * Rnd::Animatable::SetFrame() on its three views.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x00446960
     */
    void SetFrame(float flFrame);

    /**
     * Move the leader marker from one player's activator to another's.
     *
     * Does nothing in kGameModeSolo. A null player on either side is skipped. The body is
     * TnlActivator::SetLeader() inlined for each side. The title is inferred.
     *
     * @param pOldLeader The previous leader, or null.
     * @param pNewLeader The new leader, or null.
     * @ghidraAddress 0x00446838
     */
    void OnLeaderChanged(Player *pOldLeader, Player *pNewLeader);

    /**
     * Update the tunnel section of one bar whose state a BarStatusMsg changed.
     *
     * Nothing happens once the renderer's song tick is more than a quarter bar past the end of
     * the bar. In kPlayModeGame, with nUnknown zero and a song tick that is not negative, a new
     * TnlPanel starts a quarter of the way from the song tick to the start of the bar. Otherwise
     * a TnlPanel is built on the stack and applied at once. A jukebox game always passes 0 as the
     * panel's showing flag. The title is inferred.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param nUnknown The word at `+0x14` of the BarStatusMsg.
     * @param pPlayer The cell's player.
     * @param nPowerup The cell's `mPowerup`.
     * @param nEnabled The cell's `mEnabled`.
     * @ghidraAddress 0x004465a0
     */
    void
    OnBarChanged(int nTrack, int nBar, int nUnknown, Player *pPlayer, int nPowerup, int nEnabled);

    /**
     * Turn the now ring for one local view before the renderer draws it.
     *
     * The body is TnlNowRing::SetRotation() with the view index as the step, inlined.
     * Renderer::OnUnknownSlot8() is the caller. The title is inferred.
     *
     * @param nView The index of the local view.
     * @param flFrame The song position, in MIDI ticks. The body does not read it.
     * @ghidraAddress 0x00457bd0
     */
    void PrepareLocalView(int nView, float flFrame);

    /**
     * Move the first unplaced particle of "string flare.ps" to a point.
     *
     * A particle counts as unplaced while its colour alpha differs from 1. The routine sets the
     * alpha to 1 as it places the particle, and a system whose live particles are all placed is not
     * changed. The title is inferred from the particle system and its DurGemTrails callers.
     *
     * @param pos The point, in the space of the tunnel strings.
     * @ghidraAddress 0x00457a98
     */
    void PlaceStringFlare(const Vector3 &pos);

    /**
     * Report whether the player may not act on one bar of a track.
     *
     * A riff track in a jam never locks. An axe, scratch, or vocal track always locks. Any other
     * bar locks unless its Renderer cell holds the null player and is enabled. TnlGridMarkers is
     * the caller. The title is inferred.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @return 1 when the bar is locked, 0 otherwise.
     * @ghidraAddress 0x00457338
     */
    int IsTrackBarLocked(int nTrack, int nBar);

    /**
     * Show the ghost gems of one track in a drawable.
     *
     * The drawable's draw list is emptied, the track's ghost gem kind is drawn in it and shown,
     * the track's ghost material becomes transparent, and the track's ghost fade rate becomes
     * 0.15. TnlActivator is the caller. The title is inferred.
     *
     * @param nTrack The track.
     * @param pGhost The drawable, the Rnd::Drawable subobject of the activator's ghost view.
     * @ghidraAddress 0x00457418
     */
    void ShowTrackGhost(int nTrack, Rnd::Drawable *pGhost);

    /**
     * Make the ghost material of one track opaque again and fade the ghost out at -0.15.
     *
     * TnlActivator is the caller. The title is inferred.
     *
     * @param nTrack The track.
     * @ghidraAddress 0x004574c8
     */
    void HideTrackGhost(int nTrack);

    /**
     * Report the "gem_ghost<n>.mat" material of one track.
     *
     * @param nTrack The track.
     * @return The material.
     * @ghidraAddress 0x00457570
     */
    Rnd::Mat *GetGhostMat(int nTrack);

    /**
     * Start one flash of "gem_flash.ps" at a point.
     *
     * The first gem flash record with no particle takes a new particle, which becomes white at full
     * alpha and size 1 at the point. Nothing happens when every record has a particle.
     * TnlGem::Flash() and TnlGemManager::Update() are the callers. The title is inferred.
     *
     * @param pos The point.
     * @ghidraAddress 0x00457588
     */
    void StartGemFlash(const Vector3 &pos);

    /**
     * Start the first idle panel effect on one cell.
     *
     * TnlPanel::Update() and TnlPanelFXDelay::Fire() are the callers. The title is inferred.
     *
     * @param nRing The ring of the cell.
     * @param nSlice The slice of the cell.
     * @param nForward Non-zero to run the effect forward.
     * @return 1 when an effect started, 0 when every effect was busy.
     * @ghidraAddress 0x00457648
     */
    int StartPanelFX(int nRing, int nSlice, int nForward);

private:
    // The length of mTrackModes, and the track count the constructor stores.
    static constexpr int kTrackCount = 8;

    // Gem flash record, the element of mGemFlashes. The name is inferred.
    struct GemFlash {
        // Release the flash particle. The destructor inlines the body. The deleting copy
        // at 0x00456cd0 has no callers.
        ~GemFlash();

        // Take a white particle of size 1 at pos when the record is free, reporting 1, else
        // report 0. StartGemFlash() inlines it. 0x00456d28.
        int Start(const Vector3 &pos);

        // Shrink the flash by 0.1, and release it once its size is not positive. SetFrame()
        // inlines it. 0x00456db0.
        void Update();

        Rnd::ParticleSys *mSystem; // "gem_flash.ps", shared by every record.
        Rnd::Particle *mParticle;  // The flash, or null while the record is free.
    };

    // Offer a fire to each TnlFireFX in turn until one starts it. HandleMessage() inlines this.
    // 0x004576a0
    void StartFireFX(float flPathStart,
                     int nIndex,
                     int nSlot,
                     const Color &color,
                     const Color &altColor,
                     float flPathEnd);

    // Launch the first idle TnlCrippleFX at the targets, through TnlCrippleFX::Start() inlined.
    // Returns 1 when one launched. HandleMessage() inlines this. 0x00457758.
    int StartCrippleFX(const std::vector<TnlPlayer *> &targets, float flFrame);

    // Start the first idle TnlBumpFX. Returns 1 when one started. HandleMessage() inlines this.
    // 0x00457828
    int StartBumpFX(int nStep, const HxStr &colorName, int nForward, float flPathOffset);

    // Start the first idle TnlSnake, through TnlSnake::Start() inlined. Returns 1 when one
    // started. HandleMessage() inlines this. 0x00457880.
    int StartSnake(float flFrame, int nRing, const Color &color, float flPhase, float flAmplitude);

    // Append a trigger to mPendingTriggers. HandleMessage() inlines this. 0x004579e0.
    void AddPendingTrigger(TnlTrigger *pTrigger, float flFrame);

    // Move the first unplaced particle of "string flare.ps" onto one ring at the renderer's song
    // tick, pushed outwards by 0.97. Nothing calls the out-of-line copy. 0x00457ae0.
    void PlaceStringFlareOnRing(int nRing, float flBlend);

    // Append a panel to mPanels and set the frame it starts from. OnBarChanged() is the caller.
    // 0x00447268
    void AddPanel(TnlPanel *pPanel, float flStartFrame);

    // GemMsg: queue a gem of the kind the track, the powerup, the ghost flag, and the jukebox
    // select. 0x00447638.
    void OnGem(GemMsg *pMsg);

    // CatchMsg: mark the catcher target, and flash and pulse on a hit or queue a miss gem.
    // 0x00447938
    void OnCatch(CatchMsg *pMsg);

    // PhraseMuffedMsg: redraw the bar's panel when the player tried the phrase. 0x00447ba8.
    void OnPhraseMuffed(PhraseMuffedMsg *pMsg);

    // PitchMsg: flash at the pitched gem and mark the catcher target. 0x00447cc0.
    void OnPitch(PitchMsg *pMsg);

    // SeekerMsg: move or clear the player's seeker range and sabre trail. 0x00447eb0.
    void OnSeeker(SeekerMsg *pMsg);

    // ShowEraseEffectMsg: schedule a panel effect for every erased bar still ahead. 0x004481d0.
    void OnShowEraseEffect(ShowEraseEffectMsg *pMsg);

    // SectionCapturedMsg: run a fire along the captured track. 0x00448530.
    void OnSectionCaptured(SectionCapturedMsg *pMsg);

    // CripplePacket: launch a crippler at the target players. 0x004486f8.
    void OnCripple(CripplePacket *pPacket);

    // FreestyleFXMsg: two snakes and a full-screen fire along the track. 0x00448a08.
    void OnFreestyleFX(FreestyleFXMsg *pMsg);

    // DeployedPowerupMsg: the effect of a neutralizer, autocatcher, bumper, or multiplier.
    // 0x00448d58
    void OnDeployedPowerup(DeployedPowerupMsg *pMsg);

    // NowBarMsg: ease the player's pointer toward a lane. HandleMessage() inlines this.
    // 0x00457cf0
    void OnNowBar(NowBarMsg *pMsg);

    // ClearGemMsg: remove one gem. HandleMessage() inlines this. 0x00457d88.
    void OnClearGem(ClearGemMsg *pMsg);

    // ClearGemsMsg: remove one bar's gems and end its trail. HandleMessage() inlines this.
    // 0x00457dd8
    void OnClearGems(ClearGemsMsg *pMsg);

    // SusGemMsg: start or stop a sustain strip. HandleMessage() inlines this. 0x00457e48.
    void OnSusGem(SusGemMsg *pMsg);

    // DurGemMsg: add a duration gem segment. HandleMessage() inlines this. 0x00457f38.
    void OnDurGem(DurGemMsg *pMsg);

    // TrackSelectMsg: turn the player's seeker, activator, grid markers, and now-ring slot to the
    // selected track. 0x00447328.
    void OnTrackSelect(TrackSelectMsg *pMsg);

    // AdvanceSectionToggleMsg: rewrite the boundary text, move mUnknown140 to the next step,
    // rebuild every sabre trail, and replay OnBarChanged() over the window. The message is not
    // read. 0x00448048.
    void OnAdvanceSectionToggle(AdvanceSectionToggleMsg *pMsg);

    // PlaybackToggleMsg: zoom the camera rig, record the jukebox flag, reassign the gem kinds, and
    // suppress or restore every activator and the now ring. 0x00448330.
    void OnPlaybackToggle(PlaybackToggleMsg *pMsg);

    // WinMsg: draw the arms in each winner's view and start them, and in kGameModeSolo stop the
    // first winner's blink and start the lattice. 0x00449240.
    void OnWin(WinMsg *pMsg);

    // MultiplierStateMsg: switch the player's catcher to the multiplier texture while a bonus
    // applies. 0x00449450.
    void OnMultiplierState(MultiplierStateMsg *pMsg);

    // PowerupFailedMsg: after a failed freestyler, show the player's arrow over every axe,
    // scratch, and vocal track for 2000 scaled frames. 0x00449500.
    void OnPowerupFailed(PowerupFailedMsg *pMsg);

    // AxeButtonMsg: spin or reset the player's pointer. HandleMessage() inlines this. 0x00457ff0.
    void OnAxeButton(AxeButtonMsg *pMsg);

    // PlayersTrackNeutralizedMsg: rumble the player's controller. HandleMessage() inlines this.
    // 0x004580e8
    void OnPlayersTrackNeutralized(PlayersTrackNeutralizedMsg *pMsg);

    // ToggleGhostMsg: show or hide the player's track ghost. HandleMessage() inlines this.
    // 0x00458120
    void OnToggleGhost(ToggleGhostMsg *pMsg);

    // JuiceAmountMsg: in a solo game, blink the player's activator while the juice is low.
    // HandleMessage() inlines this. 0x004581b8.
    void OnJuiceAmount(JuiceAmountMsg *pMsg);

    // Find the TnlPlayer of a game player, or null. The search is inlined wherever a handler
    // needs it, and the image has no out-of-line copy.
    TnlPlayer *FindTnlPlayer(Player *pPlayer);

    // Move each ghost material's alpha by its fade rate. A ghost that fades out completely hides
    // its gem kind, and either end of the range stops the fade. SetFrame() is the caller.
    // 0x00446460
    void UpdateGhostFades();

    Renderer *mRenderer; // +0x04
    // Globals::GetGameMode() at construction.
    int mGameMode; // +0x08
    // Globals::GetPlayMode() at construction.
    int mPlayMode; // +0x0c
    TnlBoundary *mBoundary;
    TnlNowRing *mNowRing;
    TnlArms *mArms;
    TnlMultFX *mMultFX;
    TnlLattice *mLattice;
    // Two "fire%d", two "firefs%d", and four "firemult%d" fires, in that order.
    std::vector<TnlFireFX *> mFireFX;
    std::vector<TnlCrippleFX *> mCrippleFX;
    std::vector<TnlPanelFX *> mPanelFX;
    std::vector<TnlBumpFX *> mBumpFX;
    std::vector<TnlSnake *> mSnakes;
    std::vector<TnlPendingTrigger> mPendingTriggers;
    std::vector<GemFlash *> mGemFlashes;
    std::vector<TnlArrow *> mArrows;
    TnlGemManager *mGemManager;
    DurGemTrails *mGemTrails;

public:
    // Public because the seeker script command reads the first player with no accessor in the
    // image.
    std::vector<TnlPlayer *> mPlayers;

private:
    Rnd::ParticleSys *mStringFlare; // "string flare.ps".
    Rnd::View *mStringView;         // "tnl strings".
    Rnd::Mat *mStringGemMat;        // "string gem mat".
    Rnd::Mat *mVoxStringMat;        // "voxstring.mat".
    // Panels SetFrame() advances, each deleted once TnlPanel::Update() reports it finished.
    std::list<TnlPanel *> mPanels;
    // Per-track rate the ghost material's alpha moves at, 0.15 while the ghost shows.
    std::vector<float> mGhostFadeRates;
    // One "saved tnl cam%d" per local player, a copy of "tnl cam%d" at construction.
    std::vector<Rnd::Cam *> mSavedCams;
    TnlCameraRig *mCameraRig;
    int mJukebox;     // +0xc8 Set in a jam in jukebox mode, which skips the camera intro.
    int mUnknowncc;   // +0xcc Set in kPlayModeGame.
    float mUnknownd0; // +0xd0 Globals::GetTempo() divided by 480000.
    // One "gem_ghost%d.mat" per track.
    std::vector<Rnd::Mat *> mGhostMats;
    // Per track, the "gem_scratch" effect kind for a scratch track, else the kind of the track's
    // instrument.
    std::vector<char> mTrackEffectKinds;
    // The powerup gem kinds, indexed by powerup: "gem_neut", "gem_crip", "gem_free", "gem_auto",
    // "gem_bump", and "gem_mult" at 12.
    std::vector<char> mPowerupGemKinds;
    // The "gem_hex_b", "gem_hex_g", "gem_hex_r", "gem_hex_y", and "gem_hex_p" kinds.
    std::vector<char> mHexGemKinds;
    // One "gem_ghost%d" kind per track, hidden at construction.
    std::vector<char> mGhostGemKinds;
    char mMissGemKind;  // "gem_miss".
    char mCrateGemKind; // "gem_crate".

public:
    // Non-zero draws every hex gem as "gem_crate". The hx.crates script command at 0x00449dc0
    // toggles it. Public because that command writes it with no accessor in the image.
    int mShowCrates;

private:
    // The mode of each track, from LevelData::TrackAt().
    TrackMode mTrackModes[kTrackCount];
    PlayMap *mPlayMap;
    int mTrackCount;                  // kTrackCount at construction.
    int mUnknown140;                  // +0x140
    float mUnknown144;                // +0x144 Globals::GetTempo() divided by 480000.
    int mUnknown148;                  // +0x148
    unsigned char mReserved14c[0x14]; // +0x14c
    Rnd::Light *mLatLight;            // "lat light1".
    unsigned char mReserved164[0x0c]; // +0x164
};

/**
 * The tunnel that exists, or null.
 *
 * The constructor stores the object and the destructor clears the word.
 *
 * @ghidraAddress 0x006e42a0
 */
extern AppTunnel *g_pAppTunnel;
