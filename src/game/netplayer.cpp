#include "game/netplayer.h"

#include "msg/remotetrackselectmsg.h"
#include "msg/trackselectmsg.h"
#include "msg/trackselectpacket.h"

NetPlayer::NetPlayer(int nId, int nTrack, const HxStr &name, const FreqAppearance *pAppearance)
    : Player(nId, name, pAppearance), mTrack(nTrack), mPlace(0) {
}

NetPlayer::~NetPlayer() {
    // Every instruction of the routine is the inlined base destructor, restoring the three base
    // tables and releasing the pointer Player declares.
}

int NetPlayer::GetTrack() {
    return mTrack;
}

int NetPlayer::GetPlace() {
    return mPlace;
}

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

void NetPlayer::DispatchPriv(Message *message) {
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
        Player::DispatchPriv(message);
    }
}
