#include "game/nullplayer.h"

namespace {

// The colour name the stand-in player reports.
constexpr char kNullColorName[] = "null";

} // namespace

// The static initialiser at 0x00132618 constructs the stand-in inline.
NullPlayer::NullPlayer() : Player(kIDableUnregistered, HxStr(kNullColorName), nullptr) {
}

// 0x00133530
int NullPlayer::IsNull() {
    return 1;
}

// 0x00133528
void NullPlayer::HandleMessage(Message *) {
}

// 0x0066f930
NullPlayer g_nullPlayer;
