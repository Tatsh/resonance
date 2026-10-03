#include "msg/gem.h"

#include <iostream>

#include "game/idableptr.h"
#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x001a2560, PAL: 0x001a82c8
// The stream Sch::Tick::saveGuts() returns is not used.
void Gem::saveGuts(OBStream &stream) const {
    int gem = mGem;
    int trans = mTrans;
    int bar = mBar;
    OBStream &rest =
        stream.WriteLE(&gem, sizeof(gem)).WriteLE(&trans, sizeof(trans)).WriteLE(&bar, sizeof(bar));
    mLoc.saveGuts(rest);

    int id = mPlayer->mPlayerId;
    rest.WriteLE(&id, sizeof(id));
}

// NTSC-U/C: 0x001a2630, PAL: 0x001a8398
// The binary tests the local's cached pointer before the -1 case, and that pointer
// is always null here, so the order does not change the result.
void Gem::restoreGuts(IBStream &stream) {
    IBStream &rest = stream.ReadLE(&mGem, sizeof(mGem))
                         .ReadLE(&mTrans, sizeof(mTrans))
                         .ReadLE(&mBar, sizeof(mBar));
    mLoc.restoreGuts(rest);

    IDablePtr<Player> player;
    rest.ReadLE(&player.mId, sizeof(player.mId));
    mPlayer = player.mId == -1 ? nullptr : static_cast<Player *>(player);
}

// NTSC-U/C: 0x001a2ce0, PAL: 0x001a8a48
void Gem::Print(std::ostream &stream) const {
    stream << "gem: " << mGem << " trans:" << mTrans << " bar:" << mBar << " loc:";
    mLoc.Print(stream);
    stream << " pid:" << mPlayer->mPlayerId;
}
