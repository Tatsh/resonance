#include "game/levelbuilder.h"

#include <algorithm>

#include "app/attachment.h"
#include "game/trackdata.h"
#include "sch/tempomap.h"

namespace {

// Deleter the destructor runs over all three collections. The class is file-private and
// non-polymorphic, so the name is inferred from the body.
void DeleteTrackData(TrackData *pTrack) {
    delete pTrack;
}

} // namespace

// 0x001eafe8. The table store, the three vector deallocations, and the object release are compiler
// expansions.
LevelBuilder::~LevelBuilder() {
    std::for_each(mTracks.begin(), mTracks.end(), DeleteTrackData);
    std::for_each(mUnknown10.begin(), mUnknown10.end(), DeleteTrackData);
    std::for_each(mUnknown1c.begin(), mUnknown1c.end(), DeleteTrackData);
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
int LevelBuilder::UnknownCount() {
    return static_cast<int>(mUnknown10.size());
}

// 0x001ec6d0
TrackData *LevelBuilder::OwnTrack() {
    return mOwnTrack;
}

// 0x001ec6f0. The index is not tested against the collection.
TrackData *LevelBuilder::TrackAt(int nIndex) {
    return mTracks[nIndex];
}

// 0x001ec708. The index is not tested against the collection.
TrackData *LevelBuilder::UnknownAt(int nIndex) {
    return mUnknown10[nIndex];
}

// 0x001ec478
Sch::TempoMap *LevelBuilder::OnUnknownSlot7() {
    return mUnknown30;
}

// 0x001ec480
PlayMap *LevelBuilder::OnUnknownSlot8() {
    return mUnknown34;
}
