#include "msg/trackselectmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *TrackSelectMsg::New() {
    return new TrackSelectMsg;
}

Message *TrackSelectMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new TrackSelectMsg(*this);
}

int TrackSelectMsg::Type() {
    return g_dwTrackSelectMsgType;
}

const char *TrackSelectMsg::GetName() const {
    return "TrackSelectMsg";
}

void TrackSelectMsg::PrintExtra(std::ostream &stream) const {
    // The colour name is copied into a temporary before it is written.
    mPosition.Print(stream << HxStr(mPlayer->mColorName) << " tr#" << mTrack << "/" << mPlace
                           << " ");
}
