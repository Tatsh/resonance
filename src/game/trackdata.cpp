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
#include "mid/tick.h"
#include "msg/musemsg.h"
#include "msg/notemsg.h"
#include "msg/stdmidimsg.h"
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
constexpr int kFlatPointsQuery = 917;
constexpr int kCatchPointsQuery = 907;
constexpr int kScoreWeightsQuery = 935;
constexpr int kScoreThresholdsQuery = 936;

// The gem value GetGemAt() reports when no gem sits at the position.
constexpr int kNoGem = -1;

// ScoreGems() scans this many divisors.
constexpr int kScoreDivisorCount = 6;

// One scoring rule. A gem whose position is a multiple of mDivisor adds mWeight.
struct ScoreRule {
    int mDivisor;
    int mWeight;
};

// NTSC-U/C: 0x0068fef8, PAL: 0x006d1178
// The weights are overwritten from the configuration on first use.
ScoreRule g_aScoreRules[kScoreDivisorCount] = {
    {1920, 15},
    {960, 25},
    {480, 35},
    {240, 50},
    {120, 70},
    {1, 70},
};

// NTSC-U/C: 0x0068ff28, PAL: 0x006d11a8
std::vector<int> g_scoreThresholds;

// NTSC-U/C: 0x0068ff34, PAL: 0x006d11b4
int g_bScoreTablesLoaded;

// A computed position, clamped to the finite range as the inline Sch::Tick arithmetic does.
inline Sch::Tick MakePosition(int nTick) {
    return Sch::Tick(std::min(std::max(nTick, kTickMinimum), kTickMaximum));
}

// NTSC-U/C: 0x001d75a0, PAL: 0x001dd480
void DeleteMidi(TickObj<MuseMsg *> entry) {
    delete entry.mValue;
}

// NTSC-U/C: 0x001d75d8, PAL: 0x001dd4b8
void DeleteRiffSet(RiffSet *pRiffSet) {
    delete pRiffSet;
}

// NTSC-U/C: 0x001d75f8, PAL: 0x001dd4d8
void DeleteHarmony(Harmony *pHarmony) {
    delete pHarmony;
}

} // namespace

TrackData::Bar::Bar() : mQuant(kDefaultQuant), mPoints(0) {
}

TrackData::Bar::~Bar() {
    std::for_each(mMidi.begin(), mMidi.end(), DeleteMidi);
}

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
        PrintMuseMsgTickObj(stream, entry.mPosition, entry.mValue);
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

TrackData::TrackData(int nIndex, PlayMap *pMap)
    : mIndex(nIndex), mKind(kTrackModeRiff), mMap(pMap), mBars(pMap->mSteps.back()) {
    mBarLength = Sch::Tick(kBarLength).mTick;
    mGemSearchBars = kGemSearchBars;
    mCurrentRiffSet = nullptr;
    mCurrentRiffTick = Sch::Tick(kNoRiffTick).mTick;
}

TrackData::~TrackData() {
    std::for_each(mRiffSetsOwned.begin(), mRiffSetsOwned.end(), DeleteRiffSet);
    std::for_each(mHarmoniesOwned.begin(), mHarmoniesOwned.end(), DeleteHarmony);
}

void TrackData::AddRiff(int nTick, Riff *pRiff) {
    const Sch::Tick length = MakePosition(mBarLength * static_cast<int>(mBars.size()));
    if (!(nTick < length.mTick)) {
        return;
    }

    if (mCurrentRiffTick != nTick) {
        Bar *pBar;
        Sch::Tick offset;
        Locate(nTick, pBar, offset);

        mCurrentRiffSet = new RiffSet;
        mCurrentRiffTick = nTick;
        mRiffSetsOwned.push_back(mCurrentRiffSet);
        SetAtTick(pBar->mRiffSets, mCurrentRiffSet, offset.mTick);

        for (auto it = mBars.begin() + (pBar - mBars.data()) + 1; it != mBars.end(); ++it) {
            SetAtTick(it->mRiffSets, mCurrentRiffSet, Sch::Tick(0).mTick);
        }
    }
    mCurrentRiffSet->mRiffs[pRiff->mId] = pRiff;
}

