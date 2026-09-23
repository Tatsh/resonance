#include "game/player.h"

#include <algorithm>
#include <iostream>

#include "app/msgsource.h"
#include "msg/juiceamountmsg.h"
#include "msg/pointamountmsg.h"
#include "msg/updatescorepacket.h"

namespace {

// Ceiling Slot11 applies to the value it publishes.
constexpr int kJuiceMaximum = 800;

constexpr char kNullText[] = "{player null}";
constexpr char kOpenText[] = "{player ";

} // namespace

// 0x00132c20
int Player::Slot2() {
    return -1;
}

// 0x00132c60
int Player::IsNull() {
    return 0;
}

// 0x00132c98
int Player::Slot4() {
    return -1;
}

// 0x00132ca0
int Player::Slot5() {
    return 0;
}

// 0x00132ca8
int Player::Slot6() {
    return 0;
}

// 0x00132cb0
void Player::Slot7() {
}

// 0x00132cb8
void Player::Slot8(int, int) {
}

// 0x00132cc0
int Player::Slot9(int) {
    return 0;
}

// 0x00132cc8
int Player::Slot10() {
    return 0;
}

// 0x0012f788
void Player::Slot11() {
    JuiceAmountMsg message;
    message.mUnknown04 = this;
    message.mUnknown08 = mUnknown34 < kJuiceMaximum ? mUnknown34 : kJuiceMaximum;

    Send(&message);
}

// 0x00132d30
void Player::Slot12() {
}

// 0x00133110
void Player::Print(std::ostream &stream) {
    if (IsNull() != 0) {
        stream << kNullText;
        return;
    }

    stream << kOpenText << mId20;
}

// 0x00132d70
int Player::Slot14() {
    // The image leaves the return register untouched here, so the value is indeterminate.
    return 0;
}

// 0x00132d78
int Player::Slot15() {
    // The image leaves the return register untouched here, so the value is indeterminate.
    return 0;
}

// 0x00132d80
int Player::Slot16(int) {
    return 1;
}

// 0x00132d88
int Player::Slot17() {
    return 0;
}

// 0x00132d90
float Player::Slot18() {
    return 0.0f;
}

// 0x00132da0
int Player::Slot19() {
    return 0;
}

// 0x00132db0
int Player::Slot20(int) {
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
    mUnknown3c = nMaxScore;
    mScore = nScore;
}

// 0x00133100
void Player::SetJuice(int nJuice, int nMaxJuice) {
    mUnknown34 = nMaxJuice;
    mJuice = nJuice;
}

// 0x0012f808
void Player::AddScore(int nDelta, int bNotify) {
    const int nOldScore = mScore;
    mScore = std::max(0, std::min(mScore + nDelta, mUnknown3c));
    if (mScore == nOldScore) {
        return;
    }

    PointAmountMsg message;
    message.mPlayer = this;
    // Yes, the binary caps the score ceiling at the juice maximum too.
    message.mMaxScore = std::min(mUnknown3c, kJuiceMaximum);
    Send(&message);

    if (bNotify != 0) {
        UpdateScorePacket packet(mId20, nDelta);
        Send(&packet);
    }
}

// 0x0012f970
void Player::AddJuice(int nAmount, int bNotify) {
    const int nOldJuice = mJuice;
    mJuice = std::max(0, std::min(mJuice + nAmount, mUnknown34));
    if (mJuice == nOldJuice) {
        return;
    }

    JuiceAmountMsg message;
    message.mUnknown04 = this;
    message.mUnknown08 = std::min(mUnknown34, kJuiceMaximum);
    Send(&message);

    if (bNotify != 0) {
        UpdateScorePacket packet(mId20, nAmount);
        Send(&packet);
    }
}
