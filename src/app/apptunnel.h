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
 * Its RTTI descriptor is at `0x008f0860`. It has MsgSink as its one public base at offset 0. Its
 * type function is at `0x00453ff8`. The class sits in `app/` beside Renderer, its one constructor
 * caller, and it is a game-side class rather than a Rnd one.
 *
 * The table at `0x0081ba38` has four entries, the same length as MsgSink's table at `0x007ccc40`,
 * and the class therefore introduces no virtual. It overrides the destructor at slot 1 and
 * DispatchPriv() at slot 3, and inherits MsgSink::Dispatch() at slot 2.
 *
 * The object is 0x170 bytes, the size Renderer's constructor requests under the MsgSink tag at
 * `0x0042c89c`. No routine of the unit reads or writes the words from `+0x14c` to `+0x15c` or from
 * `+0x164` to the end.
 *
 * The unit ends with the out-of-line copies of the inline routines of the tunnel helper classes,
 * and of the members below that DispatchPriv() and the helpers inline. Those copies have no
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
     * @ghidraAddress NTSC-U/C: 0x00442020
     * @ghidraAddress PAL: 0x0047ee28
     */
    AppTunnel(Renderer *pRenderer);

    /**
     * Release every tunnel object and give each main camera back its saved state.
     *
     * Clears g_pAppTunnel first. The pending triggers are not deleted.
     *
     * @ghidraAddress NTSC-U/C: 0x00445740
     * @ghidraAddress PAL: 0x00482950
     */
    virtual ~AppTunnel();

    /**
     * Act on one message the renderer sends on.
     *
     * The message type selects one handler. DisplayPointerMsg, ChoosePowerupMsg, StdMidiMsg, and
     * every other type are ignored, and nothing is passed on to MsgSink.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x00449688
     * @ghidraAddress PAL: 0x00486938
     */
    virtual void DispatchPriv(Message *pMsg);

    /**
     * Advance the tunnel to one song position.
     *
     * Fades the string flares, shrinks the gem flashes, retires finished panels and fired
     * triggers, and advances every helper and effect. The fire effects' views, the arrows, and the
     * players also receive the position scaled by the tempo rate. Renderer::Update() is the
     * caller. The title is inferred from that caller, which hands the same value to
     * Rnd::Animatable::SetFrame() on its three views.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress NTSC-U/C: 0x00446960
     * @ghidraAddress PAL: 0x00483b90
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
     * @ghidraAddress NTSC-U/C: 0x00446838
     * @ghidraAddress PAL: 0x00483a68
     */
    void OnLeaderChanged(Player *pOldLeader, Player *pNewLeader);

    /**
     * Update the tunnel section of one bar whose state a BarStatusMsg changed.
     *
     * Nothing happens once the renderer's song tick is more than a quarter bar past the end of
     * the bar. In kPlayModeGame, with nRefreshing zero and a song tick that is not negative, a new
     * TnlPanel starts a quarter of the way from the song tick to the start of the bar. Otherwise
     * a TnlPanel is built on the stack and applied at once. A jukebox game always passes 0 as the
     * panel's showing flag. The title is inferred.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param nRefreshing Non-zero when PhraseMgr posted the BarStatusMsg while refreshing bars.
     * @param pPlayer The cell's player.
     * @param nPowerup The cell's `mPowerup`.
     * @param nEnabled The cell's `mEnabled`.
     * @ghidraAddress NTSC-U/C: 0x004465a0
     * @ghidraAddress PAL: 0x004837d0
     */
    void OnBarChanged(
        int nTrack, int nBar, int nRefreshing, Player *pPlayer, int nPowerup, int nEnabled);

    /**
     * Turn the now ring for one local view before the renderer draws it.
     *
     * The body is TnlNowRing::SetRotation() with the view index as the step, inlined.
     * Renderer::Draw() is the caller. The title is inferred.
     *
     * @param nView The index of the local view.
     * @param flFrame The song position, in MIDI ticks. The body does not read it.
     * @ghidraAddress NTSC-U/C: 0x00457bd0
     * @ghidraAddress PAL: 0x00495100
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
     * @ghidraAddress NTSC-U/C: 0x00457a98
     * @ghidraAddress PAL: 0x00494fc8
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
     * @ghidraAddress NTSC-U/C: 0x00457338
     * @ghidraAddress PAL: 0x00494868
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
     * @ghidraAddress NTSC-U/C: 0x00457418
     * @ghidraAddress PAL: 0x00494948
     */
    void ShowTrackGhost(int nTrack, Rnd::Drawable *pGhost);

    /**
     * Make the ghost material of one track opaque again and fade the ghost out at -0.15.
     *
     * TnlActivator is the caller. The title is inferred.
     *
     * @param nTrack The track.
     * @ghidraAddress NTSC-U/C: 0x004574c8
     * @ghidraAddress PAL: 0x004949f8
     */
    void HideTrackGhost(int nTrack);

    /**
     * Report the "gem_ghost<n>.mat" material of one track.
     *
     * @param nTrack The track.
     * @return The material.
     * @ghidraAddress NTSC-U/C: 0x00457570
     * @ghidraAddress PAL: 0x00494aa0
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
     * @ghidraAddress NTSC-U/C: 0x00457588
     * @ghidraAddress PAL: 0x00494ab8
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
     * @ghidraAddress NTSC-U/C: 0x00457648
     * @ghidraAddress PAL: 0x00494b78
     */
    int StartPanelFX(int nRing, int nSlice, int nForward);

