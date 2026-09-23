#pragma once

#include <vector>

#include "game/enablemgr.h"

class Gamer;
class PlayMap;
class Player;

/**
 * Enable policy of a network jam, which limits how many tracks one player may own per section.
 *
 * `15NetJamEnableMgr` in the RTTI descriptor at `0x00901ff0`, deriving publicly from EnableMgr at
 * offset 0. Its type function is at `0x001058e0` and its table at `0x007ccb28`. The object is
 * 0x34 bytes. It inherits SetFreeUntil() and DisableTrack(), and the destructor at `0x00104e70` is
 * implicitly declared.
 *
 * The policy records the owner of every track in every play-map section. The local player may
 * take a free track while it owns fewer than mMaxOwned tracks in the section, and always keeps the
 * tracks it owns. While mOpenTracks lists fewer tracks than the level has, only the listed tracks
 * may be caught.
 */
class NetJamEnableMgr : public EnableMgr {
public:
    /**
     * Build the policy with every track free in every section.
     *
     * @param nTrackCount The level's track count.
     * @param nMaxOwned The tracks one player may own per section.
     * @param pOpenTracks The tracks that may be caught while the list is incomplete.
     * @param pGamer The participant.
     * @ghidraAddress 0x00102790
     */
    NetJamEnableMgr(int nTrackCount,
                    int nMaxOwned,
                    const std::vector<int> *pOpenTracks,
                    Gamer *pGamer);

    /**
     * Build the policy for a network jam.
     *
     * Gamer::CreateEnableMgr() is the caller. The title is inferred.
     *
     * @param nTrackCount The level's track count.
     * @param nMaxOwned The tracks one player may own per section.
     * @param pOpenTracks The tracks that may be caught while the list is incomplete.
     * @param pGamer The participant.
     * @return The new policy.
     * @ghidraAddress 0x00105958
     */
    static NetJamEnableMgr *
    Create(int nTrackCount, int nMaxOwned, const std::vector<int> *pOpenTracks, Gamer *pGamer);

    /**
     * Record the owner of a bar's section and invalidate every track whose state it changes.
     *
     * The owner is read from the track's phrase database rather than from pPlayer. Does nothing
     * while mOpenTracks is incomplete.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param pPlayer The owner, which the body does not read.
     * @ghidraAddress 0x00103158
     */
    virtual void SetBarOwner(int nTrack, int nBar, Player *pPlayer);

    /**
     * Report whether one bar of one track may be caught.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @return Non-zero when the bar may be caught.
     * @ghidraAddress 0x001059f0
     */
    virtual int QueryBar(int nTrack, int nBar);

private:
    // The owner identifier of a track nobody owns.
    enum { kNoOwner = -2 };

    // 0x00105a98
    // Reports whether the local player may catch nTrack in nSection.
    int IsTrackAvailable(int nTrack, int nSection);

    // 0x00105b08
    // Reports the play-map section a bar lies in.
    int FindSection(int nBar);

    PlayMap *mPlayMap;
    Gamer *mGamer;
    // Player::mId20 of the first local player.
    int mLocalId;
    int mTrackCount;
    int mMaxOwned;
    std::vector<int> mOpenTracks;
    // PlayMap::mSteps, the first bar of each section.
    const std::vector<int> *mSteps;
    // Per section, the owner identifier of each track.
    std::vector<std::vector<int> > mOwners;
};
