#include "game/nullplayer.h"

NullPlayer::NullPlayer() : Player(kIDableUnregistered, HxStr(), nullptr) {
}

// 0x00133530
int NullPlayer::IsNull() {
    return 1;
}

// 0x00133528
void NullPlayer::HandleMessage(Message *) {
}

NullPlayer g_nullPlayer;
