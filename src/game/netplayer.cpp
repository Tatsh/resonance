#include "game/netplayer.h"

#include "msg/remotetrackselectmsg.h"
#include "msg/trackselectmsg.h"
#include "msg/trackselectpacket.h"

// 0x00122f10
NetPlayer::NetPlayer(int nId, int nTrack, const HxStr &name, const FreqAppearance *pAppearance)
    : Player(nId, name, pAppearance), mTrack(nTrack), mPlace(0) {
}

// 0x00125a98
//
// Every instruction of the routine is the inlined base destructor, restoring the three base
// tables and releasing the pointer Player declares, so the original body is empty.
NetPlayer::~NetPlayer() {
}

// 0x00125c48
int NetPlayer::GetTrack() {
    return mTrack;
}

// 0x00125c50
int NetPlayer::GetPlace() {
    return mPlace;
}

// 0x00122f78
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

// 0x00125f70
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