void TrackData::AddHarmony(int nTick, const Harmony &harmony) {
    const Sch::Tick length = MakePosition(mBarLength * static_cast<int>(mBars.size()));
    if (!(nTick < length.mTick)) {
        return;
    }

    Bar *pBar;
    Sch::Tick offset;
    Locate(nTick, pBar, offset);

    Harmony *pHarmony = new Harmony(harmony);
    mHarmoniesOwned.push_back(pHarmony);
    SetAtTick(pBar->mHarmonies, pHarmony, offset.mTick);

    for (auto it = mBars.begin() + (pBar - mBars.data()) + 1; it != mBars.end(); ++it) {
        SetAtTick(it->mHarmonies, pHarmony, Sch::Tick(0).mTick);
    }
}

void TrackData::AddGem(int nTick, int nGem, Riff *pRiff) {
    const Sch::Tick length = MakePosition(mBarLength * static_cast<int>(mBars.size()));
    if (!(nTick < length.mTick)) {
        return;
    }

    Bar *pBar;
    Sch::Tick offset;
    Locate(nTick, pBar, offset);
    InsertAtTick(pBar->mGems, nGem, offset.mTick);

    if (pRiff != nullptr) {
        RiffSet *pRiffSet = new RiffSet;
        mRiffSetsOwned.push_back(pRiffSet);
        pRiffSet->mRiffs[pRiff->mId] = pRiff;
        InsertAtTick(pBar->mRiffSets, pRiffSet, offset.mTick);
    }
}

void TrackData::AddMidiMsg(int nTick,
                           unsigned char nStatus,
                           unsigned char nData1,
                           unsigned char nData2) {
    const Sch::Tick length = MakePosition(mBarLength * static_cast<int>(mBars.size()));
    if (!(nTick < length.mTick)) {
        return;
    }

    Bar *pBar;
    Sch::Tick offset;
    Locate(nTick, pBar, offset);
    MuseMsg *pMsg = new StdMidiMsg(offset.mTick, nStatus, nData1, nData2);
    InsertAtTick(pBar->mMidi, pMsg, offset.mTick);
}

void TrackData::AddNoteMsg(
    int nTick, unsigned char nNote, unsigned char nVelocity, int nLength, unsigned char nChannel) {
    const Sch::Tick length = MakePosition(mBarLength * static_cast<int>(mBars.size()));
    if (!(nTick < length.mTick)) {
        return;
    }

    Bar *pBar;
    Sch::Tick offset;
    Locate(nTick, pBar, offset);
    Sch::Tick noteLength; // The length is stored without the finiteness check.
    noteLength.mTick = nLength;
    MuseMsg *pMsg = new NoteMsg(offset.mTick, nChannel, nNote, nVelocity, noteLength);
    InsertAtTick(pBar->mMidi, pMsg, offset.mTick);
}

void TrackData::ScoreBars() {
    if (mIndex == kSkippedTrack) {
        return;
    }

    const int nFlatPoints = QueryConfigValue(kFlatPointsQuery);
    (void)Application::shared()->GetGameMode(); // Yes, the binary discards this call's result.
    const int nCatchPoints = QueryConfigValue(kCatchPointsQuery);

    for (unsigned i = 0; i != mBars.size(); ++i) {
        int nPoints = nFlatPoints;
        if (!(static_cast<unsigned>(mKind - 1) < kFlatPointModeCount)) {
            nPoints = ScoreGems(mBars[i].mGems);
        }
        mBars[i].mPoints = nPoints;
        mBars[i].mCatchPoints = nCatchPoints;
    }
}

int TrackData::FindGemAtOrBefore(int nTick, int *pTick, int *pGem) const {
    const Bar *pBar;
    Sch::Tick offset;
    LocateMapped(nTick, pBar, offset);

    auto it = FindAtOrBefore(pBar->mGems, offset.mTick);
    if (it == pBar->mGems.end()) {
        if (nTick < mBarLength) {
            return 0;
        }

        const Sch::Tick start = MakePosition(mBarLength * (nTick / mBarLength));
        nTick = MakePosition(start.mTick - Sch::Tick(1).mTick).mTick;

        LocateMapped(nTick, pBar, offset);
        it = FindAtOrBefore(pBar->mGems, offset.mTick);
        if (it == pBar->mGems.end()) {
            return 0;
        }
    }

    const Sch::Tick start = MakePosition(mBarLength * (nTick / mBarLength));
    *pTick = MakePosition(it->mPosition.mTick + start.mTick).mTick;
    *pGem = it->mValue;
    return 1;
}

