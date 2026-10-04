#include "msg/gem.h"

#include <iostream>

#include "game/idableptr.h"
#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

void Gem::saveGuts(OBStream &stream) const {
    int gem = mGem;
    int trans = mTrans;
    int bar = mBar;
    OBStream &rest =
        stream.WriteLE(&gem, sizeof(gem)).WriteLE(&trans, sizeof(trans)).WriteLE(&bar, sizeof(bar));
    mLoc.saveGuts(rest); // The stream Sch::Tick::saveGuts() returns is not used.

    int id = mPlayer->mPlayerId;
    rest.WriteLE(&id, sizeof(id));
}

void Gem::restoreGuts(IBStream &stream) {
    IBStream &rest = stream.ReadLE(&mGem, sizeof(mGem))
                         .ReadLE(&mTrans, sizeof(mTrans))
                         .ReadLE(&mBar, sizeof(mBar));
    mLoc.restoreGuts(rest);

    IDablePtr<Player> player;
    rest.ReadLE(&player.mId, sizeof(player.mId));
    // The binary tests the local's cached pointer before the -1 case. The pointer is always null
    // here, and the order does not change the result.
    mPlayer = player.mId == -1 ? nullptr : static_cast<Player *>(player);
}

void Gem::Print(std::ostream &stream) const {
    stream << "gem: " << mGem << " trans:" << mTrans << " bar:" << mBar << " loc:";
    mLoc.Print(stream);
    stream << " pid:" << mPlayer->mPlayerId;
}
