#include "msg/playbacktogglemsg.h"

// NTSC-U/C: 0x003d7248, PAL: 0x0040f148
Message *PlaybackToggleMsg::New() {
    return new PlaybackToggleMsg;
}

// NTSC-U/C: 0x001161d0, PAL: 0x00116678
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PlaybackToggleMsg::Clone() {
    return new PlaybackToggleMsg(*this);
}

// NTSC-U/C: 0x00116218, PAL: 0x001166c0
int PlaybackToggleMsg::Type() {
    return g_nPlaybackToggleMsgType;
}

// NTSC-U/C: 0x00116228, PAL: 0x001166d0
const char *PlaybackToggleMsg::Name() {
    return "PlaybackToggleMsg";
}
