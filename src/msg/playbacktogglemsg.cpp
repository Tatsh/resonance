#include "msg/playbacktogglemsg.h"

Message *PlaybackToggleMsg::New() {
    return new PlaybackToggleMsg;
}

Message *PlaybackToggleMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PlaybackToggleMsg(*this);
}

int PlaybackToggleMsg::Type() {
    return g_nPlaybackToggleMsgType;
}

const char *PlaybackToggleMsg::GetName() const {
    return "PlaybackToggleMsg";
}
