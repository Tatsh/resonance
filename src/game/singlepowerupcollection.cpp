#include "game/singlepowerupcollection.h"

#include "game/localplayer.h"
#include "game/powerup.h"
#include "msg/choosepowerupmsg.h"

namespace {

// The fourth argument Deploy() passes to Powerup::Deploy().
constexpr int kDeployUnused = 0;

} // namespace

SinglePowerupCollection::SinglePowerupCollection(LocalPlayer *pOwner)
    : mType(-1), mPowerup(nullptr), mOwner(pOwner) {
}

SinglePowerupCollection::~SinglePowerupCollection() {
    delete mPowerup; // The base destructor after the release is a compiler expansion.
}

void SinglePowerupCollection::Add(PowerupType type) {
    delete mPowerup;
    mType = type; // The kind is stored before the powerup is built.
    mPowerup = Powerup::CreateForType(type);
    ChoosePowerupMsg msg(0, mOwner, mType);
    Send(&msg);
}

void SinglePowerupCollection::Deploy(int nTrack, int nBar) {
    if (mType == -1) {
        return;
    }
    if (mPowerup->Deploy(nTrack, nBar, mOwner, kDeployUnused) == 0) {
        return;
    }
    ChoosePowerupMsg msg(0, mOwner, -1);
    Send(&msg);
    delete mPowerup;
    mPowerup = nullptr;
    mType = -1;
}

void SinglePowerupCollection::SelectRelative(int) {
}

void SinglePowerupCollection::Select(int) {
}

int SinglePowerupCollection::HasSelection() const {
    return mType != -1;
}

void SinglePowerupCollection::SendState() const {
    if (HasSelection() == 0) {
        return;
    }
    ChoosePowerupMsg msg(0, mOwner, mType);
    Send(&msg);
}
