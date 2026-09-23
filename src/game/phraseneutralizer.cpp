#include "game/phraseneutralizer.h"

#include "app/hudutil.h"
#include "game/player.h"
#include "msg/deployedpowerupmsg.h"
#include "msg/playerstrackneutralizedmsg.h"

namespace {

// A neutraliser clears this many bars, starting at the bar after the message's.
constexpr int kNeutralizedBars = 4;

// Every table indexed by Player::mId20 has one slot per player.
constexpr int kMaxPlayers = 4;

// The flags the owner's score change and the phrase clear pass.
constexpr int kNotifyScore = 1;
constexpr int kClearAll = 1;

// The result written back into a NeutralizeMsg that cleared a phrase.
constexpr int kNeutralizeHandled = 1;

} // namespace

// 0x001c0918
PhraseNeutralizer::PhraseNeutralizer(const TrackData *pTrackData, PhraseMgr *pPhraseMgr)
    : mTrack(pTrackData->mUnknown04), mPhraseMgr(pPhraseMgr), mTrackData(pTrackData) {
}

// 0x001c0980
void PhraseNeutralizer::PostTrackNeutralizedMsg(NeutralizeMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }

    int anLost[kMaxPlayers]{};
    Player *apLosers[kMaxPlayers]{};
    bool bCleared = false;
    const int nFirstBar = pMsg->mBar + 1;
    const int nEndBar = nFirstBar + kNeutralizedBars;
    for (int nBar = nFirstBar; nBar < nEndBar; ++nBar) {
        Player *pOwner = mPhraseMgr->GetPhraseOwner(nBar);
        if (pOwner->IsNull()) {
            continue;
        }
        bCleared = true;
        const int nValue = mPhraseMgr->GetPhraseByte(nBar);
        pOwner->AddScore(-nValue, kNotifyScore);
        mPhraseMgr->ClearPhrase(nBar, kClearAll);
        anLost[pOwner->mId20] -= nValue;
        apLosers[pOwner->mId20] = pOwner;
    }
    if (!bCleared) {
        return;
    }

    pMsg->mUnknown04 = kNeutralizeHandled;
    DeployedPowerupMsg deployed(
        kHudItemNeutralizer, pMsg->mPlayer, nullptr, nFirstBar, kNeutralizedBars, mTrack);
    Send(&deployed);
    for (int i = 0; i < kMaxPlayers; ++i) {
        if (apLosers[i] != nullptr) {
            PlayersTrackNeutralizedMsg neutralized;
            neutralized.mPoints = anLost[i];
            neutralized.mPlayer = apLosers[i];
            Send(&neutralized);
        }
    }
}

// 0x001c1700
void PhraseNeutralizer::HandleMessage(Message *pMsg) {
    if (pMsg->Type() == g_nNeutralizeMsgType) {
        PostTrackNeutralizedMsg(static_cast<NeutralizeMsg *>(pMsg));
    }
}
