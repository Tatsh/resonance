#include "msg/trackselectmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d6e90, PAL: 0x0040ed80
Message *TrackSelectMsg::New() {
    return new TrackSelectMsg;
}

// NTSC-U/C: 0x003dc930, PAL: 0x00414d68
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *TrackSelectMsg::Clone() {
    return new TrackSelectMsg(*this);
}

// NTSC-U/C: 0x003dc990, PAL: 0x00414dc8
int TrackSelectMsg::Type() {
    return g_dwTrackSelectMsgType;
}

// NTSC-U/C: 0x003dc9a0, PAL: 0x00414dd8
const char *TrackSelectMsg::Name() {
    return "TrackSelectMsg";
}

// NTSC-U/C: 0x003e3ae8, PAL: 0x00410358
// The colour name is copied into a temporary before it is written.
void TrackSelectMsg::Print(std::ostream &stream) {
    mPosition.Print(stream << HxStr(mPlayer->mColorName) << " tr#" << mTrack << "/" << mPlace
                           << " ");
}
