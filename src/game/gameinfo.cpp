#include "game/gameinfo.h"

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x00187a08, PAL: 0x0018d230
GameInfo::~GameInfo() {
}

// NTSC-U/C: 0x001876f8, PAL: 0x0018cef0
void GameInfo::Save(OBStream &stream) {
    mAddress.Save(stream);
    unsigned int nWorldId = mMediusWorldId;
    OBStream &out = SaveHxStr(stream.WriteLE(&nWorldId, sizeof(nWorldId)), mHost);
    mParams.Save(&out);
    int nStatus = mStatus;
    out.WriteLE(&nStatus, sizeof(nStatus));
}

// NTSC-U/C: 0x00187800, PAL: 0x0018cff8
void GameInfo::Load(IBStream &stream) {
    mAddress.Load(stream);
    IBStream &in = LoadHxStr(stream.ReadLE(&mMediusWorldId, sizeof(mMediusWorldId)), mHost);
    mParams.Load(&in);
    in.ReadLE(&mStatus, sizeof(mStatus));
}

// NTSC-U/C: 0x00187c60, PAL: 0x0018d4e8
void GameInfo::Print(std::ostream &stream) {
    std::ostream &out = stream << "addr=" << "mediusWorldID=" << mMediusWorldId
                               << " host=" << mHost;
    mParams.Print(out);
    out << " status=" << mStatus;
}
