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

Player::Player(int nId, const HxStr &colorName, const FreqAppearance *pAppearance)
    : IDable<Player>(nId), mPlayerId(nId), mColorName(colorName), mAppearance(pAppearance),
      mJuice(0), mMaxJuice(kInitialCeiling), mScore(0), mMaxScore(kInitialCeiling),
      mLastEraseTime(0) {
}

int Player::GetInputSlot() const {
    return -1;
}

int Player::IsNull() {
    return 0;
}

int Player::GetTrack() {
    return -1;
}

int Player::GetPlace() {
    return 0;
}

int Player::UnusedQuery() {
    return 0;
}

void Player::UnusedHook() {
}

void Player::SetFreestyleSpan(int, int) {
}

int Player::IsFreestyleBar(int) {
    return 0;
}

int Player::IsLooping() {
    return 0;
}

void Player::AnnounceState() {
    JuiceAmountMsg message;
    message.mPlayer = this;
    message.mMaxJuice = mMaxJuice < kJuiceMaximum ? mMaxJuice : kJuiceMaximum;

    Send(&message);
}

void Player::DeactivatePlacer() {
}

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

int Player::CountCaughtGem() {
    // The image leaves the return register untouched here, so the value is indeterminate.
    return 0;
}

int Player::CountMissedGem() {
    // The image leaves the return register untouched here, so the value is indeterminate.
    return 0;
}

int Player::GetMultiplier(int) {
    return 1;
}

int Player::GetBestStreak() {
    return 0;
}

float Player::GetCaptureRatio() {
    return 0.0f;
}

int Player::GetGameMode() {
    return 0;
}

int Player::MarkBarScored(int) {
    return 1;
}

int Player::GetScore() {
    return mScore;
}

int Player::GetJuice() {
    return mJuice;
}

void Player::SetScore(int nScore, int nMaxScore) {
    mMaxScore = nMaxScore;
    mScore = nScore;
}

void Player::SetJuice(int nJuice, int nMaxJuice) {
    mMaxJuice = nMaxJuice;
    mJuice = nJuice;
}

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

void Player::OnMsg(const PhraseCapturedMsg &msg) {
    AddScore(msg.mScore, 1);
    AddJuice(msg.mJuice, 1);
}

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

Player::~Player() {
}

inline HxStr Player::GetColorName() {
    return mColorName;
}

inline HxStr Player::GetUsername() {
    return mAppearance->mUserName;
}

int Player::StartMF() {
    AnnounceState();
    return 0;
}

int Player::StopMF() {
    DeactivatePlacer();
    return 0;
}

inline void Player::OnUpdateScore(UpdateScorePacket *pPacket) {
    if (pPacket->mPlayerId == mPlayerId) {
        AddScore(pPacket->mScoreDelta, 0);
    }
}

void Player::DispatchPriv(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nUpdateScorePacketType) {
        OnUpdateScore(static_cast<UpdateScorePacket *>(pMsg));
    } else if (nType == g_nPhraseCapturedMsgType) {
        // Awarded whichever player the capture names.
        OnMsg(*static_cast<PhraseCapturedMsg *>(pMsg));
    }
}
