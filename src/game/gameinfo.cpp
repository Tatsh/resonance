#include "game/gameinfo.h"

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

GameInfo::~GameInfo() {
}

void GameInfo::Save(OBStream &stream) {
    mAddress.Save(stream);
    unsigned int nWorldId = mMediusWorldId;
    OBStream &out = SaveHxStr(stream.WriteLE(&nWorldId, sizeof(nWorldId)), mHost);
    mParams.Save(&out);
    int nStatus = mStatus;
    out.WriteLE(&nStatus, sizeof(nStatus));
}

void GameInfo::Load(IBStream &stream) {
    mAddress.Load(stream);
    IBStream &in = LoadHxStr(stream.ReadLE(&mMediusWorldId, sizeof(mMediusWorldId)), mHost);
    mParams.Load(&in);
    in.ReadLE(&mStatus, sizeof(mStatus));
}

void GameInfo::Print(std::ostream &stream) {
    std::ostream &out = stream << "addr=" << "mediusWorldID=" << mMediusWorldId
                               << " host=" << mHost;
    mParams.Print(out);
    out << " status=" << mStatus;
}
