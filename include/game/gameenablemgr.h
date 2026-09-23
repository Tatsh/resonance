#pragma once

#include <vector>

#include "game/enablemgr.h"

class Gamer;
class PlayMap;
class Player;

/**
 * Enable policy of a game, which ties each track to the tracks that must be owned first.
 *
 * `13GameEnableMgr` in the RTTI descriptor at `0x008efc00`, deriving publicly from EnableMgr at
 * offset 0. Its type function is at `0x00105208` and its table at `0x007ccbc0`. The object is 0x30
 * bytes, and the destructor at `0x00104ca0` is implicitly declared.
 *
 * A track is enabled for a bar when every track in its requirement list has an owner at that bar,
 * and never when the list holds -1. The requirement lists come from a configuration code, one
 * one-based track list per track. When a track's state changes, the policy either releases the
 * track's requirements for good or invalidates the bar for the track's receivers.
 */
class GameEnableMgr : public EnableMgr {
public:
    /**
     * Build the policy with requirement lists read from a configuration code.
     *
     * @param nConfigCode The configuration code of the requirement lists.
     * @param nTrackCount The level's track count.
     * @param pGamer The participant whose tracks the policy governs.
     * @param nReleaseWhenMet Non-zero to release a track's requirements once its state changes.
     * @ghidraAddress 0x001011c8
     */
    GameEnableMgr(int nConfigCode, int nTrackCount, Gamer *pGamer, int nReleaseWhenMet);

    /**
     * Build the policy with empty requirement lists, which enable every track.
     *
     * @param nTrackCount The level's track count.
     * @param pGamer The participant whose tracks the policy governs.
     * @param nReleaseWhenMet Non-zero to release a track's requirements once its state changes.
     * @ghidraAddress 0x00101588
     */
    GameEnableMgr(int nTrackCount, Gamer *pGamer, int nReleaseWhenMet);

    /**
     * Build the solo game policy, which releases a track's requirements once met.
     *
     * Gamer::CreateEnableMgr() calls it with configuration code 0x387, and the routine at
     * `0x001167e0` with code 0x386. The title is inferred.
     *
     * @param nConfigCode The configuration code of the requirement lists.
     * @param nTrackCount The level's track count.
     * @param pGamer The participant.
     * @return The new policy.
     * @ghidraAddress 0x00105280
     */
    static GameEnableMgr *CreateReleasing(int nConfigCode, int nTrackCount, Gamer *pGamer);

    /**
     * Build a policy from a configuration code that invalidates bars instead of releasing.
     *
     * The image has no caller. The title is inferred.
     *
     * @param nConfigCode The configuration code of the requirement lists.
     * @param nTrackCount The level's track count.
     * @param pGamer The participant.
     * @return The new policy.
     * @ghidraAddress 0x00105308
     */
    static GameEnableMgr *CreateInvalidating(int nConfigCode, int nTrackCount, Gamer *pGamer);

    /**
     * Build the policy of a game with several players, which enables every track.
     *
     * The title is inferred.
     *
     * @param nTrackCount The level's track count.
     * @param pGamer The participant.
     * @return The new policy.
     * @ghidraAddress 0x00105390
     */
    static GameEnableMgr *CreateUnrestricted(int nTrackCount, Gamer *pGamer);

    /**
     * Record a new owner and act on every track whose state it changes.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param pPlayer The new owner, or the null player.
     * @ghidraAddress 0x00101d70
     */
    virtual void SetBarOwner(int nTrack, int nBar, Player *pPlayer);

    /**
     * Leave one track free of its requirements until a bar, or release them for good.
     *
     * A bar before the current one releases the requirements. Either way the track's receivers
     * are told the whole level is invalid.
     *
     * @param nTrack The track.
     * @param nBar The current bar.
     * @param nUntilBar The first bar the requirements apply to again.
     * @ghidraAddress 0x00101c68
     */
    virtual void SetFreeUntil(int nTrack, int nBar, int nUntilBar);

    /**
     * Stop one track from ever being enabled.
     *
     * Replaces the track's requirement list with the single entry -1.
     *
     * @param nTrack The track.
     * @ghidraAddress 0x00101bb0
     */
    virtual void DisableTrack(int nTrack);

    /**
     * Report whether one bar of one track may be caught.
     *
     * A bar before the track's free-until bar may always be caught. Otherwise the bar is mapped
     * through PlayMap::Slot5() and the requirements are tested against the owners there.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @return Non-zero when the bar may be caught.
     * @ghidraAddress 0x00105408
     */
    virtual int QueryBar(int nTrack, int nBar);

private:
    // Tracks the owner table covers.
    enum { kOwnedTrackCount = 8 };
    // The requirement entry that disables a track.
    enum { kNeverEnabled = -1 };

    // 0x00101f10
    // Clears the requirement lists, sizes them to mTrackCount, and fills each from
    // nConfigCode with the one-based track number as the lookup argument, storing each value less
    // one. The title is inferred.
    void Init(int nConfigCode);

    // 0x00105498
    // Sets pOwned[i] for each of the kOwnedTrackCount tracks to whether the track has
    // an owner at nBar.
    void FindOwnedTracks(int *pOwned, int nBar);

    // 0x00105530
    // Reports whether every requirement of nTrack is owned in pOwned.
    int IsTrackEnabled(int nTrack, const int *pOwned);

    int mTrackCount;
    int mOwnedTrackCount;
    std::vector<std::vector<int> > mRequirements;
    // Per track, the first bar the requirements apply to. Starts at -1.
    std::vector<int> mFreeUntil;
    Gamer *mGamer;
    PlayMap *mPlayMap;
    int mReleaseWhenMet;
};
