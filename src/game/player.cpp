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

// NTSC-U/C: 0x0012f5c0, PAL: 0x0012fd78
Player::Player(int nId, const HxStr &colorName, const FreqAppearance *pAppearance)
    : IDable<Player>(nId), mPlayerId(nId), mColorName(colorName), mAppearance(pAppearance),
      mJuice(0), mMaxJuice(kInitialCeiling), mScore(0), mMaxScore(kInitialCeiling),
      mLastEraseTime(0) {
}

// NTSC-U/C: 0x00132c20, PAL: 0x00133460
int Player::GetInputSlot() {
    return -1;
}

// NTSC-U/C: 0x00132c60, PAL: 0x001334a0
int Player::IsNull() {
    return 0;
}

// NTSC-U/C: 0x00132c98, PAL: 0x001334d8
int Player::GetTrack() {
    return -1;
}

// NTSC-U/C: 0x00132ca0, PAL: 0x001334e0
int Player::GetPlace() {
    return 0;
}

// NTSC-U/C: 0x00132ca8, PAL: 0x001334e8
int Player::UnusedQuery() {
    return 0;
}

// NTSC-U/C: 0x00132cb0, PAL: 0x001334f0
void Player::UnusedHook() {
}

// NTSC-U/C: 0x00132cb8, PAL: 0x001334f8
void Player::SetFreestyleSpan(int, int) {
}

// NTSC-U/C: 0x00132cc0, PAL: 0x00133500
int Player::IsFreestyleBar(int) {
    return 0;
}

// NTSC-U/C: 0x00132cc8, PAL: 0x00133508
int Player::IsLooping() {
    return 0;
}

// NTSC-U/C: 0x0012f788, PAL: 0x0012ff40
void Player::AnnounceState() {
    JuiceAmountMsg message;
    message.mPlayer = this;
    message.mMaxJuice = mMaxJuice < kJuiceMaximum ? mMaxJuice : kJuiceMaximum;

    Send(&message);
}

// NTSC-U/C: 0x00132d30, PAL: 0x00133570
void Player::DeactivatePlacer() {
}

// NTSC-U/C: 0x00133110, PAL: 0x00133960
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

// NTSC-U/C: 0x00132d70, PAL: 0x001335b0
int Player::CountCaughtGem() {
    // The image leaves the return register untouched here, so the value is indeterminate.
    return 0;
}

// NTSC-U/C: 0x00132d78, PAL: 0x001335b8
int Player::CountMissedGem() {
    // The image leaves the return register untouched here, so the value is indeterminate.
    return 0;
}

// NTSC-U/C: 0x00132d80, PAL: 0x001335c0
int Player::GetMultiplier(int) {
    return 1;
}

// NTSC-U/C: 0x00132d88, PAL: 0x001335c8
int Player::GetBestStreak() {
    return 0;
}

// NTSC-U/C: 0x00132d90, PAL: 0x001335d0
float Player::GetCaptureRatio() {
    return 0.0f;
}

// NTSC-U/C: 0x00132da0, PAL: 0x001335e0
int Player::GetGameMode() {
    return 0;
}

// NTSC-U/C: 0x00132db0, PAL: 0x001335f0
int Player::MarkBarScored(int) {
    return 1;
}

// NTSC-U/C: 0x001330e0, PAL: 0x00133930
int Player::GetScore() {
    return mScore;
}

// NTSC-U/C: 0x001330f8, PAL: 0x00133948
int Player::GetJuice() {
    return mJuice;
}

// NTSC-U/C: 0x001330e8, PAL: 0x00133938
void Player::SetScore(int nScore, int nMaxScore) {
    mMaxScore = nMaxScore;
    mScore = nScore;
}

// NTSC-U/C: 0x00133100, PAL: 0x00133950
void Player::SetJuice(int nJuice, int nMaxJuice) {
    mMaxJuice = nMaxJuice;
    mJuice = nJuice;
}

// NTSC-U/C: 0x0012f808, PAL: 0x0012ffc0
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

// NTSC-U/C: 0x001331c8, PAL: 0x00133a18
void Player::OnMsg(const PhraseCapturedMsg &msg) {
    AddScore(msg.mScore, 1);
    AddJuice(msg.mJuice, 1);
}

// NTSC-U/C: 0x0012f970, PAL: 0x00130128
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

// NTSC-U/C: 0x00132ae8, PAL: 0x00133318
Player::~Player() {
}

// NTSC-U/C: 0x00132c68, PAL: 0x001334a8
// The out-of-line copy.
inline HxStr Player::GetColorName() {
    return mColorName;
}

// NTSC-U/C: 0x001330a8, PAL: 0x001338f8
// The out-of-line copy.
inline HxStr Player::GetUsername() {
    return mAppearance->mUserName;
}

// NTSC-U/C: 0x00132cd0, PAL: 0x00133510
// The out-of-line copy.
int Player::StartMF() {
    AnnounceState();
    return 0;
}

// NTSC-U/C: 0x00132d00, PAL: 0x00133540
// The out-of-line copy.
int Player::StopMF() {
    DeactivatePlacer();
    return 0;
}

// NTSC-U/C: 0x00133210, PAL: 0x00133a60
// The out-of-line copy.
inline void Player::OnUpdateScore(UpdateScorePacket *pPacket) {
    if (pPacket->mPlayerId == mPlayerId) {
        AddScore(pPacket->mScoreDelta, 0);
    }
}

// NTSC-U/C: 0x00133240, PAL: 0x00133a90
void Player::DispatchPriv(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nUpdateScorePacketType) {
        OnUpdateScore(static_cast<UpdateScorePacket *>(pMsg));
    } else if (nType == g_nPhraseCapturedMsgType) {
        // Awarded whichever player the capture names.
        OnMsg(*static_cast<PhraseCapturedMsg *>(pMsg));
    }
}
