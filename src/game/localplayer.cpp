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

// The ceiling AnnounceState() caps the announced score ceiling at, as Player's announcements do.
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
constexpr char kCaughtPowerSound[] = "SND_CAUGHT_POWERUP";

} // namespace

// NTSC-U/C: 0x0011e000, PAL: 0x0011e5a0
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
    mFreestyleStartBar = 0;
    mFreestyleEndBar = 0;
    mLastScoredBar = kNoBar;
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
    mMissedGems = 0;
    mCaughtGems = 0;

    if (mPlayMode == kPlayModeJam) {
        mCollection = new PowerupCollection(this, kJamPowerupsUnlimited);
        mPlacer = new JamPowerupPlacer(this, mCollection);
        LocalPlayer::SetFreestyleSpan(0, kJamFreestyleEndBar);
    } else if (mGameMode >= kGameModeSolo && mGameMode <= kGameModeNet) {
        mCollection = new SinglePowerupCollection(this);
        mPlacer = new SimplifiedGamePowerupPlacer(this, mCollection);
    }

    if (mPlayMode == kPlayModeJam) {
        mLooping = 1;
    }
}

// NTSC-U/C: 0x0011e348, PAL: 0x0011e8f8
LocalPlayer::~LocalPlayer() {
    delete mPlacer;
    delete mCollection;
}

// NTSC-U/C: 0x00121ea0, PAL: 0x001224a8
int LocalPlayer::GetInputSlot() {
    return mInputSlot;
}

// NTSC-U/C: 0x00121e90, PAL: 0x00122498
int LocalPlayer::GetTrack() {
    return mTrack;
}

// NTSC-U/C: 0x00121e98, PAL: 0x001224a0
int LocalPlayer::GetPlace() {
    return mPlace;
}

// NTSC-U/C: 0x00122890, PAL: 0x00122ea8
int LocalPlayer::UnusedQuery() {
    return 0;
}

// NTSC-U/C: 0x00121ea8, PAL: 0x001224b0
void LocalPlayer::UnusedHook() {
}

// NTSC-U/C: 0x00122898, PAL: 0x00122eb0
void LocalPlayer::SetFreestyleSpan(int nStartBar, int nEndBar) {
    mFreestyleStartBar = nStartBar;
    mFreestyleEndBar = nEndBar;
}

// NTSC-U/C: 0x00121eb0, PAL: 0x001224b8
int LocalPlayer::IsLooping() {
    return mLooping;
}

