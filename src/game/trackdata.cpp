#include "game/trackdata.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include "app/application.h"
#include "game/gamer.h"
#include "game/harmony.h"
#include "game/phrase.h"
#include "game/phrasedatabase.h"
#include "game/playmap.h"
#include "game/riff.h"
#include "game/riffset.h"
#include "game/tickobjvector.h"
#include "gs/multimuse.h"
#include "mid/mbt.h"
#include "msg/musemsg.h"
#include "script/configquery.h"

namespace {

// One bar at 480 ticks per quarter note.
constexpr int kBarLength = 1920;

// The bars FindGemAtOrAfter() searches ahead.
constexpr int kGemSearchBars = 4;

// The riff position before the first riff set exists.
constexpr int kNoRiffTick = -1;

// The quantisation a new bar starts with.
constexpr int kDefaultQuant = 120;

// The track identifier ScoreBars() skips.
constexpr int kSkippedTrack = -1;

// Modes 1 through 3 take a flat point value rather than one scored from the gems.
constexpr unsigned kFlatPointModeCount = 3;

// Configuration codes this file queries.
constexpr int kFlatPointsQuery = 0x395;
constexpr int kBarUnknown08Query = 0x38b;
constexpr int kScoreWeightsQuery = 0x3a7;
constexpr int kScoreThresholdsQuery = 0x3a8;

// The gem value GetGemAt() reports when no gem sits at the position.
constexpr int kNoGem = -1;

// ScoreGems() scans this many divisors.
constexpr int kScoreDivisorCount = 6;

// One scoring rule. A gem whose position is a multiple of mDivisor adds mWeight.
struct ScoreRule {
    int mDivisor;
    int mWeight;
};

// 0x0068fef8. The weights are overwritten from the configuration on first use.
ScoreRule g_aScoreRules[kScoreDivisorCount] = {
    {1920, 15},
    {960, 25},
    {480, 35},
    {240, 50},
    {120, 70},
    {1, 70},
};

// 0x0068ff28
std::vector<int> g_scoreThresholds;

// 0x0068ff34
int g_bScoreTablesLoaded;

// The clamp the inline Mid::MBT constructor applies to a computed position.
inline int ClampPosition(int nTick) {
    return std::min(std::max(nTick, kMBTMinimum), kMBTMaximum);
}

// 0x001d75a0
void DeleteMidi(TickObj<MuseMsg *> entry) {
    delete entry.mValue;
}

// 0x001d75d8
void DeleteRiffSet(RiffSet *pRiffSet) {
    delete pRiffSet;
}

// 0x001d75f8
void DeleteHarmony(Harmony *pHarmony) {
    delete pHarmony;
}

} // namespace

// 0x001d2ef0
TrackData::Bar::Bar() : mQuant(kDefaultQuant), mPoints(0) {
}

// 0x001d2f50
TrackData::Bar::~Bar() {
    std::for_each(mMidi.begin(), mMidi.end(), DeleteMidi);
}

// 0x001d31d8
void TrackData::Bar::Print(std::ostream &stream) {
    stream << "quant = " << mQuant << ". ";
    stream << "points = " << mPoints << ". ";

    stream << "harms: " << "(";
    for (auto &entry : mHarmonies) {
        if (entry.mValue == nullptr) {
            stream << "[invalid]";
        } else {
            stream << "[";
            entry.mPosition.Print(stream);
            stream << ": ";
            entry.mValue->Print(stream);
            stream << "]";
        }
        stream << " ";
    }
    stream << ")" << std::endl;

    stream << "riffs: " << "(";
    for (auto &entry : mRiffSets) {
        if (entry.mValue == nullptr) {
            stream << "[invalid]";
        } else {
            stream << "[";
            entry.mPosition.Print(stream);
            stream << ": ";
            entry.mValue->Print(stream);
            stream << "]";
        }
        stream << " ";
    }
    stream << ")" << std::endl;

    stream << "midi: " << "(";
    for (auto &entry : mMidi) {
        PrintMuseEntry(stream, entry.mPosition, entry.mValue);
        stream << " ";
    }
    stream << ")" << std::endl;

    stream << "gems: " << "(";
    for (auto &entry : mGems) {
        stream << "[";
        entry.mPosition.Print(stream);
        stream << ": " << entry.mValue << "]";
        stream << " ";
    }
    stream << ")" << std::endl;
}

