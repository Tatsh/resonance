#include "msg/playbackmodemsg.h"

// NTSC-U/C: 0x003d69a8, PAL: 0x0040e898
Message *PlaybackModeMsg::New() {
    return new PlaybackModeMsg;
}

// NTSC-U/C: 0x0011d578, PAL: 0x0011db00
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PlaybackModeMsg::Clone() {
    return new PlaybackModeMsg(*this);
}

// NTSC-U/C: 0x0011d5c8, PAL: 0x0011db50
int PlaybackModeMsg::Type() {
    return sID;
}

// NTSC-U/C: 0x0011d5d8, PAL: 0x0011db60
const char *PlaybackModeMsg::GetName() const {
    return "PlaybackModeMsg";
}
