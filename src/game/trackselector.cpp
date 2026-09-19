#include "game/trackselector.h"

#include <vector>

namespace {

// The answer Player::Slot4() reports for a player that occupies no channel.
constexpr int kNoChannel = -1;

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