// 0x001d35a8
TrackData::TrackData(int nIndex, PlayMap *pMap)
    : mUnknown04(nIndex), mKind(kTrackModeRiff), mMap(pMap), mBars(pMap->mSteps.back()) {
    (void)IsFiniteMBT(kBarLength); // Yes, the binary discards this call's result.
    mBarLength = kBarLength;
    mUnknown30 = kGemSearchBars;
    mCurrentRiffSet = nullptr;
    (void)IsFiniteMBT(kNoRiffTick); // Yes, the binary discards this call's result.
    mCurrentRiffTick = kNoRiffTick;
}

// 0x001d3900
TrackData::~TrackData() {
    std::for_each(mRiffSetsOwned.begin(), mRiffSetsOwned.end(), DeleteRiffSet);
    std::for_each(mHarmoniesOwned.begin(), mHarmoniesOwned.end(), DeleteHarmony);
}

// 0x001d3b18
void TrackData::AddRiff(int nTick, Riff *pRiff) {
    const int nLength = ClampPosition(mBarLength * static_cast<int>(mBars.size()));
    (void)IsFiniteMBT(nLength); // Yes, the binary discards this call's result.
    if (!(nTick < nLength)) {
        return;
    }

    if (mCurrentRiffTick != nTick) {
        Bar *pBar;
        Mid::MBT offset;
        Locate(nTick, pBar, offset);

        mCurrentRiffSet = new RiffSet;
        mCurrentRiffTick = nTick;
        mRiffSetsOwned.push_back(mCurrentRiffSet);
        SetAtTick(pBar->mRiffSets, mCurrentRiffSet, offset.mTick);

        for (auto it = mBars.begin() + (pBar - mBars.data()) + 1; it != mBars.end(); ++it) {
            (void)IsFiniteMBT(0); // Yes, the binary discards this call's result.
            SetAtTick(it->mRiffSets, mCurrentRiffSet, 0);
        }
    }
    mCurrentRiffSet->mRiffs[pRiff->mId] = pRiff;
}

// 0x001d3d10
void TrackData::AddHarmony(int nTick, const std::vector<char> &notes) {
    const int nLength = ClampPosition(mBarLength * static_cast<int>(mBars.size()));
    (void)IsFiniteMBT(nLength); // Yes, the binary discards this call's result.
    if (!(nTick < nLength)) {
        return;
    }

    Bar *pBar;
    Mid::MBT offset;
    Locate(nTick, pBar, offset);

    Harmony *pHarmony = new Harmony(notes);
    mHarmoniesOwned.push_back(pHarmony);
    SetAtTick(pBar->mHarmonies, pHarmony, offset.mTick);

    for (auto it = mBars.begin() + (pBar - mBars.data()) + 1; it != mBars.end(); ++it) {
        (void)IsFiniteMBT(0); // Yes, the binary discards this call's result.
        SetAtTick(it->mHarmonies, pHarmony, 0);
    }
}

// 0x001d3ee0
void TrackData::AddGem(int nTick, int nGem, Riff *pRiff) {
    const int nLength = ClampPosition(mBarLength * static_cast<int>(mBars.size()));
    (void)IsFiniteMBT(nLength); // Yes, the binary discards this call's result.
    if (!(nTick < nLength)) {
        return;
    }

    Bar *pBar;
    Mid::MBT offset;
    Locate(nTick, pBar, offset);
    InsertAtTick(pBar->mGems, nGem, offset.mTick);

    if (pRiff != nullptr) {
        RiffSet *pRiffSet = new RiffSet;
        mRiffSetsOwned.push_back(pRiffSet);
        pRiffSet->mRiffs[pRiff->mId] = pRiff;
        InsertAtTick(pBar->mRiffSets, pRiffSet, offset.mTick);
    }
}

// 0x001d4308
void TrackData::ScoreBars() {
    if (mUnknown04 == kSkippedTrack) {
        return;
    }

    const int nFlatPoints = QueryConfigValue(kFlatPointsQuery);
    (void)Application::shared()->GetGameMode(); // Yes, the binary discards this call's result.
    const int nUnknown08 = QueryConfigValue(kBarUnknown08Query);

    for (unsigned i = 0; i != mBars.size(); ++i) {
        int nPoints = nFlatPoints;
        if (!(static_cast<unsigned>(mKind - 1) < kFlatPointModeCount)) {
            nPoints = ScoreGems(mBars[i].mGems);
        }
        mBars[i].mPoints = nPoints;
        mBars[i].mUnknown08 = nUnknown08;
    }
}

