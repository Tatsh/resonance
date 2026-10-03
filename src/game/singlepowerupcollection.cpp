#include "game/singlepowerupcollection.h"

#include "game/localplayer.h"
#include "game/powerup.h"
#include "msg/choosepowerupmsg.h"

namespace {

// The fourth argument Deploy() passes to Powerup::Deploy().
constexpr int kDeployUnused = 0;

} // namespace

// NTSC-U/C: 0x001cb760, PAL: 0x001d1618
SinglePowerupCollection::SinglePowerupCollection(LocalPlayer *pOwner)
    : mType(-1), mPowerup(nullptr), mOwner(pOwner) {
}

// NTSC-U/C: 0x001cb7b0, PAL: 0x001d1668
// The base destructor after the release is a compiler expansion.
SinglePowerupCollection::~SinglePowerupCollection() {
    delete mPowerup;
}

// NTSC-U/C: 0x001cb890, PAL: 0x001d1748
// The kind is stored before the powerup is built, and the message reports an index of
// 0 because the store holds one.
void SinglePowerupCollection::AddPowerup(int nType) {
    delete mPowerup;
    mType = nType;
    mPowerup = Powerup::CreateForType(nType);
    ChoosePowerupMsg msg(0, mOwner, mType);
    Send(&msg);
}

// NTSC-U/C: 0x001cb948, PAL: 0x001d1800
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

// NTSC-U/C: 0x001cc9f0, PAL: 0x001d28a8
void SinglePowerupCollection::SelectRelative(int) {
}

// NTSC-U/C: 0x001cc9f8, PAL: 0x001d28b0
void SinglePowerupCollection::Select(int) {
}

// NTSC-U/C: 0x001cca00, PAL: 0x001d28b8
int SinglePowerupCollection::HasSelection() {
    return mType != -1;
}

// NTSC-U/C: 0x001cba20, PAL: 0x001d18d8
void SinglePowerupCollection::AnnounceState() {
    if (HasSelection() == 0) {
        return;
    }
    ChoosePowerupMsg msg(0, mOwner, mType);
    Send(&msg);
}
