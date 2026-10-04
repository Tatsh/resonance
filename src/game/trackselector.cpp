#include "game/trackselector.h"

#include <algorithm>
#include <vector>

#include "app/hudutil.h"
#include "game/localplayer.h"
#include "mid/tick.h"
#include "msg/deployedpowerupmsg.h"
#include "msg/phrasemuffedmsg.h"
#include "msg/remotetrackselectmsg.h"
#include "msg/rotleftmsg.h"
#include "msg/rotrightmsg.h"
#include "msg/trackselectmsg.h"
#include "os/hxstr.h"
#include "script/testregistry.h"

namespace {

// The answer Player::GetTrack() reports for a player that occupies no channel.
constexpr int kNoChannel = -1;

// MIDI ticks in one bar.
constexpr int kTicksPerBar = 1920;

// The channel steps the two rotation messages apply.
constexpr int kRotateDown = -1;
constexpr int kRotateUp = 1;

// The name SelfTest() registers under, and the colour name it gives its players.
static const char *const kTestName = "TrackSelector";
static const char *const kTestColorName = "null";
// The players SelfTest() builds, and the position every one of its rebinds uses.
constexpr int kTestPlayerCount = 4;
constexpr int kTestTick = 0;
// The roster indices SelfTest() moves, and the channels it moves them between.
constexpr int kTestFirstPlayer = 0;
constexpr int kTestThirdPlayer = 2;
constexpr int kTestFourthPlayer = 3;
constexpr int kTestHomeChannel = 0;

// Registers the self-test from the unit's static initialiser.
class SelfTestRegistration {
public:
    SelfTestRegistration() {
        TestRegistry::Register(kTestName, TrackSelector::RunSelfTest);
    }
};
const SelfTestRegistration sSelfTestRegistration;

// Queries a player's channel and step, discarding both answers.
void ProbePlayer(Player *pPlayer) {
    pPlayer->GetTrack();
    pPlayer->GetPlace();
}

} // namespace

TrackSelector::TrackSelector(const std::vector<Player *> &players)
    : mChannelCount(kTrackSelectorChannelCount), mSlotCount(players.size()) {
    for (int nChannel = 0; nChannel < kTrackSelectorChannelCount; ++nChannel) {
        for (int nSlot = 0; nSlot < mSlotCount; ++nSlot) {
            mGrid[nChannel][nSlot] = &NullPlayer::sInstance;
        }
    }
    for (unsigned nIndex = 0; nIndex < players.size(); ++nIndex) {
        const int nChannel = players[nIndex]->GetTrack();

        if (nChannel != kNoChannel) {
            InsertLightForDrawable(players[nIndex], nChannel, 0);
        }
    }
}

void TrackSelector::RemoveLightFromColumn(Player *pPlayer, int nChannel, int nPayload) {
    if (nChannel == kNoChannel) {
        return;
    }

    int nFound = 0;
    while (nFound < mSlotCount && mGrid[nChannel][nFound] != pPlayer) {
        ++nFound;
    }
    for (int nSlot = nFound; nSlot < mSlotCount; ++nSlot) {
        Player *pNext = (nSlot + 1 == mSlotCount) ? static_cast<Player *>(&NullPlayer::sInstance) :
                                                    mGrid[nChannel][nSlot + 1];
        if (pNext != mGrid[nChannel][nSlot]) {
            TrackSelectMsg message;
            message.mTrack = nChannel;
            message.mPlace = nSlot;
            message.mPosition = Sch::Tick(nPayload);
            message.mPlayer = pNext;
            Send(&message);
            mGrid[nChannel][nSlot] = pNext;
        }
    }
}

void TrackSelector::InsertLightForDrawable(Player *pPlayer, int nChannel, int nPayload) {
    for (int nSlot = 0; nSlot < mSlotCount; ++nSlot) {
        if (mGrid[nChannel][nSlot] == &NullPlayer::sInstance) {
            mGrid[nChannel][nSlot] = pPlayer;
            TrackSelectMsg message;
            message.mTrack = nChannel;
            message.mPlace = nSlot;
            message.mPosition = Sch::Tick(nPayload);
            message.mPlayer = pPlayer;
            Send(&message);
            return;
        }
    }
}

int TrackSelector::RebuildChannelGrid(BumpPacket *pPacket) {
    const int nChannel = pPacket->mTrack;
    Player *pPlayer = pPacket->mPlayer;
    const Sch::Tick position(std::min(
        std::max(pPacket->mBar * Sch::Tick(kTicksPerBar).mTick, kTickMinimum), kTickMaximum));
    if (pPlayer->GetPlace() == 0) {
        return 0;
    }

    {
        DeployedPowerupMsg message;
        message.mKind = kHudItemBumper;
        message.mPlayer = pPlayer;
        message.mTarget = mGrid[nChannel][0];
        message.mFirstBar = 0;
        message.mBarCount = 0;
        message.mTrack = nChannel;
        Send(&message);
    }
    while (pPlayer->GetPlace() != 0) {
        MovePlayerToBack(mGrid[nChannel][0], nChannel, position.mTick);
    }
    pPacket->mResult = 1;
    return 1;
}

inline void TrackSelector::OnMsg(const RotLeftMsg &msg) {
    Player *pPlayer = msg.mPlayer;
    if (pPlayer->IsLocal()) {
        AddLightToChannel(pPlayer, msg.mPosition.mTick, kRotateDown);
    }
}

