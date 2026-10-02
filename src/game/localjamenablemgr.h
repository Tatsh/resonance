#pragma once

#include "game/enablemgr.h"

class Player;
class TrackData;

/**
 * Enable policy of a jam session on one machine, which bars only silent vocal bars.
 *
 * `17LocalJamEnableMgr` in the RTTI descriptor at `0x008ef2f0`, deriving publicly from EnableMgr at
 * offset 0. Its type function is at `0x001057b8` and its table at `0x007ccb60`. The object is 0x24
 * bytes. It inherits SetFreeUntil() and DisableTrack() from the base, and the destructor at
 * `0x00105788` is implicitly declared.
 */
class LocalJamEnableMgr : public EnableMgr {
public:
    /** The number of tracks the object records. */
    enum { kTrackCount = 8 };

    /**
     * Record the level's first eight tracks.
     *
     * The out-of-line copy has no caller, and the two factories expand the body.
     *
     * @ghidraAddress 0x00105838
     */
    LocalJamEnableMgr();

    /**
     * Build the policy for a jam with one player.
     *
     * Gamer's constructor for the enable policy calls it in kGameModeSolo outside kPlayModeGame.
     * The body is identical to CreateLocal(). The title is inferred.
     *
     * @return The new policy.
     * @ghidraAddress 0x001025f0
     */
    static LocalJamEnableMgr *CreateSolo();

    /**
     * Build the policy for a jam with several players on one machine.
     *
     * Gamer's constructor for the enable policy calls it in kGameModeLocal outside
     * kPlayModeGame. The title is inferred.
     *
     * @return The new policy.
     * @ghidraAddress 0x001026c0
     */
    static LocalJamEnableMgr *CreateLocal();

    /**
     * Ignore an owner change.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param pPlayer The owner.
     * @ghidraAddress 0x00105830
     */
    virtual void SetBarOwner(int nTrack, int nBar, Player *pPlayer);

    /**
     * Report whether one bar may be caught.
     *
     * Every bar of a track outside kTrackModeVocal may be caught. A vocal bar may be caught only
     * when a NoteFinder finds a note sounding into it, searching the previous bar's MIDI from tick
     * 1920 and then the bar's own MIDI from tick 0.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @return Non-zero when the bar may be caught.
     * @ghidraAddress 0x00102488
     */
    virtual int QueryBar(int nTrack, int nBar);

private:
    TrackData *mTracks[kTrackCount];
};
