#include "msg/remotetrackselectmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d6ed0, PAL: 0x0040edc0
Message *RemoteTrackSelectMsg::New() {
    return new RemoteTrackSelectMsg;
}

// NTSC-U/C: 0x003dcb08, PAL: 0x00414f40
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RemoteTrackSelectMsg::Clone() {
    return new RemoteTrackSelectMsg(*this);
}

// NTSC-U/C: 0x003dcb68, PAL: 0x00414fa0
int RemoteTrackSelectMsg::Type() {
    return g_nRemoteTrackSelectMsgType;
}

// NTSC-U/C: 0x003dcb78, PAL: 0x00414fb0
const char *RemoteTrackSelectMsg::Name() {
    return "RemoteTrackSelectMsg";
}

// NTSC-U/C: 0x003e3bd0, PAL: 0x00410460
// The colour name is copied into a temporary before it is written.
void RemoteTrackSelectMsg::Print(std::ostream &stream) {
    mPosition.Print(stream << HxStr(mPlayer->mColorName) << " tr#" << mTrack << "/" << mPlace
                           << " ");
}
