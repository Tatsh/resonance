#include "msg/playbacktogglemsg.h"

// 0x003d7248
Message *PlaybackToggleMsg::New() {
    return new PlaybackToggleMsg;
}

// 0x001161d0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PlaybackToggleMsg::Clone() {
    return new PlaybackToggleMsg(*this);
}

// 0x00116218
int PlaybackToggleMsg::Type() {
    return g_nPlaybackToggleMsgType;
}

// 0x00116228
const char *PlaybackToggleMsg::Name() {
    return "PlaybackToggleMsg";
}
