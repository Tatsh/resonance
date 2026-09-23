#include "msg/trackselectmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d6e90
Message *TrackSelectMsg::New() {
    return new TrackSelectMsg;
}

// 0x003dc930
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *TrackSelectMsg::Clone() {
    return new TrackSelectMsg(*this);
}

// 0x003dc990
int TrackSelectMsg::Type() {
    return g_dwTrackSelectMsgType;
}

// 0x003dc9a0
const char *TrackSelectMsg::Name() {
    return "TrackSelectMsg";
}

// 0x003e3ae8
// The colour name is copied into a temporary before it is written.
void TrackSelectMsg::Print(std::ostream &stream) {
    mPosition.Print(stream << HxStr(mUnknown10->mColorName) << " tr#" << mUnknown04 << "/"
                           << mUnknown08 << " ");
}
