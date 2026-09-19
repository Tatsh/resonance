#include "game/nullplayer.h"

// 0x00133530
int NullPlayer::IsNull() {
    return 1;
}

// 0x00133528
void NullPlayer::HandleMessage(Message *) {
}
