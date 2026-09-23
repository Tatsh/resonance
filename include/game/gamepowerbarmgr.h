#pragma once

#include <vector>

#include "game/powerbarmgr.h"

class PhraseDatabase;
class PlayMap;
class TrackData;

/**
 * Powerbar source that deals a powerbar to scattered bars of a track once, at construction.
 *
 * `15GamePowerbarMgr` in the RTTI, with PowerbarMgr as its one base. Its table is at `0x007e3878`.
 * The object is 0x40 bytes, which the allocations in PhraseMgr's routine at `0x001ba3d8` measure
 * for both subclasses. SoloPowerbarMgr and MultiPowerbarMgr differ only in the constant arguments
 * their constructors pass here.
 *
 * The deleting destructor at `0x001c6090` is implicitly declared.
 */
class GamePowerbarMgr : public PowerbarMgr {
public:
    /** One bar's entry in mBars. */
    struct Bar {
        int mUnknown00; /*!< Zero in every entry the constructor creates. +0x00 */
        int mPowerbar;  /*!< The powerbar, or -1 for none. +0x04 */
    };

    /**
     * Size mBars to the last step of the play map and deal the powerbars.
     *
     * The body is not written, because it draws from the R250 generator at `0x0052cfd0` through
     * the two helpers at `0x0052d098` (an integer in a half-open range) and `0x0052d0e0` (a float
     * in [0, 1)), which the tree does not declare yet. Starting at a bar drawn from [0, 10), it
     * visits bars at gaps drawn from [nMinGap, nMaxGap). A visited bar that has gems and whose
     * next bar is not a step start gets a powerbar. With bRandomKind set the powerbar is 3 or 12
     * with even odds. Otherwise the bar's position through the track selects a row of the
     * weighted table at `0x0068c790`, and a second draw selects the powerbar within that row.
     *
     * @param pMap The play map.
     * @param pDatabase The phrase database of the track.
     * @param pTrackData The track description.
     * @param bRandomKind Non-zero to choose between the two powerbars with even odds.
     * @param nTrack The track's index.
     * @param nUnknown30 Stored in mUnknown30. SoloPowerbarMgr passes 0 and MultiPowerbarMgr 1.
     * @param nMinGap The shortest gap between two dealt bars.
     * @param nMaxGap The gap the draw stops below.
     * @ghidraAddress 0x001c4fb0
     */
    GamePowerbarMgr(PlayMap *pMap,
                    PhraseDatabase *pDatabase,
                    const TrackData *pTrackData,
                    int bRandomKind,
                    int nTrack,
                    int nUnknown30,
                    int nMinGap,
                    int nMaxGap);

    /**
     * @param nBar The bar.
     * @return The powerbar mBars records for the bar, or -1.
     * @ghidraAddress 0x001c6258
     */
    virtual int GetPowerbar(int nBar);

private:
    const TrackData *mTrackData; // +0x14
    PlayMap *mMap;               // +0x18
    PhraseDatabase *mDatabase;   // +0x1c
    int mTrack;                  // +0x20
    std::vector<Bar> mBars;      // +0x24
    int mUnknown30;              // +0x30
    int mMinGap;                 // +0x34
    int mMaxGap;                 // +0x38
    int mRandomKind;             // +0x3c
};
