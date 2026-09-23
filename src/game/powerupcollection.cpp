#include "game/powerupcollection.h"

#include <algorithm>

#include "game/localplayer.h"
#include "game/powerup.h"
#include "msg/choosepowerupmsg.h"
#include "msg/powerupcountmsg.h"
#include "script/configquery.h"

namespace {

// The configuration code the constructor reads the list of kinds from.
constexpr int kPowerupKindsConfigCode = 0x389;

// The largest count one entry stores. AddPowerup ignores an entry already at this count.
constexpr int kMaximumCount = 9;

// The fourth argument Deploy() passes to Powerup::Deploy().
constexpr int kDeployUnused = 0;

// Predicate std::find_if runs over the entries. The name is a placeholder: the class is
// file-private, non-polymorphic, and four bytes, so no descriptor, allocation tag, or literal in
// the image supplies one. The body appears inside the instantiation at 0x001cc628, which dispatches
// Powerup::Type() on each entry and compares the result against the stored kind.
struct MatchesType {
    explicit MatchesType(int nType) : mType(nType) {
    }

    bool operator()(const PowerupCollection::Entry &entry) const {
        return entry.mPowerup->Type() == mType;
    }

    int mType;
};

} // namespace

// 0x001cad70
PowerupCollection::PowerupCollection(LocalPlayer *pOwner, int bUnlimited)
    : mSelected(-1), mOwner(pOwner), mUnlimited(bUnlimited) {
    std::vector<int> types;
    QueryConfigVector(&types, kPowerupKindsConfigCode);
    for (std::vector<int>::iterator it = types.begin(); it != types.end(); ++it) {
        Entry entry;
        entry.mPowerup = Powerup::CreateForType(*it);
        entry.mCount = 0;
        mEntries.push_back(entry);
        if (mUnlimited != 0) {
            mEntries.back().mCount = 1;
            mSelected = 0;
        }
    }
}

// 0x001cb090. The vector release and the base destructor after it are both compiler expansions.
PowerupCollection::~PowerupCollection() {
    for (std::vector<Entry>::iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
        delete it->mPowerup;
    }
}

// 0x001cb230
void PowerupCollection::AddPowerup(int nType) {
    std::vector<Entry>::iterator it =
        std::find_if(mEntries.begin(), mEntries.end(), MatchesType(nType));
    if (it == mEntries.end()) {
        return;
    }
    if (it->mCount >= kMaximumCount) {
        return;
    }
    ++it->mCount;
    PowerupCountMsg msg(it - mEntries.begin(), it->mCount, mOwner);
    Send(&msg);
    if (mSelected == -1) {
        SelectRelative(1);
    }
}

// 0x001cb320
void PowerupCollection::SelectRelative(int nDelta) {
    if (nDelta == 0) {
        return;
    }
    const int nCount = static_cast<int>(mEntries.size());
    int nTries = 0;
    while (nTries < nCount) {
        mSelected += nDelta;
        if (mSelected < 0) {
            mSelected = nCount - 1;
        } else if (mSelected >= nCount) {
            mSelected = 0;
        }
        if (mEntries[mSelected].mCount > 0) {
            break;
        }
        ++nTries;
    }
    if (nTries >= nCount) {
        mSelected = -1;
    }
    int nType = -1;
    if (mSelected != -1) {
        nType = mEntries[mSelected].mPowerup->Type();
    }
    ChoosePowerupMsg msg(mSelected, mOwner, nType);
    Send(&msg);
}

// 0x001cb450. The index is stored before it is used, and an index outside the vector is not
// tested for.
void PowerupCollection::Select(int nIndex) {
    mSelected = nIndex;
    int nType = -1;
    if (nIndex != -1) {
        nType = mEntries[nIndex].mPowerup->Type();
    }
    ChoosePowerupMsg msg(mSelected, mOwner, nType);
    Send(&msg);
}

// 0x001cb500. The selected entry is read with no test against -1.
void PowerupCollection::Deploy(int nTrack, int nBar) {
    if (mEntries[mSelected].mPowerup->Deploy(nTrack, nBar, mOwner, kDeployUnused) == 0) {
        return;
    }
    if (mUnlimited != 0) {
        return;
    }
    --mEntries[mSelected].mCount;
    PowerupCountMsg msg(mSelected, mEntries[mSelected].mCount, mOwner);
    Send(&msg);
    if (mEntries[mSelected].mCount == 0) {
        SelectRelative(1);
    }
}

// 0x001cc9a0
int PowerupCollection::HasSelection() {
    return mSelected != -1;
}

// 0x001cb620
void PowerupCollection::AnnounceState() {
    for (std::vector<Entry>::iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
        if (it->mCount != 0) {
            PowerupCountMsg msg(it - mEntries.begin(), it->mCount, mOwner);
            Send(&msg);
        }
    }
    int nType = -1;
    if (mSelected != -1) {
        nType = mEntries[mSelected].mPowerup->Type();
    }
    ChoosePowerupMsg msg(mSelected, mOwner, nType);
    Send(&msg);
}
