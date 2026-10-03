#include "game/netplayer.h"

#include "msg/remotetrackselectmsg.h"
#include "msg/trackselectmsg.h"
#include "msg/trackselectpacket.h"

// NTSC-U/C: 0x00122f10, PAL: 0x00123540
NetPlayer::NetPlayer(int nId, int nTrack, const HxStr &name, const FreqAppearance *pAppearance)
    : Player(nId, name, pAppearance), mTrack(nTrack), mPlace(0) {
}

// NTSC-U/C: 0x00125a98, PAL: 0x00126110
//
// Every instruction of the routine is the inlined base destructor, restoring the three base
// tables and releasing the pointer Player declares, so the original body is empty.
NetPlayer::~NetPlayer() {
}

// NTSC-U/C: 0x00125c48, PAL: 0x001262d0
int NetPlayer::GetTrack() {
    return mTrack;
}

// NTSC-U/C: 0x00125c50, PAL: 0x001262d8
int NetPlayer::GetPlace() {
    return mPlace;
}

// NTSC-U/C: 0x00122f78, PAL: 0x001235a8
void NetPlayer::OnTrackSelectPacket(TrackSelectPacket *pPacket) {
    if (pPacket->mPlayer != this) {
        return;
    }

    RemoteTrackSelectMsg message;
    message.mTrack = pPacket->mTrack;
    message.mPlace = pPacket->mPlace;
    message.mPosition = pPacket->mPosition;
    message.mPlayer = pPacket->mPlayer;
    Send(&message);
}

// NTSC-U/C: 0x00125f70, PAL: 0x00126608
void NetPlayer::HandleMessage(Message *message) {
    const int nType = message->Type();
    if (nType == g_nTrackSelectPacketType) {
        OnTrackSelectPacket(static_cast<TrackSelectPacket *>(message));
    } else if (static_cast<unsigned int>(nType) == g_dwTrackSelectMsgType) {
        TrackSelectMsg *pSelect = static_cast<TrackSelectMsg *>(message);
        if (pSelect->mPlayer == this) {
            mTrack = pSelect->mTrack;
            mPlace = pSelect->mPlace;
        }
    } else {
        Player::HandleMessage(message);
    }
}