inline void TrackSelector::OnMsg(const RotRightMsg &msg) {
    Player *pPlayer = msg.mPlayer;
    if (pPlayer->IsLocal()) {
        AddLightToChannel(pPlayer, msg.mPosition.mTick, kRotateUp);
    }
}

inline void TrackSelector::OnPhraseMuffed(PhraseMuffedMsg *pMsg) {
    Player *pPlayer = pMsg->mPlayer;
    if (pPlayer->IsLocal()) {
        pPlayer->Dispatch(pMsg);
    }
}

inline void TrackSelector::OnRemoteTrackSelect(RemoteTrackSelectMsg *pMsg) {
    Player *pPlayer = pMsg->mPlayer;
    MovePlayer(pPlayer, pPlayer->GetTrack(), pMsg->mTrack, pMsg->mPosition.mTick);
}

void TrackSelector::DispatchPriv(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nRotLeftMsgType) {
        OnMsg(*static_cast<RotLeftMsg *>(pMsg));
    } else if (nType == g_nRotRightMsgType) {
        OnMsg(*static_cast<RotRightMsg *>(pMsg));
    } else if (nType == g_nPhraseMuffedMsgType) {
        OnPhraseMuffed(static_cast<PhraseMuffedMsg *>(pMsg));
    } else if (nType == g_nBumpPacketType) {
        RebuildChannelGrid(static_cast<BumpPacket *>(pMsg));
    } else if (nType == g_nRemoteTrackSelectMsgType) {
        OnRemoteTrackSelect(static_cast<RemoteTrackSelectMsg *>(pMsg));
    }
}

int TrackSelector::SelfTest() {
    std::vector<Player *> players;
    for (int nIndex = 0; nIndex < kTestPlayerCount; ++nIndex) {
        players.push_back(
            new LocalPlayer(nIndex, nIndex, HxStr(kTestColorName), nullptr, nullptr, nIndex));
    }

    TrackSelector selector(players);
    for (int nIndex = 0; nIndex < kTestPlayerCount; ++nIndex) {
        selector.AddSink(players[nIndex]);
    }
    for (int nIndex = 0; nIndex < kTestPlayerCount; ++nIndex) {
        ProbePlayer(players[nIndex]);
    }

    selector.MovePlayer(players[kTestFourthPlayer],
                        kTestFourthPlayer,
                        kTestHomeChannel,
                        Sch::Tick(kTestTick).mTick);
    // Yes, the binary keeps an empty loop here.
    for (int nSlot = kTrackSelectorSlotCount - 1; nSlot >= 0; --nSlot) {
    }
    ProbePlayer(players[kTestFourthPlayer]);

    selector.MovePlayer(
        players[kTestThirdPlayer], kTestThirdPlayer, kTestHomeChannel, Sch::Tick(kTestTick).mTick);
    // Yes, the binary keeps an empty loop here.
    for (int nSlot = kTrackSelectorSlotCount - 1; nSlot >= 0; --nSlot) {
    }
    ProbePlayer(players[kTestFirstPlayer]);
    ProbePlayer(players[kTestFourthPlayer]);
    ProbePlayer(players[kTestThirdPlayer]);

    selector.MovePlayerToBack(
        players[kTestThirdPlayer], kTestHomeChannel, Sch::Tick(kTestTick).mTick);
    ProbePlayer(players[kTestFirstPlayer]);
    ProbePlayer(players[kTestFourthPlayer]);
    ProbePlayer(players[kTestThirdPlayer]);

    selector.MovePlayerToBack(
        players[kTestFourthPlayer], kTestHomeChannel, Sch::Tick(kTestTick).mTick);
    ProbePlayer(players[kTestFirstPlayer]);
    ProbePlayer(players[kTestThirdPlayer]);
    ProbePlayer(players[kTestFourthPlayer]);

    selector.MovePlayerToBack(
        players[kTestFirstPlayer], kTestHomeChannel, Sch::Tick(kTestTick).mTick);
    ProbePlayer(players[kTestThirdPlayer]);
    ProbePlayer(players[kTestFourthPlayer]);
    ProbePlayer(players[kTestFirstPlayer]);

    // Yes, the binary never deletes the players.
    return 1;
}

int TrackSelector::RunSelfTest() {
    return SelfTest();
}

TrackSelector::~TrackSelector() {
}

void TrackSelector::MovePlayer(Player *pPlayer, int nFromChannel, int nToChannel, int nPayload) {
    RemoveLightFromColumn(pPlayer, nFromChannel, nPayload);
    InsertLightForDrawable(pPlayer, nToChannel, nPayload);
}

void TrackSelector::MovePlayerToBack(Player *pPlayer, int nChannel, int nPayload) {
    if (mGrid[nChannel][0] == pPlayer && mGrid[nChannel][1] == &NullPlayer::sInstance) {
        return;
    }
    RemoveLightFromColumn(pPlayer, nChannel, nPayload);
    InsertLightForDrawable(pPlayer, nChannel, nPayload);
}

int TrackSelector::AddLightToChannel(Player *pPlayer, int nPayload, int nDelta) {
    const int nChannel = pPlayer->GetTrack();
    const int nTarget = (nChannel + nDelta + mChannelCount) % mChannelCount;

    RemoveLightFromColumn(pPlayer, nChannel, nPayload);
    InsertLightForDrawable(pPlayer, nTarget, nPayload);
    return 1;
}