int TrackData::FindGemAtOrAfter(int nTick, int *pTick, int *pGem) const {
    const Bar *pBar;
    Sch::Tick offset;
    for (int nBarsSearched = 0; nBarsSearched < mGemSearchBars;) {
        LocateMapped(nTick, pBar, offset);
        if (pBar->mGems.size() != 0) {
            const auto it = FindAtOrAfter(pBar->mGems, offset.mTick);
            if (it != pBar->mGems.end()) {
                const Sch::Tick start = MakePosition(mBarLength * (nTick / mBarLength));
                *pTick = MakePosition(it->mPosition.mTick + start.mTick).mTick;
                *pGem = it->mValue;
                return 1;
            }
        }

        ++nBarsSearched;
        const Sch::Tick start = MakePosition(mBarLength * (nTick / mBarLength));
        nTick = MakePosition(start.mTick + mBarLength).mTick;
    }
    return 0;
}

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

    stream << "TrackData[" << mIndex << "]" << std::endl;
    stream << "Chan = " << static_cast<int>(mChannel) << ". Mode = " << pszMode << std::endl;

    for (int i = 0; static_cast<unsigned>(i) < mBars.size(); ++i) {
        stream << "Bar# " << i << std::endl;
        mBars[i].Print(stream);
        stream << std::endl << std::endl;
    }
}

void TrackData::Locate(int nTick, Bar *&pBar, Sch::Tick &offset) {
    const int nBar = nTick / mBarLength;
    const Sch::Tick start = MakePosition(mBarLength * nBar);
    offset = MakePosition(nTick - start.mTick);
    pBar = &mBars[nBar];
}

void TrackData::LocateMapped(int nTick, const Bar *&pBar, Sch::Tick &offset) const {
    const int nBar = nTick / mBarLength;
    const int nMappedBar = mMap->MapBar(nBar);
    const Sch::Tick start = MakePosition(mBarLength * nBar);
    offset = MakePosition(nTick - start.mTick);
    pBar = &mBars[nMappedBar];
}

void TrackData::AddPhrases(PhraseDatabase *pDatabase) {
    const int nStepCount = mMap->mSteps.back();
    const int nCatchPoints = QueryConfigValue(kCatchPointsQuery);

    for (int i = 0; i < nStepCount; ++i) {
        Phrase *pPhrase = pDatabase->GetPhrase(i);
        if (pPhrase == nullptr) {
            continue;
        }

        for (const auto &gem : pPhrase->mGems) {
            const Sch::Tick start = MakePosition(i * Sch::Tick(kBarLength).mTick);
            AddGem(MakePosition(gem.mPosition.mTick + start.mTick).mTick, gem.mGem, nullptr);
        }

        mBars[i].mPoints = ScoreGems(mBars[i].mGems);
        mBars[i].mCatchPoints = nCatchPoints;
    }
}

void TrackData::SetQuant(int nTick, int nQuant) {
    const Sch::Tick length = MakePosition(mBarLength * static_cast<int>(mBars.size()));
    if (!(nTick < length.mTick)) {
        return;
    }

    Bar *pBar;
    Locate(nTick, pBar);
    pBar->mQuant = nQuant;
}

void TrackData::SetActive([[maybe_unused]] int nTick, [[maybe_unused]] int bActive) {
}

void TrackData::SetOwner(Player *pPlayer, int nBar) const {
    mGamer->SetBarOwner(mIndex, nBar, pPlayer);
}

Harmony *TrackData::GetHarmony(int nTick) const {
    const Bar *pBar;
    Sch::Tick offset;
    LocateMapped(nTick, pBar, offset);

    const auto it = FindAtOrBefore(pBar->mHarmonies, offset.mTick);
    if (it == pBar->mHarmonies.end()) {
        return nullptr;
    }
    return it->mValue;
}