private:
    // The length of mTrackModes, and the track count the constructor stores.
    static constexpr int kTrackCount = 8;

    // Gem flash record, the element of mGemFlashes. The name is inferred.
    class GemFlash {
    public:
        /**
         * Release the flash particle.
         *
         * The destructor inlines the body. The deleting copy has no callers.
         *
         * @ghidraAddress NTSC-U/C: 0x00456cd0
         * @ghidraAddress PAL: 0x00494200
         */
        ~GemFlash();

        /**
         * Take a white particle of size 1 at pos when the record is free, reporting 1, else report
         * 0.
         *
         * StartGemFlash() inlines it.
         *
         * @ghidraAddress NTSC-U/C: 0x00456d28
         * @ghidraAddress PAL: 0x00494258
         */
        int Start(const Vector3 &pos);

        /**
         * Shrink the flash by 0.1, and release it once its size is not positive.
         *
         * SetFrame() inlines it.
         *
         * @ghidraAddress NTSC-U/C: 0x00456db0
         * @ghidraAddress PAL: 0x004942e0
         */
        void Update();

        Rnd::ParticleSys *mSystem; // "gem_flash.ps", shared by every record.
        Rnd::Particle *mParticle;  // The flash, or null while the record is free.
    };

    /**
     * Offer a fire to each TnlFireFX in turn until one starts it.
     *
     * DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x004576a0
     * @ghidraAddress PAL: 0x00494bd0
     */
    void StartFireFX(float flPathStart,
                     int nIndex,
                     int nSlot,
                     const Color &color,
                     const Color &altColor,
                     float flPathEnd);

    /**
     * Launch the first idle TnlCrippleFX at the targets, through TnlCrippleFX::Start() inlined.
     *
     * Returns 1 when one launched. DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x00457758
     * @ghidraAddress PAL: 0x00494c88
     */
    int StartCrippleFX(const std::vector<TnlPlayer *> &targets, float flFrame);

    /**
     * Start the first idle TnlBumpFX.
     *
     * Returns 1 when one started. DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x00457828
     * @ghidraAddress PAL: 0x00494d58
     */
    int StartBumpFX(int nStep, const HxStr &colorName, int nForward, float flPathOffset);

    /**
     * Start the first idle TnlSnake, through TnlSnake::Start() inlined.
     *
     * Returns 1 when one started. DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x00457880
     * @ghidraAddress PAL: 0x00494db0
     */
    int StartSnake(float flFrame, int nRing, const Color &color, float flPhase, float flAmplitude);

    /**
     * Append a trigger to mPendingTriggers.
     *
     * DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x004579e0
     * @ghidraAddress PAL: 0x00494f10
     */
    void AddPendingTrigger(TnlTrigger *pTrigger, float flFrame);

    /**
     * Move the first unplaced particle of "string flare.ps" onto one ring at the renderer's song
     * tick, pushed outwards by 0.97.
     *
     * Nothing calls the out-of-line copy.
     *
     * @ghidraAddress NTSC-U/C: 0x00457ae0
     * @ghidraAddress PAL: 0x00495010
     */
    void PlaceStringFlareOnRing(int nRing, float flBlend);

    /**
     * Append a panel to mPanels and set the frame it starts from.
     *
     * OnBarChanged() is the caller.
     *
     * @ghidraAddress NTSC-U/C: 0x00447268
     * @ghidraAddress PAL: 0x00484498
     */
    void AddPanel(TnlPanel *pPanel, float flStartFrame);

    /**
     * On a GemMsg, queue a gem of the kind the track, the powerup, the ghost flag, and the jukebox
     * select.
     *
     * @ghidraAddress NTSC-U/C: 0x00447638
     * @ghidraAddress PAL: 0x00484868
     */
    void OnGem(GemMsg *pMsg);

    /**
     * On a CatchMsg, mark the catcher target, and flash and pulse on a hit or queue a miss gem.
     *
     * @ghidraAddress NTSC-U/C: 0x00447938
     * @ghidraAddress PAL: 0x00484b88
     */
    void OnCatch(CatchMsg *pMsg);

    /**
     * On a PhraseMuffedMsg, redraw the bar's panel when the player tried the phrase.
     *
     * @ghidraAddress NTSC-U/C: 0x00447ba8
     * @ghidraAddress PAL: 0x00484df8
     */
    void OnPhraseMuffed(PhraseMuffedMsg *pMsg);

    /**
     * On a PitchMsg, flash at the pitched gem and mark the catcher target.
     *
     * @ghidraAddress NTSC-U/C: 0x00447cc0
     * @ghidraAddress PAL: 0x00484f10
     */
    void OnPitch(PitchMsg *pMsg);

    /**
     * On a SeekerMsg, move or clear the player's seeker range and sabre trail.
     *
     * @ghidraAddress NTSC-U/C: 0x00447eb0
     * @ghidraAddress PAL: 0x00485100
     */
    void OnSeeker(SeekerMsg *pMsg);

    /**
     * On a ShowEraseEffectMsg, schedule a panel effect for every erased bar still ahead.
     *
     * @ghidraAddress NTSC-U/C: 0x004481d0
     * @ghidraAddress PAL: 0x00485420
     */
    void OnShowEraseEffect(ShowEraseEffectMsg *pMsg);

    /**
     * On a SectionCapturedMsg, run a fire along the captured track.
     *
     * @ghidraAddress NTSC-U/C: 0x00448530
     * @ghidraAddress PAL: 0x00485780
     */
    void OnSectionCaptured(SectionCapturedMsg *pMsg);

    /**
     * On a CripplePacket, launch a crippler at the target players.
     *
     * @ghidraAddress NTSC-U/C: 0x004486f8
     * @ghidraAddress PAL: 0x00485968
     */
    void OnCripple(CripplePacket *pPacket);

    /**
     * On a FreestyleFXMsg, start two snakes and a full-screen fire along the track.
     *
     * @ghidraAddress NTSC-U/C: 0x00448a08
     * @ghidraAddress PAL: 0x00485c78
     */
    void OnFreestyleFX(FreestyleFXMsg *pMsg);

    /**
     * On a DeployedPowerupMsg, show the effect of a neutralizer, autocatcher, bumper, or
     * multiplier.
     *
     * @ghidraAddress NTSC-U/C: 0x00448d58
     * @ghidraAddress PAL: 0x00485fc8
     */
    void OnDeployedPowerup(DeployedPowerupMsg *pMsg);

    /**
     * On a NowBarMsg, ease the player's pointer toward a lane.
     *
     * DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x00457cf0
     * @ghidraAddress PAL: 0x00495220
     */
    void OnNowBar(NowBarMsg *pMsg);

    /**
     * On a ClearGemMsg, remove one gem.
     *
     * DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x00457d88
     * @ghidraAddress PAL: 0x004952b8
     */
    void OnClearGem(ClearGemMsg *pMsg);

    /**
     * On a ClearGemsMsg, remove one bar's gems and end its trail.
     *
     * DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x00457dd8
     * @ghidraAddress PAL: 0x00495308
     */
    void OnClearGems(ClearGemsMsg *pMsg);

    /**
     * On a SusGemMsg, start or stop a sustain strip.
     *
     * DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x00457e48
     * @ghidraAddress PAL: 0x00495378
     */
    void OnSusGem(SusGemMsg *pMsg);

    /**
     * On a DurGemMsg, add a duration gem segment.
     *
     * DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x00457f38
     * @ghidraAddress PAL: 0x00495488
     */
    void OnDurGem(DurGemMsg *pMsg);

    /**
     * On a TrackSelectMsg, turn the player's seeker, activator, grid markers, and now-ring slot to
     * the selected track.
     *
     * @ghidraAddress NTSC-U/C: 0x00447328
     * @ghidraAddress PAL: 0x00484558
     */
    void OnTrackSelect(TrackSelectMsg *pMsg);

    /**
     * On AdvanceSectionToggleMsg, rewrite the boundary text, move mNextStepBar to the next step,
     * rebuild every sabre trail, and replay OnBarChanged() over the window.
     *
     * The message is not read.
     *
     * @ghidraAddress NTSC-U/C: 0x00448048
     * @ghidraAddress PAL: 0x00485298
     */
    void OnAdvanceSectionToggle(AdvanceSectionToggleMsg *pMsg);

    /**
     * On a PlaybackToggleMsg, zoom the camera rig, record the jukebox flag, reassign the gem kinds,
     * and suppress or restore every activator and the now ring.
     *
     * @ghidraAddress NTSC-U/C: 0x00448330
     * @ghidraAddress PAL: 0x00485580
     */
    void OnPlaybackToggle(PlaybackToggleMsg *pMsg);

    /**
     * On a WinMsg, draw the arms in each winner's view and start them, and in kGameModeSolo stop
     * the first winner's blink and start the lattice.
     *
     * @ghidraAddress NTSC-U/C: 0x00449240
     * @ghidraAddress PAL: 0x004864f0
     */
    void OnWin(WinMsg *pMsg);

    /**
     * On a MultiplierStateMsg, switch the player's catcher to the multiplier texture while a bonus
     * applies.
     *
     * @ghidraAddress NTSC-U/C: 0x00449450
     * @ghidraAddress PAL: 0x00486700
     */
    void OnMultiplierState(MultiplierStateMsg *pMsg);

    /**
     * On a PowerupFailedMsg, after a failed freestyler, show the player's arrow over every axe,
     * scratch, and vocal track for 2000 scaled frames.
     *
     * @ghidraAddress NTSC-U/C: 0x00449500
     * @ghidraAddress PAL: 0x004867b0
     */
    void OnPowerupFailed(PowerupFailedMsg *pMsg);

    /**
     * On an AxeButtonMsg, spin or reset the player's pointer.
     *
     * DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x00457ff0
     * @ghidraAddress PAL: 0x00495560
     */
    void OnAxeButton(AxeButtonMsg *pMsg);

    /**
     * On a PlayersTrackNeutralizedMsg, rumble the player's controller.
     *
     * DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x004580e8
     * @ghidraAddress PAL: 0x00495658
     */
    void OnPlayersTrackNeutralized(PlayersTrackNeutralizedMsg *pMsg);

    /**
     * On a ToggleGhostMsg, show or hide the player's track ghost.
     *
     * DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x00458120
     * @ghidraAddress PAL: 0x00495690
     */
    void OnToggleGhost(ToggleGhostMsg *pMsg);

    /**
     * On a JuiceAmountMsg, in a solo game, blink the player's activator while the juice is low.
     *
     * DispatchPriv() inlines this.
     *
     * @ghidraAddress NTSC-U/C: 0x004581b8
     * @ghidraAddress PAL: 0x00495728
     */
    void OnJuiceAmount(JuiceAmountMsg *pMsg);

    // Find the TnlPlayer of a game player, or null. The search is inlined wherever a handler
    // needs it, and the image has no out-of-line copy.
    TnlPlayer *FindTnlPlayer(Player *pPlayer);

    /**
     * Move each ghost material's alpha by its fade rate.
     *
     * A ghost that fades out completely hides its gem kind, and either end of the range stops the
     * fade. SetFrame() is the caller.
     *
     * @ghidraAddress NTSC-U/C: 0x00446460
     * @ghidraAddress PAL: 0x00483690
     */
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
    int mJukebox; // +0xc8 Set in a jam in jukebox mode. The camera intro is then skipped.
    // Set in kPlayModeGame. New panels and far gems then appear ahead of their bar rather than
    // at once.
    int mStaggerPanels;
    // Globals::GetTempo() divided by 480000, written by the constructor and never read.
    float mInitialTempoRate;
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
    int mNextStepBar;                 // The bar at which SetFrame() advances to the next step.
    float mTempoRate;                 // Globals::GetTempo() divided by 480000.
    int mUnusedWord;                  // +0x148 Zeroed by the constructor and never read.
    unsigned char mReserved14c[0x14]; // +0x14c
    Rnd::Light *mLatLight;            // "lat light1".
    unsigned char mReserved164[0x0c]; // +0x164
};

/**
 * The tunnel that exists, or null.
 *
 * The constructor stores the object and the destructor clears the word.
 *
 * @ghidraAddress NTSC-U/C: 0x006e42a0
 * @ghidraAddress PAL: 0x00727bc0
 */
extern AppTunnel *g_pAppTunnel;
