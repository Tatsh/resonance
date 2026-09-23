#include "msg/playbackmodemsg.h"

// 0x003d69a8
Message *PlaybackModeMsg::New() {
    return new PlaybackModeMsg;
}

// 0x0011d578
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PlaybackModeMsg::Clone() {
    return new PlaybackModeMsg(*this);
}

// 0x0011d5c8
int PlaybackModeMsg::Type() {
    return g_nPlaybackModeMsgType;
}

// 0x0011d5d8
const char *PlaybackModeMsg::Name() {
    return "PlaybackModeMsg";
}