// 0x001d4428
int TrackData::FindGemAtOrBefore(int nTick, int *pTick, int *pGem) const {
    const Bar *pBar;
    Mid::MBT offset;
    LocateMapped(nTick, pBar, offset);

    auto it = FindAtOrBefore(pBar->mGems, offset.mTick);
    if (it == pBar->mGems.end()) {
        if (nTick < mBarLength) {
            return 0;
        }

        const int nStart = ClampPosition(mBarLength * (nTick / mBarLength));
        (void)IsFiniteMBT(nStart); // Yes, the binary discards this call's result.
        (void)IsFiniteMBT(1);      // Yes, the binary discards this call's result.
        nTick = ClampPosition(nStart - 1);
        (void)IsFiniteMBT(nTick); // Yes, the binary discards this call's result.

        LocateMapped(nTick, pBar, offset);
        it = FindAtOrBefore(pBar->mGems, offset.mTick);
        if (it == pBar->mGems.end()) {
            return 0;
        }
    }

    const int nStart = ClampPosition(mBarLength * (nTick / mBarLength));
    (void)IsFiniteMBT(nStart); // Yes, the binary discards this call's result.
    const int nGemTick = ClampPosition(it->mPosition.mTick + nStart);
    (void)IsFiniteMBT(nGemTick); // Yes, the binary discards this call's result.
    *pTick = nGemTick;
    *pGem = it->mValue;
    return 1;
}

// 0x001d46f0
int TrackData::FindGemAtOrAfter(int nTick, int *pTick, int *pGem) const {
    const Bar *pBar;
    Mid::MBT offset;
    for (int nBarsSearched = 0; nBarsSearched < mUnknown30;) {
        LocateMapped(nTick, pBar, offset);
        if (pBar->mGems.size() != 0) {
            const auto it = FindAtOrAfter(pBar->mGems, offset.mTick);
            if (it != pBar->mGems.end()) {
                const int nStart = ClampPosition(mBarLength * (nTick / mBarLength));
                (void)IsFiniteMBT(nStart); // Yes, the binary discards this call's result.
                const int nGemTick = ClampPosition(it->mPosition.mTick + nStart);
                (void)IsFiniteMBT(nGemTick); // Yes, the binary discards this call's result.
                *pTick = nGemTick;
                *pGem = it->mValue;
                return 1;
            }
        }

        ++nBarsSearched;
        const int nStart = ClampPosition(mBarLength * (nTick / mBarLength));
        (void)IsFiniteMBT(nStart); // Yes, the binary discards this call's result.
        nTick = ClampPosition(nStart + mBarLength);
        (void)IsFiniteMBT(nTick); // Yes, the binary discards this call's result.
    }
    return 0;
}

// 0x001d4978
void TrackData::Print(std::ostream &stream) {
    const char *pszMode;
    switch (mKind) {
    case kTrackModeAxe:
        pszMode = "axe";
        break;
    case kTrackModeRiff:
        pszMode = "riff";
        break;
    case kTrackModeCatch:
        pszMode = "catch";
        break;
    default:
        pszMode = "none";
        break;
    }

    stream << "TrackData[" << mUnknown04 << "]" << std::endl;
    stream << "Chan = " << static_cast<int>(mChannel) << ". Mode = " << pszMode << std::endl;

    for (unsigned i = 0; i < mBars.size(); ++i) {
        stream << "Bar# " << i << std::endl;
        mBars[i].Print(stream);
        stream << std::endl << std::endl;
    }
}

// 0x001d4b40
void TrackData::Locate(int nTick, Bar *&pBar, Mid::MBT &offset) {
    const int nBar = nTick / mBarLength;
    const int nStart = ClampPosition(mBarLength * nBar);
    (void)IsFiniteMBT(nStart); // Yes, the binary discards this call's result.
    const int nOffset = ClampPosition(nTick - nStart);
    (void)IsFiniteMBT(nOffset); // Yes, the binary discards this call's result.
    offset.mTick = nOffset;
    pBar = &mBars[nBar];
}

