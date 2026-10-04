#include "msg/remotetrackselectmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *RemoteTrackSelectMsg::New() {
    return new RemoteTrackSelectMsg;
}

Message *RemoteTrackSelectMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new RemoteTrackSelectMsg(*this);
}

int RemoteTrackSelectMsg::Type() {
    return g_nRemoteTrackSelectMsgType;
}

const char *RemoteTrackSelectMsg::GetName() const {
    return "RemoteTrackSelectMsg";
}

void RemoteTrackSelectMsg::PrintExtra(std::ostream &stream) const {
    // The colour name is copied into a temporary before it is written.
    mPosition.Print(stream << HxStr(mPlayer->mColorName) << " tr#" << mTrack << "/" << mPlace
                           << " ");
}
