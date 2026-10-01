#include "game/player.h"

#include <algorithm>
#include <iostream>

#include "app/msgsource.h"
#include "game/freqappearance.h"
#include "msg/juiceamountmsg.h"
#include "msg/phrasecapturedmsg.h"
#include "msg/pointamountmsg.h"
#include "msg/updatescorepacket.h"

namespace {

// Ceiling AnnounceState() applies to the value it publishes.
constexpr int kJuiceMaximum = 800;

// The ceiling the constructor gives both the score and the juice.
constexpr int kInitialCeiling = 1;

constexpr char kNullText[] = "{player null}";
constexpr char kOpenText[] = "{player ";
constexpr char kNetText[] = " net}";
constexpr char kLocalText[] = " local}";

} // namespace

// 0x0012f5c0
Player::Player(int nId, const HxStr &colorName, const FreqAppearance *pAppearance)
    : IDable<Player>(nId), mPlayerId(nId), mColorName(colorName), mAppearance(pAppearance),
      mJuice(0), mMaxJuice(kInitialCeiling), mScore(0), mMaxScore(kInitialCeiling),
      mLastEraseTime(0) {
}

// 0x00132c20
int Player::GetInputSlot() {
    return -1;
}

// 0x00132c60
int Player::IsNull() {
    return 0;
}

// 0x00132c98
int Player::GetTrack() {
    return -1;
}

// 0x00132ca0
int Player::GetPlace() {
    return 0;
}

// 0x00132ca8
int Player::UnusedQuery() {
    return 0;
}

// 0x00132cb0
void Player::UnusedHook() {
}

// 0x00132cb8
void Player::SetFreestyleSpan(int, int) {
}

// 0x00132cc0
int Player::IsFreestyleBar(int) {
    return 0;
}

// 0x00132cc8
int Player::IsLooping() {
    return 0;
}

// 0x0012f788
void Player::AnnounceState() {
    JuiceAmountMsg message;
    message.mPlayer = this;
    message.mMaxJuice = mMaxJuice < kJuiceMaximum ? mMaxJuice : kJuiceMaximum;

    Send(&message);
}

// 0x00132d30
void Player::DeactivatePlacer() {
}

// 0x00133110
void Player::Print(std::ostream &stream) {
    if (IsNull() != 0) {
        stream << kNullText;
        return;
    }

    stream << kOpenText << mPlayerId;
    if (GetInputSlot() == kNoInputSlot) {
        stream << kNetText;
    } else {
        stream << kLocalText;
    }
}

// 0x00132d70
int Player::CountCaughtGem() {
    // The image leaves the return register untouched here, so the value is indeterminate.
    return 0;
}

// 0x00132d78
int Player::CountMissedGem() {
    // The image leaves the return register untouched here, so the value is indeterminate.
    return 0;
}

// 0x00132d80
int Player::GetMultiplier(int) {
    return 1;
}

// 0x00132d88
int Player::GetBestStreak() {
    return 0;
}

// 0x00132d90
float Player::GetCaptureRatio() {
    return 0.0f;
}

// 0x00132da0
int Player::GetGameMode() {
    return 0;
}

// 0x00132db0
int Player::MarkBarScored(int) {
    return 1;
}

// 0x001330e0
int Player::GetScore() {
    return mScore;
}

// 0x001330f8
int Player::GetJuice() {
    return mJuice;
}

// 0x001330e8
void Player::SetScore(int nScore, int nMaxScore) {
    mMaxScore = nMaxScore;
    mScore = nScore;
}

// 0x00133100
void Player::SetJuice(int nJuice, int nMaxJuice) {
    mMaxJuice = nMaxJuice;
    mJuice = nJuice;
}

// 0x0012f808
void Player::AddScore(int nDelta, int bNotify) {
    const int nOldScore = mScore;
    mScore = std::max(0, std::min(mScore + nDelta, mMaxScore));
    if (mScore == nOldScore) {
        return;
    }

    PointAmountMsg message;
    message.mPlayer = this;
    // Yes, the binary caps the score ceiling at the juice maximum too.
    message.mMaxScore = std::min(mMaxScore, kJuiceMaximum);
    Send(&message);

    if (bNotify != 0) {
        UpdateScorePacket packet(mPlayerId, nDelta);
        Send(&packet);
    }
}

// 0x001331c8
void Player::AwardCapture(PhraseCapturedMsg *pMsg) {
    AddScore(pMsg->mScore, 1);
    AddJuice(pMsg->mJuice, 1);
}

// 0x0012f970
void Player::AddJuice(int nAmount, int bNotify) {
    const int nOldJuice = mJuice;
    mJuice = std::max(0, std::min(mJuice + nAmount, mMaxJuice));
    if (mJuice == nOldJuice) {
        return;
    }

    JuiceAmountMsg message;
    message.mPlayer = this;
    message.mMaxJuice = std::min(mMaxJuice, kJuiceMaximum);
    Send(&message);

    if (bNotify != 0) {
        UpdateScorePacket packet(mPlayerId, nAmount);
        Send(&packet);
    }
}

// 0x00132ae8
Player::~Player() {
}

// 0x00132c68
// The out-of-line copy.
inline HxStr Player::GetColorName() {
    return mColorName;
}

// 0x001330a8
// The out-of-line copy.
inline HxStr Player::GetUsername() {
    return mAppearance->mUserName;
}

// 0x00132cd0
// The out-of-line copy.
int Player::CallAnnounceState() {
    AnnounceState();
    return 0;
}

// 0x00132d00
// The out-of-line copy.
int Player::CallDeactivatePlacer() {
    DeactivatePlacer();
    return 0;
}

// 0x00133210
// The out-of-line copy.
inline void Player::OnUpdateScore(UpdateScorePacket *pPacket) {
    if (pPacket->mPlayerId == mPlayerId) {
        AddScore(pPacket->mScoreDelta, 0);
    }
}

// 0x00133240
void Player::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nUpdateScorePacketType) {
        OnUpdateScore(static_cast<UpdateScorePacket *>(pMsg));
    } else if (nType == g_nPhraseCapturedMsgType) {
        // Awarded whichever player the capture names.
        AwardCapture(static_cast<PhraseCapturedMsg *>(pMsg));
    }
}
