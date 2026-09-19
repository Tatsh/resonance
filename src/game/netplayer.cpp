#include "game/netplayer.h"

// 0x00125a98
//
// Every instruction of the routine is the inlined base destructor, restoring the three base
// tables and releasing the pointer Player declares, so the original body is empty.
NetPlayer::~NetPlayer() {
}

// 0x00125c48
int NetPlayer::Slot4() {
    return mUnknown48;
}

// 0x00125c50
int NetPlayer::Slot5() {
    return mUnknown4c;
}