// 0x001d4c58
void TrackData::LocateMapped(int nTick, const Bar *&pBar, Mid::MBT &offset) const {
    const int nBar = nTick / mBarLength;
    const int nMappedBar = mMap->Slot5(nBar);
    const int nStart = ClampPosition(mBarLength * nBar);
    (void)IsFiniteMBT(nStart); // Yes, the binary discards this call's result.
    const int nOffset = ClampPosition(nTick - nStart);
    (void)IsFiniteMBT(nOffset); // Yes, the binary discards this call's result.
    offset.mTick = nOffset;
    pBar = &mBars[nMappedBar];
}

// 0x001d4d90
void TrackData::AddPhrases(PhraseDatabase *pDatabase) {
    const int nStepCount = mMap->mSteps.back();
    const int nUnknown08 = QueryConfigValue(kBarUnknown08Query);

    for (int i = 0; i < nStepCount; ++i) {
        Phrase *pPhrase = pDatabase->GetPhrase(i);
        if (pPhrase == nullptr) {
            continue;
        }

        for (const auto &gem : pPhrase->mGems) {
            (void)IsFiniteMBT(kBarLength); // Yes, the binary discards this call's result.
            const int nStart = ClampPosition(i * kBarLength);
            (void)IsFiniteMBT(nStart); // Yes, the binary discards this call's result.
            const int nGemTick = ClampPosition(gem.mPosition.mTick + nStart);
            (void)IsFiniteMBT(nGemTick); // Yes, the binary discards this call's result.
            AddGem(nGemTick, gem.mGem, nullptr);
        }

        mBars[i].mPoints = ScoreGems(mBars[i].mGems);
        mBars[i].mUnknown08 = nUnknown08;
    }
}

// 0x001d7688
void TrackData::SetQuant(int nTick, int nQuant) {
    const int nLength = ClampPosition(mBarLength * static_cast<int>(mBars.size()));
    (void)IsFiniteMBT(nLength); // Yes, the binary discards this call's result.
    if (!(nTick < nLength)) {
        return;
    }

    Bar *pBar;
    Locate(nTick, pBar);
    pBar->mQuant = nQuant;
}

// 0x001d7758
void TrackData::OnUnknown001d7758([[maybe_unused]] int nFirst, [[maybe_unused]] int nSecond) {
}

// 0x001d7760
void TrackData::SetOwner(Player *pPlayer, int nBar) {
    mGamer->SetBarOwner(mUnknown04, nBar, pPlayer);
}

// 0x001d7788
Harmony *TrackData::GetHarmony(int nTick) const {
    const Bar *pBar;
    Mid::MBT offset;
    LocateMapped(nTick, pBar, offset);

    const auto it = FindAtOrBefore(pBar->mHarmonies, offset.mTick);
    if (it == pBar->mHarmonies.end()) {
        return nullptr;
    }
    return it->mValue;
}

// 0x001d77e0
Riff *TrackData::GetRiff(int nTick, int nLevel) const {
    const Bar *pBar;
    Mid::MBT offset;
    LocateMapped(nTick, pBar, offset);

    const auto it = FindAtOrBefore(pBar->mRiffSets, offset.mTick);
    if (it == pBar->mRiffSets.end() || !(static_cast<unsigned>(nLevel) < kRiffSetLevelCount)) {
        return nullptr;
    }
    return it->mValue->mRiffs[nLevel];
}

// 0x001d7858
Riff *TrackData::GetRiffInMappedBar(int nBar, int nOffset, int nLevel) const {
    const Bar *pBar;
    GetBar(nBar, pBar);

    const auto it = FindAtOrBefore(pBar->mRiffSets, nOffset);
    if (it == pBar->mRiffSets.end() || !(static_cast<unsigned>(nLevel) < kRiffSetLevelCount)) {
        return nullptr;
    }
    return it->mValue->mRiffs[nLevel];
}

// 0x001d78d0
Riff *TrackData::GetRiffInBar(int nBar, int nOffset, int nLevel) const {
    const Bar &bar = mBars[nBar];
    const auto it = FindAtOrBefore(bar.mRiffSets, nOffset);
    if (it == bar.mRiffSets.end() || !(static_cast<unsigned>(nLevel) < kRiffSetLevelCount)) {
        return nullptr;
    }
    return it->mValue->mRiffs[nLevel];
}

