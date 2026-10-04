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

LocalPlayer::~LocalPlayer() {
    delete mPlacer;
    delete mCollection;
}

int LocalPlayer::GetInputSlot() const {
    return mInputSlot;
}

int LocalPlayer::GetTrack() {
    return mTrack;
}

int LocalPlayer::GetPlace() {
    return mPlace;
}

int LocalPlayer::UnusedQuery() {
    return 0;
}

void LocalPlayer::UnusedHook() {
}

void LocalPlayer::SetFreestyleSpan(int nStartBar, int nEndBar) {
    mFreestyleStartBar = nStartBar;
    mFreestyleEndBar = nEndBar;
}

int LocalPlayer::IsLooping() {
    return mLooping;
}

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

    mCollection->SendState();

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

    const int nFirstBarEnd = Sch::Tick(kTicksPerBar).mTick;
    LocalPlayerCmd *pCommand = new LocalPlayerCmd(this, nFirstBarEnd);
    mClock->PostAtSongTick(pCommand, nFirstBarEnd, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

void LocalPlayer::DeactivatePlacer() {
    mPlacer->Deactivate();
}

void LocalPlayer::SetLooping(int bLooping, const Sch::Tick &position) {
    if (mPlayMode != kPlayModeJam) {
        return;
    }

    mLooping = bLooping;
    InvalidateSeekerMsg invalidateSeeker(position.mTick / Sch::Tick(kTicksPerBar).mTick, mTrack);
    Send(&invalidateSeeker);
    LoopToggleMsg loop(mLooping, this);
    Send(&loop);
}

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

int LocalPlayer::CountCaughtGem() {
    return mCaughtGems += 1;
}

int LocalPlayer::CountMissedGem() {
    return mMissedGems += 1;
}

float LocalPlayer::GetCaptureRatio() {
    if (mCaptures == 0) {
        return 0.0f;
    }
    return static_cast<float>(mCaptures) / static_cast<float>(mCaptures + mMisses);
}

int LocalPlayer::GetBestStreak() {
    return mBestStreak;
}

int LocalPlayer::GetGameMode() {
    return mGameMode;
}

int LocalPlayer::IsFreestyleBar(int nBar) {
    if (nBar < mFreestyleStartBar) {
        return 0;
    }
    return nBar < mFreestyleEndBar;
}

int LocalPlayer::GetMultiplier(int nBar) {
    if (mRunEndBar < nBar) {
        return mBonus + 1;
    }
    return mMultiplier + (mBonus + 1);
}

int LocalPlayer::MarkBarScored(int nBar) {
    if (mLastScoredBar < nBar) {
        mLastScoredBar = nBar;
        return 1;
    }
    return 0;
}

inline void LocalPlayer::OnAxisYPow(AxisYPowMsg *pMsg) {
    if (pMsg->mPlayer == this && mCollection != nullptr) {
        mCollection->SelectRelative(pMsg->mValue);
    }
}

inline void LocalPlayer::OnLoopTool(LoopToolMsg *pMsg) {
    if (pMsg->mPlayer == this) {
        const Sch::Tick position = pMsg->mPosition;
        ToggleLoop(position);
    }
}

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
    OnMsg(*pMsg);
}

inline void LocalPlayer::OnPhraseMuffed(PhraseMuffedMsg *pMsg) {
    if (pMsg->mPlayer != this || pMsg->mTried == 0) {
        return;
    }

    const int nBar = pMsg->mPosition.mTick / Sch::Tick(kTicksPerBar).mTick;
    if (mLastMuffedBar != nBar) {
        mLastMuffedBar = nBar;
        ++mMisses;
    }
}

inline void LocalPlayer::OnButtonPow(ButtonPowMsg *pMsg) {
    if (pMsg->mPlayer == this && mPlacer != nullptr) {
        mPlacer->DeployPowerup();
    }
}

inline void LocalPlayer::OnCaughtPowerbar(CaughtPowerbarMsg *pMsg) {
    if (pMsg->mPlayer != this) {
        return;
    }

    if (mCollection != nullptr) {
        mCollection->Add(pMsg->mKind);
    }
    PlaySoundByName(kCaughtPowerSound);
    PlayPowerupSound(pMsg->mKind);
    Send(pMsg);
}

void LocalPlayer::DispatchPriv(Message *pMsg) {
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
        Player::DispatchPriv(pMsg);
    }
}

void LocalPlayer::AddSink(MsgSink *pSink) {
    MsgSource::AddSink(pSink);
    mPlacer->AddSink(pSink);
    mCollection->AddSink(pSink);
}

void LocalPlayer::RemoveSink(MsgSink *pSink) {
    MsgSource::RemoveSink(pSink);
    mPlacer->RemoveSink(pSink);
    mCollection->RemoveSink(pSink);
}

void LocalPlayer::OnBarTick(int nTick) {
    const int nBar = nTick / Sch::Tick(kTicksPerBar).mTick;
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

    const Sch::Tick next(
        std::min(std::max(nTick + Sch::Tick(kTicksPerBar).mTick, kTickMinimum), kTickMaximum));
    LocalPlayerCmd *pCommand = new LocalPlayerCmd(this, next.mTick);
    mClock->PostAtSongTick(pCommand, next.mTick, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

void LocalPlayer::ToggleLoop(const Sch::Tick &position) {
    if (mPlayMode != kPlayModeJam) {
        return;
    }

    mLooping ^= 1;
    InvalidateSeekerMsg invalidateSeeker(position.mTick / Sch::Tick(kTicksPerBar).mTick, mTrack);
    Send(&invalidateSeeker);
    LoopToggleMsg loop(mLooping, this);
    Send(&loop);
}

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

void LocalPlayer::OnMultiplier(MultiplierMsg *pMsg) {
    mBonus = kBonusMultiplier;
    mBonusEndBar = pMsg->mBar + kBonusBars;
    DeployedPowerupMsg deployed(kHudItemMultiplier, this, nullptr, 0, 0, 0);
    Send(&deployed);
    MultiplierStateMsg state(this, mMultiplier + 1, mBonus);
    Send(&state);
}
