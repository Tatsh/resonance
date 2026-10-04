#include "game/powerupcollection.h"

#include <algorithm>

#include "game/localplayer.h"
#include "game/powerup.h"
#include "msg/choosepowerupmsg.h"
#include "msg/powerupcountmsg.h"
#include "script/configquery.h"

namespace {

// The configuration code the constructor reads the list of kinds from.
constexpr int kPowerupKindsConfigCode = 905;

// The largest count one entry stores. Add() ignores an entry already at this count.
constexpr int kMaximumCount = 9;

// The fourth argument Deploy() passes to Powerup::Deploy().
constexpr int kDeployUnused = 0;

} // namespace

PowerupCollection::PowerupCollection(LocalPlayer *pOwner, int bUnlimited)
    : mSelected(-1), mOwner(pOwner), mUnlimited(bUnlimited) {
    std::vector<int> types;
    QueryConfigVector(&types, kPowerupKindsConfigCode);
    for (std::vector<int>::iterator it = types.begin(); it != types.end(); ++it) {
        PowCount entry;
        entry.mPowerup = Powerup::CreateForType(*it);
        entry.mCount = 0;
        mEntries.push_back(entry);
        if (mUnlimited != 0) {
            mEntries.back().mCount = 1;
            mSelected = 0;
        }
    }
}

PowerupCollection::~PowerupCollection() {
    // The vector release and the base destructor after it are both compiler expansions.
    for (std::vector<PowCount>::iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
        delete it->mPowerup;
    }
}

void PowerupCollection::Add(PowerupType type) {
    std::vector<PowCount>::iterator it = std::find(mEntries.begin(), mEntries.end(), type);
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

void PowerupCollection::Select(int nIndex) {
    mSelected = nIndex; // The index is stored before it is used.
    int nType = -1;
    if (nIndex != -1) {
        nType = mEntries[nIndex].mPowerup->Type();
    }
    ChoosePowerupMsg msg(mSelected, mOwner, nType);
    Send(&msg);
}

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

int PowerupCollection::HasSelection() const {
    return mSelected != -1;
}

void PowerupCollection::SendState() const {
    for (std::vector<PowCount>::const_iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
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
