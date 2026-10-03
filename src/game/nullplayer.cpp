#include "game/nullplayer.h"

namespace {

// The colour name the stand-in player reports.
constexpr char kNullColorName[] = "null";

} // namespace

// The static initialiser at 0x00132618 constructs the stand-in inline.
NullPlayer::NullPlayer() : Player(kIDableUnregistered, HxStr(kNullColorName), nullptr) {
}

// NTSC-U/C: 0x00133530, PAL: 0x00133d98
int NullPlayer::IsNull() {
    return 1;
}

// NTSC-U/C: 0x00133528, PAL: 0x00133d90
void NullPlayer::DispatchPriv(Message *) {
}

// NTSC-U/C: 0x0066f930, PAL: 0x006b0520
NullPlayer NullPlayer::sInstance;