Riff *TrackData::GetRiff(int nTick, int nLevel) const {
    const Bar *pBar;
    Sch::Tick offset;
    LocateMapped(nTick, pBar, offset);

    const auto it = FindAtOrBefore(pBar->mRiffSets, offset.mTick);
    if (it == pBar->mRiffSets.end() || !(static_cast<unsigned>(nLevel) < kRiffSetLevelCount)) {
        return nullptr;
    }
    return it->mValue->mRiffs[nLevel];
}

Riff *TrackData::GetRiffInMappedBar(int nBar, int nOffset, int nLevel) const {
    const Bar *pBar;
    GetBar(nBar, pBar);

    const auto it = FindAtOrBefore(pBar->mRiffSets, nOffset);
    if (it == pBar->mRiffSets.end() || !(static_cast<unsigned>(nLevel) < kRiffSetLevelCount)) {
        return nullptr;
    }
    return it->mValue->mRiffs[nLevel];
}

Riff *TrackData::GetRiffInBar(int nBar, int nOffset, int nLevel) const {
    const Bar &bar = mBars[nBar];
    const auto it = FindAtOrBefore(bar.mRiffSets, nOffset);
    if (it == bar.mRiffSets.end() || !(static_cast<unsigned>(nLevel) < kRiffSetLevelCount)) {
        return nullptr;
    }
    return it->mValue->mRiffs[nLevel];
}

int TrackData::GetGemAt(int nTick) const {
    const Bar *pBar;
    Sch::Tick offset;
    LocateMapped(nTick, pBar, offset);

    const auto it = FindAtOrBefore(pBar->mGems, offset.mTick);
    if (it != pBar->mGems.end() && it->mPosition.mTick == offset.mTick) {
        return it->mValue;
    }
    return kNoGem;
}

int TrackData::GetQuant(int nBar) const {
    const Bar *pBar;
    GetBar(nBar, pBar);
    return pBar->mQuant;
}

int TrackData::IsStepStart(int nBar) const {
    return mMap->IsStepStart(nBar);
}

int TrackData::StepStartBar(int nBar) const {
    return mMap->StepStartBar(nBar);
}

int TrackData::NextStepBar(int nBar) const {
    return mMap->NextStepBar(nBar);
}

int TrackData::FollowingStepBar(int nBar) const {
    return mMap->FollowingStepBar(nBar);
}

int TrackData::QueryBar(int nBar) const {
    return mGamer->QueryBar(mIndex, nBar);
}

int TrackData::GetPoints(int nBar) const {
    const Bar *pBar;
    GetBar(nBar, pBar);
    return pBar->mPoints;
}

int TrackData::GetJuice(int nBar) const {
    const Bar *pBar;
    GetBar(nBar, pBar);
    return pBar->mCatchPoints;
}

const std::vector<TickObj<MuseMsg *> > *TrackData::GetMidiInBar(int nBar) const {
    return &mBars[nBar].mMidi;
}

const std::vector<TickObj<int> > *TrackData::GetGemsInBar(int nBar) const {
    return &mBars[nBar].mGems;
}

const std::vector<TickObj<MuseMsg *> > *TrackData::GetMidi(int nBar) const {
    const Bar *pBar;
    GetBar(nBar, pBar);
    return &pBar->mMidi;
}

const std::vector<TickObj<int> > *TrackData::GetGems(int nBar) const {
    const Bar *pBar;
    GetBar(nBar, pBar);
    return &pBar->mGems;
}

void TrackData::Locate(int nTick, Bar *&pBar) {
    Sch::Tick offset;
    Locate(nTick, pBar, offset);
}

void TrackData::LocateMapped(int nTick, const Bar *&pBar) const {
    Sch::Tick offset;
    LocateMapped(nTick, pBar, offset);
}

void TrackData::GetBar(int nBar, const Bar *&pBar) const {
    pBar = &mBars[mMap->MapBar(nBar)];
}

int TrackData::FindStepIndex(int nBar) const {
    return mMap->FindStepIndex(mMap->MapBar(nBar));
}

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

void TrackData::InitScoreTables() {
    std::vector<int> weights;
    QueryConfigVector(&weights, kScoreWeightsQuery);
    for (int i = 0; i < kScoreDivisorCount; ++i) {
        g_aScoreRules[i].mWeight = weights[i];
    }

    QueryConfigVector(&g_scoreThresholds, kScoreThresholdsQuery);
    g_bScoreTablesLoaded = 1;
}
