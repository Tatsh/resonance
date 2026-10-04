#include "msg/playbackmodemsg.h"

Message *PlaybackModeMsg::New() {
    return new PlaybackModeMsg;
}

Message *PlaybackModeMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PlaybackModeMsg(*this);
}

int PlaybackModeMsg::Type() {
    return sID;
}

const char *PlaybackModeMsg::GetName() const {
    return "PlaybackModeMsg";
}