// NTSC-U/C: 0x0011e4e0, PAL: 0x0011eaa0
void LocalPlayer::AnnounceState() {
    Player::AnnounceState();
    const int nTick = Application::shared()->GetSongClock()->SongTick();
    mPlacer->Activate();

    TrackSelectMsg trackSelect;
    trackSelect.mTrack = mTrack;
    trackSelect.mPlace = 0;
    trackSelect.mPosition.mTick = nTick;
    trackSelect.mPlayer = this;
    Send(&trackSelect);

    mCollection->AnnounceState();

    PointAmountMsg points;
    points.mPlayer = this;
    points.mMaxScore = std::min(mMaxScore, kAnnouncedMaximum);
    Send(&points);

    ToggleGhostMsg ghost;
    ghost.mPlayer = this;
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

// NTSC-U/C: 0x001228c8, PAL: 0x00122ee0
void LocalPlayer::DeactivatePlacer() {
    mPlacer->Deactivate();
}

// NTSC-U/C: 0x0011e810, PAL: 0x0011edd0
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

// NTSC-U/C: 0x0011e908, PAL: 0x0011eec8
void LocalPlayer::SetGhost(int bGhost) {
    if (mPlayMode != kPlayModeJam) {
        return;
    }

    mGhost = bGhost;

    ToggleGhostMsg message;
    message.mOn = bGhost;
    message.mPlayer = this;
    Send(&message);
}

// NTSC-U/C: 0x00121ec0, PAL: 0x001224c8
int LocalPlayer::CountCaughtGem() {
    return mCaughtGems += 1;
}

// NTSC-U/C: 0x00121ed0, PAL: 0x001224d8
int LocalPlayer::CountMissedGem() {
    return mMissedGems += 1;
}

// NTSC-U/C: 0x00122cc8, PAL: 0x001232e0
float LocalPlayer::GetCaptureRatio() {
    if (mCaptures == 0) {
        return 0.0f;
    }
    return static_cast<float>(mCaptures) / static_cast<float>(mCaptures + mMisses);
}

// NTSC-U/C: 0x00121ee0, PAL: 0x001224e8
int LocalPlayer::GetBestStreak() {
    return mBestStreak;
}

// NTSC-U/C: 0x00121ee8, PAL: 0x001224f0
int LocalPlayer::GetGameMode() {
    return mGameMode;
}

// NTSC-U/C: 0x001228a8, PAL: 0x00122ec0
int LocalPlayer::IsFreestyleBar(int nBar) {
    if (nBar < mFreestyleStartBar) {
        return 0;
    }
    return nBar < mFreestyleEndBar;
}

// NTSC-U/C: 0x00122ca0, PAL: 0x001232b8
int LocalPlayer::GetMultiplier(int nBar) {
    if (mRunEndBar < nBar) {
        return mBonus + 1;
    }
    return mMultiplier + (mBonus + 1);
}

// NTSC-U/C: 0x00122c00, PAL: 0x00123218
int LocalPlayer::MarkBarScored(int nBar) {
    if (mLastScoredBar < nBar) {
        mLastScoredBar = nBar;
        return 1;
    }
    return 0;
}

// The address below is the out-of-line copy.
// NTSC-U/C: 0x00122a20, PAL: 0x00123038
inline void LocalPlayer::OnAxisYPow(AxisYPowMsg *pMsg) {
    if (pMsg->mPlayer == this && mCollection != nullptr) {
        mCollection->SelectRelative(pMsg->mValue);
    }
}

// The address below is the out-of-line copy.
// NTSC-U/C: 0x00122ae0, PAL: 0x001230f8
inline void LocalPlayer::OnLoopTool(LoopToolMsg *pMsg) {
    if (pMsg->mPlayer == this) {
        const Mid::MBT position = pMsg->mPosition;
        ToggleLoop(position);
    }
}

// The address below is the out-of-line copy.
// NTSC-U/C: 0x00122b38, PAL: 0x00123150
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
// NTSC-U/C: 0x00122c20, PAL: 0x00123238
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
// NTSC-U/C: 0x001229d8, PAL: 0x00122ff0
inline void LocalPlayer::OnButtonPow(ButtonPowMsg *pMsg) {
    if (pMsg->mPlayer == this && mPlacer != nullptr) {
        mPlacer->DeployPowerup();
    }
}

// The address below is the out-of-line copy.
// NTSC-U/C: 0x00122a68, PAL: 0x00123080
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

// NTSC-U/C: 0x0011ed98, PAL: 0x0011f358
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

// NTSC-U/C: 0x001228f8, PAL: 0x00122f10
void LocalPlayer::AddSink(MsgSink *pSink) {
    MsgSource::AddSink(pSink);
    mPlacer->AddSink(pSink);
    mCollection->AddSink(pSink);
}

// NTSC-U/C: 0x00122968, PAL: 0x00122f80
void LocalPlayer::RemoveSink(MsgSink *pSink) {
    MsgSource::RemoveSink(pSink);
    mPlacer->RemoveSink(pSink);
    mCollection->RemoveSink(pSink);
}

// NTSC-U/C: 0x0011ec00, PAL: 0x0011f1c0
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

// NTSC-U/C: 0x0011e700, PAL: 0x0011ecc0
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

// NTSC-U/C: 0x0011e980, PAL: 0x0011ef40
void LocalPlayer::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mPlayer != this) {
        return;
    }

    const int nOldTrack = mTrack;
    const int nOldPlace = mPlace;
    mTrack = pMsg->mTrack;
    mPlace = pMsg->mPlace;
    if (mPlacer != nullptr) {
        mPlacer->AnnounceCursor();
    }

    if (mTrack == nOldTrack && mPlace < nOldPlace) {
        return;
    }
    TrackSelectPacket packet(pMsg->mPosition, this, mTrack, mPlace);
    Send(&packet);
}

// NTSC-U/C: 0x0011eaa8, PAL: 0x0011f068
void LocalPlayer::OnToggleGhost(ToggleGhostMsg *pMsg) {
    if (pMsg->mPlayer != this) {
        return;
    }

    mGhost ^= 1;
    ToggleGhostMsg ghost;
    ghost.mPlayer = this;
    ghost.mOn = mGhost;
    Send(&ghost);
}

// NTSC-U/C: 0x0011eb20, PAL: 0x0011f0e0
void LocalPlayer::OnMultiplier(MultiplierMsg *pMsg) {
    mBonus = kBonusMultiplier;
    mBonusEndBar = pMsg->mBar + kBonusBars;
    DeployedPowerupMsg deployed(kHudItemMultiplier, this, nullptr, 0, 0, 0);
    Send(&deployed);
    MultiplierStateMsg state(this, mMultiplier + 1, mBonus);
    Send(&state);
}
