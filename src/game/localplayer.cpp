#include "game/localplayer.h"

#include <algorithm>

#include "app/application.h"
#include "app/msgsource.h"
#include "app/playsound.h"
#include "game/gamemanagerimpl.h"
#include "game/jampowerupplacer.h"
#include "game/localplayercmd.h"
#include "game/powerupcollection.h"
#include "game/simplifiedgamepowerupplacer.h"
#include "game/singlepowerupcollection.h"
#include "msg/axisxpowmsg.h"
#include "msg/axisypowmsg.h"
#include "msg/buttonpowmsg.h"
#include "msg/caughtbarmsg.h"
#include "msg/caughtpowerbarmsg.h"
#include "msg/deployedpowerupmsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/looptogglemsg.h"
#include "msg/looptoolmsg.h"
#include "msg/multipliermsg.h"
#include "msg/multiplierstatemsg.h"
#include "msg/phrasecapturedmsg.h"
#include "msg/phrasemuffedmsg.h"
#include "msg/pointamountmsg.h"
#include "msg/toggleghostmsg.h"
#include "msg/trackselectmsg.h"
#include "msg/trackselectpacket.h"
#include "sch/tickclock.h"

namespace {

// MIDI ticks in one bar.
constexpr int kTicksPerBar = 1920;

// The handle the constructor starts mCommand with, before any command is posted.
constexpr int kUnallocatedCommand = -2;

// The bars and flags the constructor starts at.
constexpr int kNoBar = -1;
constexpr int kJamPowerupsUnlimited = 1;
constexpr int kJamFreestyleEndBar = 10000000;

// The ceiling Slot11() caps the announced score ceiling at, as Player's announcements do.
constexpr int kAnnouncedMaximum = 800;

// Capture streaks and multipliers.
constexpr int kMultiplierStart = 1;
constexpr int kMultiplierCapExceeded = 4;
constexpr int kMultiplierMaximum = 3;

// A multiplier powerup raises the multiplier by this much for this many bars.
constexpr int kBonusMultiplier = 2;
constexpr int kBonusBars = 8;

// The multiplier falls back once this many bars pass after the last caught bar.
constexpr int kMultiplierDecayBars = 2;

// The sound a caught powerup plays before its own.
constexpr char kCaughtPowerSound[] = "SND_CAUGHT_POWER";

} // namespace

// 0x0011e000
LocalPlayer::LocalPlayer(int nId,
                         int nInputSlot,
                         const HxStr &colorName,
                         const FreqAppearance *pAppearance,
                         Sch::TickClock *pClock,
                         int nTrack)
    : Player(nId, colorName, pAppearance), mClock(pClock), mInputSlot(nInputSlot), mTrack(nTrack),
      mPlace(0), mLooping(0), mGhost(0) {
    mCommand.mValue = kUnallocatedCommand;
    mPlayMode = Application::shared()->GetPlayMode();
    mGameMode = Application::shared()->GetGameMode();
    mLastMuffedBar = kNoBar;
    mUnknown70 = 0;
    mUnknown74 = 0;
    mUnknown78 = kNoBar;
    mRunEndBar = kNoBar;
    mLastCaughtBar = kNoBar;
    mStreak = 0;
    mBestStreak = 0;
    mMultiplier = 0;
    mBonus = 0;
    mCaptures = 0;
    mMisses = 0;
    mCollection = nullptr;
    mPlacer = nullptr;
    mUnknownac = 0;
    mUnknownb0 = 0;

    if (mPlayMode == kPlayModeJam) {
        mCollection = new PowerupCollection(this, kJamPowerupsUnlimited);
        mPlacer = new JamPowerupPlacer(this, mCollection);
        LocalPlayer::Slot8(0, kJamFreestyleEndBar);
    } else if (mGameMode >= kGameModeSolo && mGameMode <= kGameModeNet) {
        mCollection = new SinglePowerupCollection(this);
        mPlacer = new SimplifiedGamePowerupPlacer(this, mCollection);
    }

    if (mPlayMode == kPlayModeJam) {
        mLooping = 1;
    }
}

