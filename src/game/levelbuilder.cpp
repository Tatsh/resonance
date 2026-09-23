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

// 0x001ec328
// Deleter the destructor runs over all three collections. The class is file-private and
// non-polymorphic, so the name is inferred from the body.
void DeleteTrackData(TrackData *pTrack) {
    delete pTrack;
}

// Configuration codes the constructor queries.
constexpr int kPlayMapSlot17Query = 0x3a1;
constexpr int kPlayMapStepsQuery = 0x396;
constexpr int kPlayMapLabelsQuery = 0x3a0;
constexpr int kPlayMapSlot20Query = 0x39d;

// The tempo a level starts with, 120 beats per minute.
constexpr int kDefaultMicrosecondsPerQuarter = 500000;

// The constructor has PlayMapLinear's constructor run LoadStepRings().
constexpr int kPlayMapLoadStepRings = 1;

// The argument the constructor passes to PlayMap::Slot17() when kPlayMapSlot17Query is set.
constexpr int kPlayMapSlot17Argument = 0xe;

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

// 0x001ea838
LevelBuilder::LevelBuilder(unsigned nTrackCount)
    : mOwnTrack(nullptr), mCurrentTrack(nullptr), mUnknown30(nullptr) {
    const int bSlot17 = QueryConfigFlag(kPlayMapSlot17Query);
    mUnknown30 = new Sch::TempoMap(kDefaultMicrosecondsPerQuarter);
    PlayMapLinear *pMap = new PlayMapLinear(kPlayMapLoadStepRings);
    mUnknown34 = pMap;

    std::vector<int> steps;
    QueryConfigVector(&steps, kPlayMapStepsQuery);
    std::vector<HxStr> labels;
    QueryConfigStrings(&labels, kPlayMapLabelsQuery);
    for (unsigned i = 0; i < steps.size(); ++i) {
        mUnknown34->Slot2(steps[i], labels[i]);
    }

    mTracks.resize(nTrackCount, nullptr);
    for (unsigned i = 0; i < nTrackCount; ++i) {
        mTracks[i] = new TrackData(i, mUnknown34);
    }

    {
        std::vector<int> values;
        QueryConfigVector(&values, kPlayMapSlot20Query);
        for (auto it = values.begin(); it != values.end(); ++it) {
            pMap->Slot20(*it);
        }
        pMap->Slot19();
    }

    if (bSlot17) {
        mUnknown34->Slot17(kPlayMapSlot17Argument); // Yes, the binary discards the result.
    }
}

// 0x001eafe8
// The table store, the three vector deallocations, and the object release are compiler
// expansions.
LevelBuilder::~LevelBuilder() {
    std::for_each(mTracks.begin(), mTracks.end(), DeleteTrackData);
    std::for_each(mBackingTracks.begin(), mBackingTracks.end(), DeleteTrackData);
    std::for_each(mIntroTracks.begin(), mIntroTracks.end(), DeleteTrackData);
    delete mOwnTrack;
    if (mUnknown30 != nullptr) {
        mUnknown30->Release();
    }
    // The image deletes the object at +0x34 here through its own table slot 1 with an in-charge
    // argument of 3. The statement is omitted rather than written, because the object's class is
    // unrecovered and deleting through an untyped pointer would assert the wrong thing.
}

// 0x001ec430
int LevelBuilder::TrackCount() {
    return static_cast<int>(mTracks.size());
}

// 0x001ec448
int LevelBuilder::BackingTrackCount() {
    return static_cast<int>(mBackingTracks.size());
}

// 0x001ec6d0
TrackData *LevelBuilder::OwnTrack() {
    return mOwnTrack;
}

// 0x001ec6f0
// The index is not tested against the collection.
TrackData *LevelBuilder::TrackAt(int nIndex) {
    return mTracks[nIndex];
}

// 0x001ec6d8
TrackData *LevelBuilder::GetTrack(int nIndex) {
    return mTracks[nIndex];
}

// 0x001ec708
// The index is not tested against the collection.
TrackData *LevelBuilder::BackingTrackAt(int nIndex) {
    return mBackingTracks[nIndex];
}

// 0x001ec720
// The index is not tested against the collection.
TrackData *LevelBuilder::IntroTrackAt(int nIndex) {
    return mIntroTracks[nIndex];
}

