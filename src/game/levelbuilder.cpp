#include "game/levelbuilder.h"

#include <algorithm>

#include "app/attachment.h"
#include "game/playmap.h"
#include "game/playmaplinear.h"
#include "game/trackdata.h"
#include "os/hxstr.h"
#include "sch/tempomap.h"
#include "script/configquery.h"

namespace {

// NTSC-U/C: 0x001ec328, PAL: 0x001f25b0
// Deleter the destructor runs over all three collections. The class is file-private and
// non-polymorphic, so the name is inferred from the body.
void DeleteTrackData(TrackData *pTrack) {
    delete pTrack;
}

// Configuration codes the constructor queries.
constexpr int kStartLoopQuery = 929;
constexpr int kPlayMapStepsQuery = 918;
constexpr int kPlayMapLabelsQuery = 928;
constexpr int kPlayMapSectionsQuery = 925;

// The tempo a level starts with, 120 beats per minute.
constexpr int kDefaultMicrosecondsPerQuarter = 500000;

// The constructor has PlayMapLinear's constructor run LoadStepRings().
constexpr int kPlayMapLoadStepRings = 1;

// The bar the constructor passes to PlayMap::StartLoop() when kStartLoopQuery is set.
constexpr int kStartLoopBar = 0xe;

// The index every track SelectTrack() creates is given.
constexpr int kUnindexedTrack = -1;

// Grows a backing or intro collection to include nIndex and fills an empty slot with a new track.
inline TrackData *
SelectCollectionTrack(std::vector<TrackData *> &tracks, int nIndex, PlayMap *pMap) {
    if (tracks.size() < static_cast<unsigned>(nIndex + 1)) {
        tracks.resize(nIndex + 1, nullptr);
    }
    if (tracks[nIndex] == nullptr) {
        tracks[nIndex] = new TrackData(kUnindexedTrack, pMap);
    }
    return tracks[nIndex];
}

// Writes one collection for Print(), each track under its label and position.
inline void
PrintCollection(std::ostream &stream, const char *pszLabel, std::vector<TrackData *> &tracks) {
    int i = 0;
    for (auto it = tracks.begin(); it != tracks.end(); ++it) {
        stream << pszLabel << i << ":" << std::endl;
        ++i;
        (*it)->Print(stream);
        stream << std::endl;
    }
}

} // namespace

// NTSC-U/C: 0x001ea838, PAL: 0x001f0aa8
LevelBuilder::LevelBuilder(unsigned nTrackCount)
    : mOwnTrack(nullptr), mCurrentTrack(nullptr), mTempoMap(nullptr) {
    const int bStartLoop = QueryConfigFlag(kStartLoopQuery);
    mTempoMap = new Sch::TempoMap(kDefaultMicrosecondsPerQuarter);
    PlayMapLinear *pMap = new PlayMapLinear(kPlayMapLoadStepRings);
    mPlayMap = pMap;

    std::vector<int> steps;
    QueryConfigVector(&steps, kPlayMapStepsQuery);
    std::vector<HxStr> labels;
    QueryConfigStrings(&labels, kPlayMapLabelsQuery);
    for (unsigned i = 0; i < steps.size(); ++i) {
        mPlayMap->AddStep(steps[i], labels[i]);
    }

    mTracks.resize(nTrackCount, nullptr);
    for (unsigned i = 0; i < nTrackCount; ++i) {
        mTracks[i] = new TrackData(i, mPlayMap);
    }

    {
        std::vector<int> sections;
        QueryConfigVector(&sections, kPlayMapSectionsQuery);
        for (auto it = sections.begin(); it != sections.end(); ++it) {
            pMap->AppendSection(*it);
        }
        pMap->RecordPattern();
    }

    if (bStartLoop) {
        mPlayMap->StartLoop(kStartLoopBar); // Yes, the binary discards the result.
    }
}

// NTSC-U/C: 0x001eafe8, PAL: 0x001f1270
// The table store, the three vector deallocations, and the object release are compiler
// expansions.
LevelBuilder::~LevelBuilder() {
    std::for_each(mTracks.begin(), mTracks.end(), DeleteTrackData);
    std::for_each(mBackingTracks.begin(), mBackingTracks.end(), DeleteTrackData);
    std::for_each(mIntroTracks.begin(), mIntroTracks.end(), DeleteTrackData);
    delete mOwnTrack;
    if (mTempoMap != nullptr) {
        mTempoMap->Release();
    }
    delete mPlayMap;
}

// NTSC-U/C: 0x001ec430, PAL: 0x001f26b8
int LevelBuilder::TrackCount() {
    return static_cast<int>(mTracks.size());
}

// NTSC-U/C: 0x001ec448, PAL: 0x001f26d0
int LevelBuilder::BackingTrackCount() {
    return static_cast<int>(mBackingTracks.size());
}

// NTSC-U/C: 0x001ec6d0, PAL: 0x001f2958
TrackData *LevelBuilder::OwnTrack() {
    return mOwnTrack;
}

// NTSC-U/C: 0x001ec6f0, PAL: 0x001f2978
// The index is not tested against the collection.
TrackData *LevelBuilder::TrackAt(int nIndex) {
    return mTracks[nIndex];
}

// NTSC-U/C: 0x001ec6d8, PAL: 0x001f2960
TrackData *LevelBuilder::GetTrack(int nIndex) {
    return mTracks[nIndex];
}

