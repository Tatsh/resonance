#include "game/nullplayer.h"

namespace {

// The colour name the stand-in player reports.
constexpr char kNullColorName[] = "null";

} // namespace

NullPlayer::NullPlayer() : Player(kIDableUnregistered, HxStr(kNullColorName), nullptr) {
    // The static initialiser at 0x00132618 constructs the stand-in inline.
}

int NullPlayer::IsNull() {
    return 1;
}

void NullPlayer::DispatchPriv(Message *) {
}

// NTSC-U/C: 0x0066f930, PAL: 0x006b0520
NullPlayer NullPlayer::sInstance;
