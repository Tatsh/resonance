#include "game/gameinfo.h"

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x00187a08
GameInfo::~GameInfo() {
}

// 0x001876f8
void GameInfo::Save(OBStream &stream) {
    mAddress.Save(stream);
    unsigned int nWorldId = mMediusWorldId;
    OBStream &out = SaveHxStr(stream.Write(&nWorldId, sizeof(nWorldId)), mHost);
    mParams.Save(&out);
    int nStatus = mStatus;
    out.Write(&nStatus, sizeof(nStatus));
}

// 0x00187800
void GameInfo::Load(IBStream &stream) {
    mAddress.Load(stream);
    IBStream &in = LoadHxStr(stream.Read(&mMediusWorldId, sizeof(mMediusWorldId)), mHost);
    mParams.Load(&in);
    in.Read(&mStatus, sizeof(mStatus));
}

// 0x00187c60
void GameInfo::Print(std::ostream &stream) {
    std::ostream &out = stream << "addr=" << "mediusWorldID=" << mMediusWorldId
                               << " host=" << mHost;
    mParams.Print(out);
    out << " status=" << mStatus;
}
