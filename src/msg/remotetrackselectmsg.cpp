#include "msg/remotetrackselectmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d6ed0
Message *RemoteTrackSelectMsg::New() {
    return new RemoteTrackSelectMsg;
}

// 0x003dcb08
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RemoteTrackSelectMsg::Clone() {
    return new RemoteTrackSelectMsg(*this);
}

// 0x003dcb68
int RemoteTrackSelectMsg::Type() {
    return g_nRemoteTrackSelectMsgType;
}

// 0x003dcb78
const char *RemoteTrackSelectMsg::Name() {
    return "RemoteTrackSelectMsg";
}

// 0x003e3bd0
// The colour name is copied into a temporary before it is written.
void RemoteTrackSelectMsg::Print(std::ostream &stream) {
    mPosition.Print(stream << HxStr(mPlayer->mColorName) << " tr#" << mUnknown04 << "/"
                           << mUnknown08 << " ");
}