// 0x001d7948
int TrackData::GetGemAt(int nTick) const {
    const Bar *pBar;
    Mid::MBT offset;
    LocateMapped(nTick, pBar, offset);

    const auto it = FindAtOrBefore(pBar->mGems, offset.mTick);
    if (it != pBar->mGems.end() && it->mPosition.mTick == offset.mTick) {
        return it->mValue;
    }
    return kNoGem;
}

// 0x001d79a8
int TrackData::GetQuant(int nBar) const {
    const Bar *pBar;
    GetBar(nBar, pBar);
    return pBar->mQuant;
}

// 0x001d79d0
int TrackData::IsStepStart(int nBar) const {
    return mMap->IsStepStart(nBar);
}

// 0x001d79f0
int TrackData::StepStartBar(int nBar) const {
    return mMap->StepStartBar(nBar);
}

// 0x001d7a10
int TrackData::NextStepBar(int nBar) const {
    return mMap->NextStepBar(nBar);
}

// 0x001d7a30
int TrackData::FollowingStepBar(int nBar) const {
    return mMap->FollowingStepBar(nBar);
}

// 0x001d7a50
int TrackData::QueryBar(int nBar) const {
    return mGamer->QueryBar(mUnknown04, nBar);
}

// 0x001d7a78
int TrackData::GetPoints(int nBar) const {
    const Bar *pBar;
    GetBar(nBar, pBar);
    return pBar->mPoints;
}

// 0x001d7aa0
int TrackData::GetUnknown08(int nBar) const {
    const Bar *pBar;
    GetBar(nBar, pBar);
    return pBar->mUnknown08;
}

// 0x001d7ac8
const std::vector<TickObj<MuseMsg *> > *TrackData::GetMidiInBar(int nBar) const {
    return &mBars[nBar].mMidi;
}

// 0x001d7ae0
const std::vector<TickObj<int> > *TrackData::GetGemsInBar(int nBar) const {
    return &mBars[nBar].mGems;
}

// 0x001d7af8
const std::vector<TickObj<MuseMsg *> > *TrackData::GetMidi(int nBar) const {
    const Bar *pBar;
    GetBar(nBar, pBar);
    return &pBar->mMidi;
}

// 0x001d7b20
const std::vector<TickObj<int> > *TrackData::GetGems(int nBar) const {
    const Bar *pBar;
    GetBar(nBar, pBar);
    return &pBar->mGems;
}

// 0x001d7b48
void TrackData::Locate(int nTick, Bar *&pBar) {
    Mid::MBT offset;
    Locate(nTick, pBar, offset);
}

// 0x001d7b70
void TrackData::LocateMapped(int nTick, const Bar *&pBar) const {
    Mid::MBT offset;
    LocateMapped(nTick, pBar, offset);
}

// 0x001d7b98
void TrackData::GetBar(int nBar, const Bar *&pBar) const {
    pBar = &mBars[mMap->Slot5(nBar)];
}

// 0x001d7bf0
int TrackData::FindStepIndex(int nBar) const {
    return mMap->FindStepIndex(mMap->Slot5(nBar));
}

// 0x001d2e08
int TrackData::ScoreGems(const std::vector<TickObj<int> > &gems) {
    int nScore = 0;
    if (g_bScoreTablesLoaded == 0) {
        InitScoreTables();
    }

    for (const auto &gem : gems) {
        for (const auto &rule : g_aScoreRules) {
            if (gem.mPosition.mTick % rule.mDivisor == 0) {
                nScore += rule.mWeight;
                break;
            }
        }
    }

    return static_cast<int>(
        std::upper_bound(g_scoreThresholds.begin(), g_scoreThresholds.end(), nScore) -
        g_scoreThresholds.begin());
}

// 0x001d2ca8
void TrackData::InitScoreTables() {
    std::vector<int> weights;
    QueryConfigVector(&weights, kScoreWeightsQuery);
    for (int i = 0; i < kScoreDivisorCount; ++i) {
        g_aScoreRules[i].mWeight = weights[i];
    }

    QueryConfigVector(&g_scoreThresholds, kScoreThresholdsQuery);
    g_bScoreTablesLoaded = 1;
}