// 0x001ec478
Sch::TempoMap *LevelBuilder::OnUnknownSlot7() {
    return mUnknown30;
}

// 0x001ec480
PlayMap *LevelBuilder::OnUnknownSlot8() {
    return mUnknown34;
}

// 0x001ec738
int LevelBuilder::OnUnknownSlot9() {
    return mUnknown34->Slot8();
}

// 0x001eb200
void LevelBuilder::SelectTrack(int nKind, int nIndex) {
    switch (nKind) {
    case kLevelTrackNone:
        mCurrentTrack = nullptr;
        break;
    case kLevelTrackBacking:
        mCurrentTrack = SelectCollectionTrack(mBackingTracks, nIndex, mUnknown34);
        break;
    case kLevelTrackIntro:
        mCurrentTrack = SelectCollectionTrack(mIntroTracks, nIndex, mUnknown34);
        break;
    case kLevelTrackScore:
        mCurrentTrack = mTracks[nIndex];
        break;
    case kLevelTrackOwn:
        if (mOwnTrack == nullptr) {
            mOwnTrack = new TrackData(kUnindexedTrack, mUnknown34);
        }
        mCurrentTrack = mOwnTrack;
        break;
    }
}

// 0x001eb4a8
void LevelBuilder::Print(std::ostream &stream) {
    PrintCollection(stream, "Score Track#", mTracks);
    PrintCollection(stream, "Backing Track#", mBackingTracks);
    PrintCollection(stream, "Intro Track#", mIntroTracks);
}

// 0x001ec498
void LevelBuilder::SetBarCount(int nBarCount) {
    mUnknown34->Slot3(nBarCount);
}

// 0x001ec488
void LevelBuilder::SetChannel(unsigned char nChannel) {
    mCurrentTrack->mChannel = nChannel;
}

// 0x001ec5c0
void LevelBuilder::SetKind(int nKind) {
    mCurrentTrack->mKind = nKind;
}

// 0x001ec5d0
void LevelBuilder::SetInstrument(int nInstrument, const HxStr &name) {
    mCurrentTrack->mInstrument = nInstrument;
    mCurrentTrack->mName = name;
}

// 0x001ec4c8
void LevelBuilder::AddEvent(int nTick,
                            unsigned char nStatus,
                            unsigned char nData1,
                            unsigned char nData2,
                            unsigned char nChannel) {
    mCurrentTrack->AddMidiMsg(nTick, nChannel | nStatus, nData1, nData2);
}

// 0x001ec4f8
void LevelBuilder::AddNoteMsg(
    int nTick, unsigned char nNote, unsigned char nVelocity, int nLength, unsigned char nChannel) {
    mCurrentTrack->AddNoteMsg(nTick, nNote, nVelocity, nLength, nChannel);
}

// 0x001ec520
void LevelBuilder::SetQuant(int nTick, int nQuant) {
    mCurrentTrack->SetQuant(nTick, nQuant);
}

// 0x001ec540
void LevelBuilder::AddRiff(int nTick, Riff *pRiff) {
    mCurrentTrack->AddRiff(nTick, pRiff);
}

// 0x001ec560
void LevelBuilder::AddHarmony(int nTick, const Harmony &harmony) {
    mCurrentTrack->AddHarmony(nTick, harmony);
}

// 0x001ec580
void LevelBuilder::OnUnknownForwarder001ec580(int nFirst, int nSecond) {
    mCurrentTrack->OnUnknown001d7758(nFirst, nSecond);
}

// 0x001ec5a0
void LevelBuilder::AddGem(int nTick, int nGem, Riff *pRiff) {
    mCurrentTrack->AddGem(nTick, nGem, pRiff);
}

// 0x001ec680
void LevelBuilder::PrepareTracks() {
    for (auto it = mTracks.begin(); it != mTracks.end(); ++it) {
        (*it)->ScoreBars();
    }
}

// 0x001ec5f8
void LevelBuilder::SetTempo([[maybe_unused]] int nTick, int nMicrosecondsPerQuarter) {
    if (mUnknown30 != nullptr) {
        mUnknown30->Release();
    }
    mUnknown30 = new Sch::TempoMap(nMicrosecondsPerQuarter);
}
