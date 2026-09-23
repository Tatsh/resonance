#include "game/trackselector.h"

#include <algorithm>
#include <vector>

#include "app/hudutil.h"
#include "game/localplayer.h"
#include "mid/mbt.h"
#include "msg/deployedpowerupmsg.h"
#include "msg/phrasemuffedmsg.h"
#include "msg/remotetrackselectmsg.h"
#include "msg/rotleftmsg.h"
#include "msg/rotrightmsg.h"
#include "msg/trackselectmsg.h"
#include "os/hxstr.h"
#include "script/testregistry.h"

namespace {

// The answer Player::Slot4() reports for a player that occupies no channel.
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
struct SelfTestRegistration {
    SelfTestRegistration() {
        TestRegistry::Register(kTestName, TrackSelector::RunSelfTest);
    }
};
const SelfTestRegistration sSelfTestRegistration;

// Queries a player's channel and step, discarding both answers.
void ProbePlayer(Player *pPlayer) {
    pPlayer->Slot4();
    pPlayer->Slot5();
}

} // namespace

// 0x0013b250
TrackSelector::TrackSelector(const std::vector<Player *> &players)
    : mChannelCount(kTrackSelectorChannelCount), mSlotCount(players.size()) {
    for (int nChannel = 0; nChannel < kTrackSelectorChannelCount; ++nChannel) {
        for (int nSlot = 0; nSlot < mSlotCount; ++nSlot) {
            mGrid[nChannel][nSlot] = &g_nullPlayer;
        }
    }
    for (unsigned nIndex = 0; nIndex < players.size(); ++nIndex) {
        const int nChannel = players[nIndex]->Slot4();

        if (nChannel != kNoChannel) {
            InsertLightForDrawable(players[nIndex], nChannel, 0);
        }
    }
}

// 0x0013b480
void TrackSelector::RemoveLightFromColumn(Player *pPlayer, int nChannel, int nPayload) {
    if (nChannel == kNoChannel) {
        return;
    }

    int nFound = 0;
    while (nFound < mSlotCount && mGrid[nChannel][nFound] != pPlayer) {
        ++nFound;
    }
    for (int nSlot = nFound; nSlot < mSlotCount; ++nSlot) {
        Player *pNext = (nSlot + 1 == mSlotCount) ? static_cast<Player *>(&g_nullPlayer) :
                                                    mGrid[nChannel][nSlot + 1];
        if (pNext != mGrid[nChannel][nSlot]) {
            TrackSelectMsg message;
            message.mUnknown04 = nChannel;
            message.mUnknown08 = nSlot;
            message.mPosition = Mid::MBT(nPayload);
            message.mUnknown10 = pNext;
            Send(&message);
            mGrid[nChannel][nSlot] = pNext;
        }
    }
}

// 0x0013b5e8
void TrackSelector::InsertLightForDrawable(Player *pPlayer, int nChannel, int nPayload) {
    for (int nSlot = 0; nSlot < mSlotCount; ++nSlot) {
        if (mGrid[nChannel][nSlot] == &g_nullPlayer) {
            mGrid[nChannel][nSlot] = pPlayer;
            TrackSelectMsg message;
            message.mUnknown04 = nChannel;
            message.mUnknown08 = nSlot;
            message.mPosition = Mid::MBT(nPayload);
            message.mUnknown10 = pPlayer;
            Send(&message);
            return;
        }
    }
}