// NTSC-U/C: 0x001ec708, PAL: 0x001f2990
// The index is not tested against the collection.
TrackData *LevelBuilder::BackingTrackAt(int nIndex) {
    return mBackingTracks[nIndex];
}

// NTSC-U/C: 0x001ec720, PAL: 0x001f29a8
// The index is not tested against the collection.
TrackData *LevelBuilder::IntroTrackAt(int nIndex) {
    return mIntroTracks[nIndex];
}

// NTSC-U/C: 0x001ec478, PAL: 0x001f2700
Sch::TempoMap *LevelBuilder::GetTempoMap() {
    return mTempoMap;
}

// NTSC-U/C: 0x001ec480, PAL: 0x001f2708
PlayMap *LevelBuilder::GetPlayMap() {
    return mPlayMap;
}

// NTSC-U/C: 0x001ec738, PAL: 0x001f29c0
int LevelBuilder::GetEndBar() {
    return mPlayMap->GetExtent();
}

// NTSC-U/C: 0x001eb200, PAL: 0x001f1488
void LevelBuilder::SelectTrack(int nKind, int nIndex) {
    switch (nKind) {
    case kLevelTrackNone:
        mCurrentTrack = nullptr;
        break;
    case kLevelTrackBacking:
        mCurrentTrack = SelectCollectionTrack(mBackingTracks, nIndex, mPlayMap);
        break;
    case kLevelTrackIntro:
        mCurrentTrack = SelectCollectionTrack(mIntroTracks, nIndex, mPlayMap);
        break;
    case kLevelTrackScore:
        mCurrentTrack = mTracks[nIndex];
        break;
    case kLevelTrackOwn:
        if (mOwnTrack == nullptr) {
            mOwnTrack = new TrackData(kUnindexedTrack, mPlayMap);
        }
        mCurrentTrack = mOwnTrack;
        break;
    }
}

// NTSC-U/C: 0x001eb4a8, PAL: 0x001f1730
void LevelBuilder::Print(std::ostream &stream) {
    PrintCollection(stream, "Score Track#", mTracks);
    PrintCollection(stream, "Backing Track#", mBackingTracks);
    PrintCollection(stream, "Intro Track#", mIntroTracks);
}

// NTSC-U/C: 0x001ec498, PAL: 0x001f2720
void LevelBuilder::SetBarCount(int nBarCount) {
    mPlayMap->SetBarCount(nBarCount);
}

// NTSC-U/C: 0x001ec488, PAL: 0x001f2710
void LevelBuilder::SetChannel(unsigned char nChannel) {
    mCurrentTrack->mChannel = nChannel;
}

// NTSC-U/C: 0x001ec5c0, PAL: 0x001f2848
void LevelBuilder::SetKind(int nKind) {
    mCurrentTrack->mKind = nKind;
}

// NTSC-U/C: 0x001ec5d0, PAL: 0x001f2858
void LevelBuilder::SetInstrument(int nInstrument, const HxStr &name) {
    mCurrentTrack->mInstrument = nInstrument;
    mCurrentTrack->mName = name;
}

// NTSC-U/C: 0x001ec4c8, PAL: 0x001f2750
void LevelBuilder::AddEvent(int nTick,
                            unsigned char nStatus,
                            unsigned char nData1,
                            unsigned char nData2,
                            unsigned char nChannel) {
    mCurrentTrack->AddMidiMsg(nTick, nChannel | nStatus, nData1, nData2);
}

// NTSC-U/C: 0x001ec4f8, PAL: 0x001f2780
void LevelBuilder::AddNoteMsg(
    int nTick, unsigned char nNote, unsigned char nVelocity, int nLength, unsigned char nChannel) {
    mCurrentTrack->AddNoteMsg(nTick, nNote, nVelocity, nLength, nChannel);
}

// NTSC-U/C: 0x001ec520, PAL: 0x001f27a8
void LevelBuilder::SetQuant(int nTick, int nQuant) {
    mCurrentTrack->SetQuant(nTick, nQuant);
}

// NTSC-U/C: 0x001ec540, PAL: 0x001f27c8
void LevelBuilder::AddRiff(int nTick, Riff *pRiff) {
    mCurrentTrack->AddRiff(nTick, pRiff);
}

// NTSC-U/C: 0x001ec560, PAL: 0x001f27e8
void LevelBuilder::AddHarmony(int nTick, const Harmony &harmony) {
    mCurrentTrack->AddHarmony(nTick, harmony);
}

// NTSC-U/C: 0x001ec580, PAL: 0x001f2808
void LevelBuilder::SetActive(int nTick, int bActive) {
    mCurrentTrack->SetActive(nTick, bActive);
}

// NTSC-U/C: 0x001ec5a0, PAL: 0x001f2828
void LevelBuilder::AddGem(int nTick, int nGem, Riff *pRiff) {
    mCurrentTrack->AddGem(nTick, nGem, pRiff);
}

// NTSC-U/C: 0x001ec680, PAL: 0x001f2908
void LevelBuilder::PrepareTracks() {
    for (auto it = mTracks.begin(); it != mTracks.end(); ++it) {
        (*it)->ScoreBars();
    }
}

// NTSC-U/C: 0x001ec5f8, PAL: 0x001f2880
void LevelBuilder::SetTempo([[maybe_unused]] int nTick, int nMicrosecondsPerQuarter) {
    if (mTempoMap != nullptr) {
        mTempoMap->Release();
    }
    mTempoMap = new Sch::TempoMap(nMicrosecondsPerQuarter);
}
