#include "msg/gem.h"

#include <iostream>

#include "game/idableptr.h"
#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x001a2560, PAL: 0x001a82c8
// The stream Mid::MBT::Save() returns is not used.
void Gem::saveGuts(OBStream &stream) const {
    int gem = mGem;
    int trans = mTrans;
    int bar = mBar;
    OBStream &rest =
        stream.Write(&gem, sizeof(gem)).Write(&trans, sizeof(trans)).Write(&bar, sizeof(bar));
    mLoc.Save(rest);

    int id = mPlayer->mPlayerId;
    rest.Write(&id, sizeof(id));
}

// NTSC-U/C: 0x001a2630, PAL: 0x001a8398
// The binary tests the local's cached pointer before the -1 case, and that pointer
// is always null here, so the order does not change the result.
void Gem::restoreGuts(IBStream &stream) {
    IBStream &rest =
        stream.Read(&mGem, sizeof(mGem)).Read(&mTrans, sizeof(mTrans)).Read(&mBar, sizeof(mBar));
    mLoc.Load(rest);

    IDablePtr<Player> player;
    rest.Read(&player.mId, sizeof(player.mId));
    mPlayer = player.mId == -1 ? nullptr : static_cast<Player *>(player);
}

// NTSC-U/C: 0x001a2ce0, PAL: 0x001a8a48
void Gem::Print(std::ostream &stream) const {
    stream << "gem: " << mGem << " trans:" << mTrans << " bar:" << mBar << " loc:";
    mLoc.Print(stream);
    stream << " pid:" << mPlayer->mPlayerId;
}