// 0x0013b6a8
int TrackSelector::RebuildChannelGrid(BumpPacket *pPacket) {
    const int nChannel = pPacket->mTrack;
    Player *pPlayer = pPacket->mPlayer;
    const Mid::MBT position(
        std::min(std::max(pPacket->mBar * Mid::MBT(kTicksPerBar).mTick, kMBTMinimum), kMBTMaximum));
    if (pPlayer->Slot5() == 0) {
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
    while (pPlayer->Slot5() != 0) {
        RebindLightIfChanged(mGrid[nChannel][0], nChannel, position.mTick);
    }
    pPacket->mResult = 1;
    return 1;
}

// 0x0013f5a0, the out-of-line copy.
inline void TrackSelector::OnRotLeft(RotLeftMsg *pMsg) {
    Player *pPlayer = pMsg->mPlayer;
    if (pPlayer->HasInputSlot()) {
        AddLightToChannel(pPlayer, pMsg->mPosition.mTick, kRotateDown);
    }
}

// 0x0013f608, the out-of-line copy.
inline void TrackSelector::OnRotRight(RotRightMsg *pMsg) {
    Player *pPlayer = pMsg->mPlayer;
    if (pPlayer->HasInputSlot()) {
        AddLightToChannel(pPlayer, pMsg->mPosition.mTick, kRotateUp);
    }
}

// 0x0013f670, the out-of-line copy.
inline void TrackSelector::OnPhraseMuffed(PhraseMuffedMsg *pMsg) {
    Player *pPlayer = pMsg->mPlayer;
    if (pPlayer->HasInputSlot()) {
        pPlayer->Handle(pMsg);
    }
}

// 0x0013f6d8, the out-of-line copy.
inline void TrackSelector::OnRemoteTrackSelect(RemoteTrackSelectMsg *pMsg) {
    Player *pPlayer = pMsg->mPlayer;
    RebindLightColumn(pPlayer, pPlayer->Slot4(), pMsg->mUnknown04, pMsg->mPosition.mTick);
}

// 0x0013b868
void TrackSelector::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nRotLeftMsgType) {
        OnRotLeft(static_cast<RotLeftMsg *>(pMsg));
    } else if (nType == g_nRotRightMsgType) {
        OnRotRight(static_cast<RotRightMsg *>(pMsg));
    } else if (nType == g_nPhraseMuffedMsgType) {
        OnPhraseMuffed(static_cast<PhraseMuffedMsg *>(pMsg));
    } else if (nType == g_nBumpPacketType) {
        RebuildChannelGrid(static_cast<BumpPacket *>(pMsg));
    } else if (nType == g_nRemoteTrackSelectMsgType) {
        OnRemoteTrackSelect(static_cast<RemoteTrackSelectMsg *>(pMsg));
    }
}

// 0x0013ba08
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

    selector.RebindLightColumn(
        players[kTestFourthPlayer], kTestFourthPlayer, kTestHomeChannel, Mid::MBT(kTestTick).mTick);
    // Yes, the binary keeps an empty loop here.
    for (int nSlot = kTrackSelectorSlotCount - 1; nSlot >= 0; --nSlot) {
    }
    ProbePlayer(players[kTestFourthPlayer]);

    selector.RebindLightColumn(
        players[kTestThirdPlayer], kTestThirdPlayer, kTestHomeChannel, Mid::MBT(kTestTick).mTick);
    // Yes, the binary keeps an empty loop here.
    for (int nSlot = kTrackSelectorSlotCount - 1; nSlot >= 0; --nSlot) {
    }
    ProbePlayer(players[kTestFirstPlayer]);
    ProbePlayer(players[kTestFourthPlayer]);
    ProbePlayer(players[kTestThirdPlayer]);

    selector.RebindLightIfChanged(
        players[kTestThirdPlayer], kTestHomeChannel, Mid::MBT(kTestTick).mTick);
    ProbePlayer(players[kTestFirstPlayer]);
    ProbePlayer(players[kTestFourthPlayer]);
    ProbePlayer(players[kTestThirdPlayer]);

    selector.RebindLightIfChanged(
        players[kTestFourthPlayer], kTestHomeChannel, Mid::MBT(kTestTick).mTick);
    ProbePlayer(players[kTestFirstPlayer]);
    ProbePlayer(players[kTestThirdPlayer]);
    ProbePlayer(players[kTestFourthPlayer]);

    selector.RebindLightIfChanged(
        players[kTestFirstPlayer], kTestHomeChannel, Mid::MBT(kTestTick).mTick);
    ProbePlayer(players[kTestThirdPlayer]);
    ProbePlayer(players[kTestFourthPlayer]);
    ProbePlayer(players[kTestFirstPlayer]);

    // Yes, the binary never deletes the players.
    return 1;
}

// 0x0013f8e8
void TrackSelector::RunSelfTest() {
    SelfTest();
}

// 0x0013f020
TrackSelector::~TrackSelector() {
}

// 0x0013f748
void TrackSelector::RebindLightColumn(Player *pPlayer,
                                      int nFromChannel,
                                      int nToChannel,
                                      int nPayload) {
    RemoveLightFromColumn(pPlayer, nFromChannel, nPayload);
    InsertLightForDrawable(pPlayer, nToChannel, nPayload);
}

// 0x0013f7a8
void TrackSelector::RebindLightIfChanged(Player *pPlayer, int nChannel, int nPayload) {
    if (mGrid[nChannel][0] == pPlayer && mGrid[nChannel][1] == &g_nullPlayer) {
        return;
    }
    RemoveLightFromColumn(pPlayer, nChannel, nPayload);
    InsertLightForDrawable(pPlayer, nChannel, nPayload);
}

// 0x0013f840
int TrackSelector::AddLightToChannel(Player *pPlayer, int nPayload, int nDelta) {
    const int nChannel = pPlayer->Slot4();
    const int nTarget = (nChannel + nDelta + mChannelCount) % mChannelCount;

    RemoveLightFromColumn(pPlayer, nChannel, nPayload);
    InsertLightForDrawable(pPlayer, nTarget, nPayload);
    return 1;
}