// 0x0011e348
LocalPlayer::~LocalPlayer() {
    delete mPlacer;
    delete mCollection;
}

// 0x00121ea0
int LocalPlayer::Slot2() {
    return mInputSlot;
}

// 0x00121e90
int LocalPlayer::Slot4() {
    return mTrack;
}

// 0x00121e98
int LocalPlayer::Slot5() {
    return mPlace;
}

// 0x00122890
int LocalPlayer::Slot6() {
    return 0;
}

// 0x00121ea8
void LocalPlayer::Slot7() {
}

// 0x00122898
void LocalPlayer::Slot8(int first, int second) {
    mUnknown70 = first;
    mUnknown74 = second;
}

// 0x00121eb0
int LocalPlayer::Slot10() {
    return mLooping;
}

// 0x0011e4e0
void LocalPlayer::Slot11() {
    Player::Slot11();
    const int nTick = Application::shared()->GetSongClock()->SongTick();
    mPlacer->OnUnknownSlot4();

    TrackSelectMsg trackSelect;
    trackSelect.mUnknown04 = mTrack;
    trackSelect.mUnknown08 = 0;
    trackSelect.mPosition.mTick = nTick;
    trackSelect.mUnknown10 = this;
    Send(&trackSelect);

    mCollection->AnnounceState();

    PointAmountMsg points;
    points.mPlayer = this;
    points.mMaxScore = std::min(mUnknown3c, kAnnouncedMaximum);
    Send(&points);

    ToggleGhostMsg ghost;
    ghost.mUnknown04 = this;
    ghost.mOn = mGhost;
    Send(&ghost);

    LoopToggleMsg loop(mLooping, this);
    Send(&loop);

    const int nFirstBarEnd = Mid::MBT(kTicksPerBar).mTick;
    LocalPlayerCmd *pCommand = new LocalPlayerCmd(this, nFirstBarEnd);
    mClock->PostAtSongTick(pCommand, nFirstBarEnd, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// 0x001228c8
void LocalPlayer::Slot12() {
    mPlacer->OnUnknownSlot5();
}

// 0x0011e810
void LocalPlayer::SetLooping(int bLooping, const Mid::MBT &position) {
    if (mPlayMode != kPlayModeJam) {
        return;
    }

    mLooping = bLooping;
    InvalidateSeekerMsg invalidateSeeker(position.mTick / Mid::MBT(kTicksPerBar).mTick, mTrack);
    Send(&invalidateSeeker);
    LoopToggleMsg loop(mLooping, this);
    Send(&loop);
}

// 0x0011e908
void LocalPlayer::Slot22(int value) {
    if (mPlayMode != kPlayModeJam) {
        return;
    }

    mGhost = value;

    ToggleGhostMsg message;
    message.mOn = value;
    message.mUnknown04 = this;
    Send(&message);
}

// 0x00121ec0
int LocalPlayer::Slot14() {
    return mUnknownb0 + 1;
}

// 0x00121ed0
int LocalPlayer::Slot15() {
    return mUnknownac + 1;
}

// 0x00122cc8
float LocalPlayer::Slot18() {
    if (mCaptures == 0) {
        return 0.0f;
    }
    return static_cast<float>(mCaptures) / static_cast<float>(mCaptures + mMisses);
}

// 0x00121ee0
int LocalPlayer::Slot17() {
    return mBestStreak;
}

// 0x00121ee8
int LocalPlayer::Slot19() {
    return mGameMode;
}

// 0x001228a8
int LocalPlayer::Slot9(int value) {
    if (value < mUnknown70) {
        return 0;
    }
    return value < mUnknown74;
}

// 0x00122ca0
int LocalPlayer::Slot16(int value) {
    if (mRunEndBar < value) {
        return mBonus + 1;
    }
    return mMultiplier + (mBonus + 1);
}

// 0x00122c00
int LocalPlayer::Slot20(int value) {
    if (mUnknown78 < value) {
        mUnknown78 = value;
        return 1;
    }
    return 0;
}

// The address below is the out-of-line copy.
// 0x00122a20
inline void LocalPlayer::OnAxisYPow(AxisYPowMsg *pMsg) {
    if (pMsg->mPlayer == this && mCollection != nullptr) {
        mCollection->SelectRelative(pMsg->mValue);
    }
}

// The address below is the out-of-line copy.
// 0x00122ae0
inline void LocalPlayer::OnLoopTool(LoopToolMsg *pMsg) {
    if (pMsg->mPlayer == this) {
        const Mid::MBT position = pMsg->mPosition;
        ToggleLoop(position);
    }
}

// The address below is the out-of-line copy.
// 0x00122b38
inline void LocalPlayer::OnPhraseCaptured(PhraseCapturedMsg *pMsg) {
    if (pMsg->mPlayer != this) {
        return;
    }

    if (mRunEndBar < pMsg->mRunFirstBar) {
        mStreak = 1;
        mMultiplier = kMultiplierStart;
    } else if (pMsg->mExtendsStreak != 0) {
        ++mMultiplier;
        ++mStreak;
    }
    if (mBestStreak < mStreak) {
        mBestStreak = mStreak;
    }
    if (mMultiplier == kMultiplierCapExceeded) {
        mMultiplier = kMultiplierMaximum;
    }
    ++mCaptures;
    mRunEndBar = pMsg->mRunEndBar;
    mLastCaughtBar = pMsg->mRunEndBar - 1;
    AwardCapture(pMsg);
}

// The address below is the out-of-line copy.
// 0x00122c20
inline void LocalPlayer::OnPhraseMuffed(PhraseMuffedMsg *pMsg) {
    if (pMsg->mPlayer != this || pMsg->mTried == 0) {
        return;
    }

    const int nBar = pMsg->mPosition.mTick / Mid::MBT(kTicksPerBar).mTick;
    if (mLastMuffedBar != nBar) {
        mLastMuffedBar = nBar;
        ++mMisses;
    }
}

// The address below is the out-of-line copy.
// 0x001229d8
inline void LocalPlayer::OnButtonPow(ButtonPowMsg *pMsg) {
    if (pMsg->mPlayer == this && mPlacer != nullptr) {
        mPlacer->OnUnknownSlot8();
    }
}

// The address below is the out-of-line copy.
// 0x00122a68
inline void LocalPlayer::OnCaughtPowerbar(CaughtPowerbarMsg *pMsg) {
    if (pMsg->mPlayer != this) {
        return;
    }

    if (mCollection != nullptr) {
        mCollection->AddPowerup(pMsg->mKind);
    }
    PlaySoundByName(kCaughtPowerSound);
    PlayPowerupSound(pMsg->mKind);
    Send(pMsg);
}

// 0x0011ed98
void LocalPlayer::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (static_cast<unsigned int>(nType) == g_dwTrackSelectMsgType) {
        OnTrackSelect(static_cast<TrackSelectMsg *>(pMsg));
    } else if (nType == g_nAxisXPowMsgType) {
        return;
    } else if (nType == g_nAxisYPowMsgType) {
        OnAxisYPow(static_cast<AxisYPowMsg *>(pMsg));
    } else if (nType == g_nLoopToolMsgType) {
        OnLoopTool(static_cast<LoopToolMsg *>(pMsg));
    } else if (nType == g_nPhraseCapturedMsgType) {
        OnPhraseCaptured(static_cast<PhraseCapturedMsg *>(pMsg));
    } else if (nType == g_nPhraseMuffedMsgType) {
        OnPhraseMuffed(static_cast<PhraseMuffedMsg *>(pMsg));
    } else if (nType == g_nCaughtBarMsgType) {
        CaughtBarMsg *pCaught = static_cast<CaughtBarMsg *>(pMsg);
        if (pCaught->mPlayer == this) {
            mLastCaughtBar = pCaught->mBar;
        }
    } else if (nType == g_nMultiplierMsgType) {
        OnMultiplier(static_cast<MultiplierMsg *>(pMsg));
    } else if (nType == g_nToggleGhostMsgType) {
        OnToggleGhost(static_cast<ToggleGhostMsg *>(pMsg));
    } else if (nType == g_nButtonPowMsgType) {
        OnButtonPow(static_cast<ButtonPowMsg *>(pMsg));
    } else if (nType == g_nCaughtPowerbarMsgType) {
        OnCaughtPowerbar(static_cast<CaughtPowerbarMsg *>(pMsg));
    } else {
        Player::HandleMessage(pMsg);
    }
}

// 0x001228f8
void LocalPlayer::AddSink(MsgSink *pSink) {
    MsgSource::AddSink(pSink);
    mPlacer->AddSink(pSink);
    mCollection->AddSink(pSink);
}

// 0x00122968
void LocalPlayer::RemoveSink(MsgSink *pSink) {
    MsgSource::RemoveSink(pSink);
    mPlacer->RemoveSink(pSink);
    mCollection->RemoveSink(pSink);
}

// 0x0011ec00
void LocalPlayer::OnBarTick(int nTick) {
    const int nBar = nTick / Mid::MBT(kTicksPerBar).mTick;
    bool bChanged = false;
    if (mBonus > 0 && nBar >= mBonusEndBar) {
        mBonus = 0;
        bChanged = true;
    }
    if (mLastCaughtBar + kMultiplierDecayBars == nBar) {
        mMultiplier = 0;
        bChanged = true;
    }
    if (bChanged) {
        MultiplierStateMsg state(this, mMultiplier + 1, mBonus);
        Send(&state);
    }

    const Mid::MBT next(
        std::min(std::max(nTick + Mid::MBT(kTicksPerBar).mTick, kMBTMinimum), kMBTMaximum));
    LocalPlayerCmd *pCommand = new LocalPlayerCmd(this, next.mTick);
    mClock->PostAtSongTick(pCommand, next.mTick, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// 0x0011e700
void LocalPlayer::ToggleLoop(const Mid::MBT &position) {
    if (mPlayMode != kPlayModeJam) {
        return;
    }

    mLooping ^= 1;
    InvalidateSeekerMsg invalidateSeeker(position.mTick / Mid::MBT(kTicksPerBar).mTick, mTrack);
    Send(&invalidateSeeker);
    LoopToggleMsg loop(mLooping, this);
    Send(&loop);
}

// 0x0011e980
void LocalPlayer::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mUnknown10 != this) {
        return;
    }

    const int nOldTrack = mTrack;
    const int nOldPlace = mPlace;
    mTrack = pMsg->mUnknown04;
    mPlace = pMsg->mUnknown08;
    if (mPlacer != nullptr) {
        mPlacer->OnUnknownSlot7();
    }

    if (mTrack == nOldTrack && mPlace < nOldPlace) {
        return;
    }
    TrackSelectPacket packet(pMsg->mPosition, this, mTrack, mPlace);
    Send(&packet);
}

// 0x0011eaa8
void LocalPlayer::OnToggleGhost(ToggleGhostMsg *pMsg) {
    if (pMsg->mUnknown04 != this) {
        return;
    }

    mGhost ^= 1;
    ToggleGhostMsg ghost;
    ghost.mUnknown04 = this;
    ghost.mOn = mGhost;
    Send(&ghost);
}

// 0x0011eb20
void LocalPlayer::OnMultiplier(MultiplierMsg *pMsg) {
    mBonus = kBonusMultiplier;
    mBonusEndBar = pMsg->mBar + kBonusBars;
    DeployedPowerupMsg deployed(kHudItemMultiplier, this, nullptr, 0, 0, 0);
    Send(&deployed);
    MultiplierStateMsg state(this, mMultiplier + 1, mBonus);
    Send(&state);
}
