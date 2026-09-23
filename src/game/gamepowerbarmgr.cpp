#include "game/gamepowerbarmgr.h"

#include <vector>

#include "game/playmap.h"
#include "game/trackdata.h"
#include "os/r250.h"

namespace {

// The value of a bar that awards no powerbar.
constexpr int kNoPowerbar = -1;

// The first dealt bar is drawn from this range.
constexpr int kFirstBarLow = 0;
constexpr int kFirstBarHigh = 10;

// The two powerbars the random kind chooses between, and the draw that separates them.
constexpr int kLowDrawPowerbar = 3;
constexpr int kHighDrawPowerbar = 12;
constexpr double kEvenOdds = 0.5;

// One weighted choice. A draw selects the first entry whose threshold it does not exceed.
struct PowerbarWeight {
    int mPowerbar;
    float mThreshold;
};

// The weights for one stretch of the track. A bar selects the first row whose threshold its
// position through the track does not exceed.
struct PowerbarWeightRow {
    const PowerbarWeight *mWeights;
    float mThreshold;
};

// 0x0068c718
const PowerbarWeight g_aPowerbarWeightsEarly[] = {
    {3, 0.2f},
    {4, 0.6f},
    {1, 0.8f},
    {2, 1.01f},
    {0, 1.01f},
};

// 0x0068c740
const PowerbarWeight g_aPowerbarWeightsMiddle[] = {
    {3, 0.2f},
    {4, 0.4f},
    {1, 0.6f},
    {2, 0.8f},
    {0, 1.01f},
};

// 0x0068c768
const PowerbarWeight g_aPowerbarWeightsLate[] = {
    {3, 0.16f},
    {4, 0.33f},
    {1, 0.49f},
    {2, 0.67f},
    {0, 1.01f},
};

// 0x0068c790
const PowerbarWeightRow g_aPowerbarWeightRows[] = {
    {g_aPowerbarWeightsEarly, 0.25f},
    {g_aPowerbarWeightsMiddle, 0.6f},
    {g_aPowerbarWeightsLate, 1.01f},
};

} // namespace

// 0x001c4fb0
GamePowerbarMgr::GamePowerbarMgr(PlayMap *pMap,
                                 PhraseDatabase *pDatabase,
                                 const TrackData *pTrackData,
                                 int bRandomKind,
                                 int nTrack,
                                 int nUnknown30,
                                 int nMinGap,
                                 int nMaxGap)
    : mTrackData(pTrackData), mMap(pMap), mDatabase(pDatabase), mTrack(nTrack),
      mUnknown30(nUnknown30), mMinGap(nMinGap), mMaxGap(nMaxGap), mRandomKind(bRandomKind) {
    const Bar empty{0, kNoPowerbar};
    mBars.resize(mMap->mSteps.back(), empty);

    const int nBarCount = static_cast<int>(mBars.size());
    for (int nBar = RandomInt(kFirstBarLow, kFirstBarHigh); nBar < nBarCount;
         nBar += RandomInt(mMinGap, mMaxGap)) {
        if (mTrackData->GetGemsInBar(nBar)->empty()) {
            continue;
        }
        if (mMap->IsStepStart(nBar + 1)) {
            continue;
        }

        if (mRandomKind) {
            mBars[nBar].mPowerbar =
                (RandomFloat() < kEvenOdds) ? kLowDrawPowerbar : kHighDrawPowerbar;
            continue;
        }

        const float flPosition = static_cast<float>(nBar) / static_cast<float>(nBarCount);
        int nRow = 0;
        while (g_aPowerbarWeightRows[nRow].mThreshold < flPosition) {
            ++nRow;
        }
        const PowerbarWeight *pWeights = g_aPowerbarWeightRows[nRow].mWeights;
        const float flDraw = RandomFloat();
        int nWeight = 0;
        while (pWeights[nWeight].mThreshold < flDraw) {
            ++nWeight;
        }
        mBars[nBar].mPowerbar = pWeights[nWeight].mPowerbar;
    }
}

// 0x001c6258
int GamePowerbarMgr::GetPowerbar(int nBar) {
    return mBars[nBar].mPowerbar;
}
