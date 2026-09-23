#include "app/overlay.h"

#include <algorithm>

#include "app/application.h"
#include "app/hudbadge.h"
#include "app/hudpanel.h"
#include "app/hudtrack.h"
#include "app/hudutil.h"
#include "app/renderer.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/grooveworld.h"
#include "game/leveldata.h"
#include "game/player.h"
#include "game/playmap.h"
#include "game/trackdata.h"
#include "gfx/gfxdevice.h"
#include "mid/mbt.h"
#include "msg/advancesectiontogglemsg.h"
#include "msg/beginphrasecatchmsg.h"
#include "msg/catchmsg.h"
#include "msg/caughtpowerbarmsg.h"
#include "msg/choosepowerupmsg.h"
#include "msg/deployedpowerupmsg.h"
#include "msg/fadegamemsg.h"
#include "msg/gameovermsg.h"
#include "msg/jameffectmsg.h"
#include "msg/juiceamountmsg.h"
#include "msg/looptogglemsg.h"
#include "msg/message.h"
#include "msg/multiplierstatemsg.h"
#include "msg/phrasecapturedmsg.h"
#include "msg/phrasemuffedmsg.h"
#include "msg/playbacktogglemsg.h"
#include "msg/playerstrackneutralizedmsg.h"
#include "msg/pointamountmsg.h"
#include "msg/powerupcountmsg.h"
#include "msg/powerupfailedmsg.h"
#include "msg/showeraseeffectmsg.h"
#include "msg/textmsg.h"
#include "msg/toggleghostmsg.h"
#include "msg/trackselectmsg.h"
#include "msg/winmsg.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/view.h"
#include "script/configquery.h"
#include "script/scripthost.h"

namespace {

// MIDI ticks in one bar.
constexpr float kTicksPerBar = 1920.0f;

// Configuration code whose flag the constructor records in mUnknown44.
constexpr int kDisplayModeConfigCode = 0x3a1;

// mCurrentBar before SetFrame() sees the first bar.
constexpr int kNoCurrentBar = -123123;

// Player::Slot2() for a player without a track.
constexpr int kNoPlayerSlot = -1;

// A three-player session uses the four-player layout.
constexpr int kThreePlayerCount = 3;
constexpr int kFourPlayerLayout = 4;

// Microseconds per millisecond times MIDI ticks per quarter note. The tempo divided by it is the
// milliseconds one MIDI tick lasts.
constexpr float kTempoToMsPerTick = 480000.0f;

// The constructor records a name and a kind for this many tracks of the level.
constexpr int kTrackCount = 8;

// TrackData::mInstrument values.
enum Instrument {
    kInstrumentDrums = 0,
    kInstrumentBass = 1,
    kInstrumentSynth = 2,
    kInstrumentGuitar = 3,
    kInstrumentVocal = 4,
    kInstrumentFx = 5,
};

// Configuration code of the jukebox caption for a level.
constexpr int kJukeboxCaptionConfigCode = 0x320;

// Script template a GameOverMsg runs when mUnknown44 is set.
constexpr int kGameOverScriptTemplate = 1001;

// Script template a JamEffectMsg runs in kPlayModeJam when mUnknown44 is set.
constexpr int kJamEffectScriptTemplate = 1017;

// Script templates other handlers run when mUnknown44 is set.
constexpr int kPhraseCapturedScriptTemplate = 1005;
constexpr int kLoopToggleScriptTemplate = 1011;
constexpr int kChoosePowerupScriptTemplate = 1016;
constexpr int kToggleGhostScriptTemplate = 1021;

// MIDI ticks in one bar, as the integer the handlers divide positions by.
constexpr int kTicksPerBarInt = 1920;

// Font scale and hold time, in milliseconds, of most handler messages.
constexpr float kMessageScale = 1.0f;
constexpr float kMessageHold = 1500.0f;

// Font scale and hold time of the win and lose messages.
constexpr float kResultScale = 2.0f;
constexpr float kResultHold = 3000.0f;

// Font scale of the powerup failure messages.
constexpr float kFailureScale = 0.8f;

// The multiplier OnWin() resets every readout to.
constexpr int kBaseMultiplier = 1;

// Two or more track displays split the result messages over two lines.
constexpr unsigned kTwoLineTrackCount = 2;

// The juice fraction above which a solo player's icon pulses.
constexpr float kPulseJuice = 0.85f;

// The song position, in MIDI ticks, after which a LoopToggleMsg shows its text.
constexpr float kLoopTextStart = -1000.0f;

// An erase shorter than this many bars reports a bar rather than a track.
constexpr int kTrackEraseBars = 2;

// The difficulty from which OnCatch() stops counting blocked catches.
constexpr int kNoRotateHintDifficulty = 2;

// OnCatch() suggests another track after this many blocked catches in a row.
constexpr int kRotateHintCatches = 3;

// The badge score change time that asks for a redraw on the next update.
constexpr float kScoreChangedNow = -1.0f;

} // namespace

Overlay *g_pOverlay;
HxStr g_hudLayoutName;

Overlay::Overlay(Renderer *pRenderer) : mPanel(nullptr), mRenderer(pRenderer) {
    mUnknown44 = QueryConfigFlag(kDisplayModeConfigCode);
    mPlaybackOn = 0;
    mCurrentBar = kNoCurrentBar;
    mGameMode = Application::shared()->GetGameMode();
    mPlayMode = Application::shared()->GetPlayMode();
    mLastBar = Application::shared()->GetPlayMap()->Slot9();

    std::vector<Player *> &players = Application::shared()->GetWorld()->mPlayers;
    int nLayout = 0;
    for (unsigned i = 0; i < players.size(); ++i) {
        if (players[i]->Slot2() != kNoPlayerSlot) {
            ++nLayout;
        }
    }
    if (nLayout == kThreePlayerCount) {
        nLayout = kFourPlayerLayout;
    }
    SetLayoutName(nLayout);
    mMsPerTick = static_cast<float>(Application::shared()->GetTempo()) / kTempoToMsPerTick;

    Rnd::View *pHud = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("hud.view")));
    pHud->RemoveView(dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("hud1.view"))));
    pHud->RemoveView(dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("hud2.view"))));
    pHud->RemoveView(dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("hud4.view"))));
    Rnd::View *pLayout =
        dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(FormatString("hud%d.view", nLayout))));
    pHud->AddView(pLayout);
    pLayout->SetRate(mMsPerTick);

    const char *pszLayoutName =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    pLayout->RemoveView(dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s test.anim", pszLayoutName)))));
    for (std::list<Rnd::Drawable *>::iterator it = pLayout->GetDraws().begin();
         it != pLayout->GetDraws().end();
         ++it) {
        if ((*it)->mName != "hud.cam" && (*it)->mName != "hud.env") {
            (*it)->SetShowing(0);
        }
    }

    int nTrack = 0;
    for (unsigned i = 0; i < players.size(); ++i) {
        if (players[i]->Slot2() != kNoPlayerSlot) {
            mTracks.push_back(new HudTrack(players[i], nTrack++));
        }
        mBadges.push_back(new HudBadge(players[i], i));
    }
    mPanel = new HudPanel();

    for (int i = 0; i < kTrackCount; ++i) {
        const int nInstrument = Application::shared()->GetLevel()->TrackAt(i)->mInstrument;
        const int nKind = Application::shared()->GetLevel()->TrackAt(i)->mKind;
        HxStr name;
        switch (nInstrument) {
        case kInstrumentDrums:
            name = "DRUMS";
            break;
        case kInstrumentBass:
            name = "BASS";
            break;
        case kInstrumentSynth:
            name = "SYNTH";
            break;
        case kInstrumentGuitar:
            name = "GUITAR";
            break;
        case kInstrumentVocal:
            name = "VOCAL";
            break;
        case kInstrumentFx:
            name = "FX";
            break;
        }
        if (nKind == kTrackModeAxe) {
            name = "AXE";
        } else if (nKind == kTrackModeScratch) {
            name = "SCRATCH";
        } else if (nKind == kTrackModeVocal) {
            name = "VOCAL";
        }
        mInstrumentNames.push_back(name);
        mTrackKinds.push_back(nKind);
    }

    if (Application::shared()->IsJukeboxMode()) {
        mPanel->mMessage.Show(HxStr("Jukebox Mode\nPress the START button to quit"));
        mPanel->mAssembly.Jump(0.0f);
        mPanel->mLetterbox.Jump(1.0f);

        HxStr songName(Application::shared()->GetWorld()->mSongName);
        const HxStr &levelName = Application::shared()->GetGameManager()->GetParams()->mLevelName;
        HxStr caption;
        QueryConfigString(&caption,
                          kJukeboxCaptionConfigCode,
                          levelName.mStr != nullptr ? levelName.mStr : g_szEmptyString);
        Rnd::Text *pLine = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr("HUD juke1.txt")));
        pLine->SetText(songName);
        pLine->SetShowing(1);
        pLine = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr("HUD juke2.txt")));
        pLine->SetText(caption);
        pLine->SetShowing(1);
    } else {
        mPanel->mAssembly.SetTarget(1.0f);
    }

    mDifficulty = Application::shared()->GetGameManager()->GetDifficulty();
    mPanel->mLabelSwap.Jump(0.0f);
    if (mPlayMode == kPlayModeJam) {
        for (std::vector<HudBadge *>::iterator it = mBadges.begin(); it != mBadges.end(); ++it) {
            (*it)->mFreq.SetPulsing(1);
        }
    }
    g_pOverlay = this;
}

Overlay::~Overlay() {
    g_pOverlay = nullptr;
    for (std::vector<HudBadge *>::iterator it = mBadges.begin(); it != mBadges.end(); ++it) {
        delete *it;
    }
    for (std::vector<HudTrack *>::iterator it = mTracks.begin(); it != mTracks.end(); ++it) {
        delete *it;
    }
    delete mPanel;
    g_gfxDevice.mFeedbackEnabled = 0;
}

void Overlay::SetFrame(float flFrame) {
    const int nBar = static_cast<int>(flFrame / kTicksPerBar);
    bool bBarChanged = false;
    if (nBar != mCurrentBar) {
        mCurrentBar = nBar;
        bBarChanged = true;
    }

    const float flTime = flFrame * mMsPerTick;
    for (std::vector<HudBadge *>::iterator it = mBadges.begin(); it != mBadges.end(); ++it) {
        (*it)->SetFrame(flFrame, flTime);
    }
    for (std::vector<HudTrack *>::iterator it = mTracks.begin(); it != mTracks.end(); ++it) {
        (*it)->SetFrame(flFrame, flTime);
        if (bBarChanged) {
            (*it)->mEffects.SetMask(mRenderer->GetCell((*it)->mTrack, mCurrentBar)->mEffects);
        }
    }
    mPanel->SetFrame(flFrame, flTime);

    if (Application::shared()->GetGameManager()->IsPlaybackActive() != 0) {
        mPanel->mMessage.Show(HxStr("DEMO\n\nPress any button to exit"));
    }
}

void Overlay::OnBarChanged(int nTrack, int nBar, BarStatusMsg::Effects effects) {
    if (nBar != mCurrentBar) {
        return;
    }

    for (std::vector<HudTrack *>::iterator it = mTracks.begin(); it != mTracks.end(); ++it) {
        if ((*it)->mTrack == nTrack) {
            (*it)->mEffects.SetMask(effects);
        }
    }
}

inline HudTrack *Overlay::FindTrack(Player *pPlayer) {
    for (std::vector<HudTrack *>::iterator it = mTracks.begin(); it != mTracks.end(); ++it) {
        if ((*it)->mPlayer == pPlayer) {
            return *it;
        }
    }
    return nullptr;
}

void Overlay::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(pMsg);
    } else if (nType == g_nGameOverMsgType) {
        OnGameOver();
    } else if (nType == g_nWinMsgType) {
        OnWin(pMsg);
    } else if (nType == g_nChoosePowerupMsgType) {
        OnChoosePowerup(pMsg);
    } else if (nType == g_nPowerupCountMsgType) {
        // Yes, the binary recognises a PowerupCountMsg and does nothing with it.
    } else if (nType == g_nCaughtPowerbarMsgType) {
        OnCaughtPowerbar(pMsg);
    } else if (nType == g_nDeployedPowerupMsgType) {
        OnDeployedPowerup(pMsg);
    } else if (nType == g_nPointAmountMsgType) {
        OnPointAmount(pMsg);
    } else if (nType == g_nJuiceAmountMsgType) {
        OnJuiceAmount(pMsg);
    } else if (nType == g_nPhraseCapturedMsgType) {
        OnPhraseCaptured(pMsg);
    } else if (nType == g_nTextMsgType) {
        OnText(pMsg);
    } else if (nType == g_nLoopToggleMsgType) {
        OnLoopToggle(pMsg);
    } else if (nType == g_nAdvanceSectionToggleMsgType) {
        OnAdvanceSectionToggle(pMsg);
    } else if (nType == g_nShowEraseEffectMsgType) {
        OnShowEraseEffect(pMsg);
    } else if (nType == g_nPlaybackToggleMsgType) {
        OnPlaybackToggle(pMsg);
    } else if (nType == g_nToggleGhostMsgType) {
        OnToggleGhost(pMsg);
    } else if (nType == g_nJamEffectMsgType) {
        OnJamEffect();
    } else if (nType == g_nCatchMsgType) {
        OnCatch(pMsg);
    } else if (nType == g_nPhraseMuffedMsgType) {
        OnPhraseMuffed(pMsg);
    } else if (nType == g_nBeginPhraseCatchMsgType) {
        OnBeginPhraseCatch(pMsg);
    } else if (nType == g_nFadeGameMsgType) {
        OnFadeGame(pMsg);
    } else if (nType == g_nPlayersTrackNeutralizedMsgType) {
        OnPlayersTrackNeutralized(pMsg);
    } else if (nType == g_nMultiplierStateMsgType) {
        OnMultiplierState(pMsg);
    } else if (nType == g_nPowerupFailedMsgType) {
        OnPowerupFailed(pMsg);
    }
}

void Overlay::OnTrackSelect(Message *pMsg) {
    TrackSelectMsg *pSelect = static_cast<TrackSelectMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pSelect->mUnknown10);
    if (pTrack == nullptr) {
        return;
    }

    const int nBar = static_cast<int>(mRenderer->mSongTick) / kTicksPerBarInt;
    const int nTrack = pSelect->mUnknown04;
    pTrack->mTrackLabel.SetText(mInstrumentNames[nTrack]);
    pTrack->mTrack = nTrack;
    pTrack->mEffects.SetMask(mRenderer->GetCell(nTrack, nBar)->mEffects);
    if (mUnknown44 == 0) {
        pTrack->mPoints.Bank();
    }
}

void Overlay::OnWin(Message *pMsg) {
    WinMsg *pWin = static_cast<WinMsg *>(pMsg);
    for (std::vector<HudTrack *>::iterator it = mTracks.begin(); it != mTracks.end(); ++it) {
        (*it)->mPoints.SetMultiplier(kBaseMultiplier);
    }

    if (GetDoWinSequence() != 0 && mGameMode == kGameModeSolo && pWin->mWinners.size() != 0) {
        mPanel->mWinMessage.mState = HudWinMessage::kStateStart;
        return;
    }

    HxStr separator(" ");
    if (mTracks.size() >= kTwoLineTrackCount) {
        separator = "\n";
    }
    for (std::vector<HudTrack *>::iterator it = mTracks.begin(); it != mTracks.end(); ++it) {
        HudTrack *pTrack = *it;
        if (std::find(pWin->mWinners.begin(), pWin->mWinners.end(), pTrack->mPlayer) !=
            pWin->mWinners.end()) {
            pTrack->mTextMessage.Show(HxStr("YOU") + separator + "WIN", kResultScale, kResultHold);
            if (mGameMode == kGameModeSolo) {
                mPanel->mMessage.Show(HxStr("FREESTYLE\n\nPress the START button to exit"));
            }
        } else if (mGameMode == kGameModeSolo) {
            pTrack->mTextMessage.Show(
                HxStr("GAME") + separator + "OVER", kResultScale, kResultHold);
        } else {
            pTrack->mTextMessage.Show(HxStr("YOU") + separator + "LOSE", kResultScale, kResultHold);
        }
        if (pTrack->mTextMessage.mText->GetShowing() != 0) {
            pTrack->mTextMessage.mActive = 1;
        }
    }
}

void Overlay::OnChoosePowerup(Message *pMsg) {
    ChoosePowerupMsg *pChoose = static_cast<ChoosePowerupMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pChoose->mOwner);
    if (pTrack == nullptr) {
        return;
    }

    if (mPlayMode == kPlayModeGame) {
        pTrack->mPowerup.Show(pChoose->mType);
    } else {
        pTrack->mEffects.Select(pChoose->mType);
    }
    if (mUnknown44 != 0) {
        CallScriptTemplate(kChoosePowerupScriptTemplate, pChoose->mType);
    }
}

void Overlay::OnCaughtPowerbar(Message *pMsg) {
    CaughtPowerbarMsg *pCaught = static_cast<CaughtPowerbarMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pCaught->mPlayer);
    if (pTrack == nullptr) {
        return;
    }

    const HxStr text = HudPowerupName(pCaught->mKind) + "\nCAPTURED";
    pTrack->mTextMessage.Show(text, kMessageScale, kMessageHold);
}

void Overlay::OnDeployedPowerup(Message *pMsg) {
    if (mPlayMode != kPlayModeGame) {
        return;
    }

    DeployedPowerupMsg *pDeployed = static_cast<DeployedPowerupMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pDeployed->mPlayer);
    if (pTrack == nullptr) {
        return;
    }

    const HxStr text = HudPowerupName(pDeployed->mKind) + "\nDEPLOYED";
    pTrack->mTextMessage.Show(text, kMessageScale, kMessageHold);
    pTrack->mUnknownec = 1;
    if (pDeployed->mKind == kHudItemBumper) {
        HudTrack *pTarget = FindTrack(pDeployed->mTarget);
        if (pTarget != nullptr) {
            pTarget->mTextMessage.Show(HxStr("YOU GOT\nBUMPED!"), kMessageScale, kMessageHold);
        }
    }
}

void Overlay::OnPointAmount(Message *pMsg) {
    PointAmountMsg *pPoints = static_cast<PointAmountMsg *>(pMsg);
    HudBadge *pBadge = FindBadge(pPoints->mPlayer);
    if (pBadge == nullptr) {
        return;
    }

    pBadge->mScore.mScore = pPoints->GetScore();
    pBadge->mScore.mChangeTime = kScoreChangedNow;
}

void Overlay::OnJuiceAmount(Message *pMsg) {
    if (mGameMode != kGameModeSolo || mPlayMode != kPlayModeGame) {
        return;
    }

    JuiceAmountMsg *pJuice = static_cast<JuiceAmountMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pJuice->mUnknown04);
    if (pTrack == nullptr) {
        return;
    }

    const float flLevel = pJuice->GetJuiceFraction();
    (void)pJuice->GetJuice(); // Yes, the binary discards this call's result.
    pTrack->mEnergy.mLevel = flLevel;
    // Yes, the binary does not test the badge for null.
    FindBadge(pJuice->mUnknown04)->mFreq.SetPulsing(pJuice->GetJuiceFraction() > kPulseJuice);
}

void Overlay::OnPhraseCaptured(Message *pMsg) {
    PhraseCapturedMsg *pCaptured = static_cast<PhraseCapturedMsg *>(pMsg);
    if (mUnknown44 != 0) {
        CallScriptTemplate(kPhraseCapturedScriptTemplate, pCaptured->mTrack);
        return;
    }
    if (mPlayMode != kPlayModeGame) {
        return;
    }
    if (static_cast<float>(mLastBar * kTicksPerBarInt) <= mRenderer->mSongTick) {
        return;
    }

    HudTrack *pTrack = FindTrack(pCaptured->mPlayer);
    if (pTrack != nullptr && pCaptured->mScore != 0) {
        pTrack->mPoints.ShowExit(pCaptured->mScore);
    }
}

void Overlay::OnText(Message *pMsg) {
    // Yes, the binary reads the first display without testing for an empty vector.
    HudTrack *pTrack = mTracks[0];
    if (pTrack == nullptr) {
        return;
    }

    pTrack->mTextMessage.Show(
        HxStr(static_cast<TextMsg *>(pMsg)->mText), kMessageScale, kMessageHold);
}

void Overlay::OnLoopToggle(Message *pMsg) {
    if (mPlayMode == kPlayModeGame) {
        return;
    }

    LoopToggleMsg *pLoop = static_cast<LoopToggleMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pLoop->mPlayer);
    if (pTrack == nullptr) {
        return;
    }

    if (mUnknown44 != 0) {
        CallScriptTemplate(kLoopToggleScriptTemplate);
    }
    pTrack->mLoop.SetShowing(pLoop->mOn);
    if (!(mRenderer->mSongTick > kLoopTextStart)) {
        return;
    }

    if (pLoop->mOn != 0) {
        pTrack->mTextMessage.Show(HxStr("LOOP ON"), kMessageScale, kMessageHold);
    } else {
        pTrack->mTextMessage.Show(HxStr("LOOP OFF"), kMessageScale, kMessageHold);
    }
}

void Overlay::OnAdvanceSectionToggle(Message *pMsg) {
    if (mUnknown44 != 0) {
        return;
    }

    mPanel->mPosition.Update();
    if (mPlaybackOn != 0) {
        return;
    }

    HxStr text;
    if (static_cast<AdvanceSectionToggleMsg *>(pMsg)->mAdvance != 0) {
        text = "ADVANCE TO\nNEXT SECTION";
    } else {
        text = "REPEAT\nSECTION";
    }
    for (std::vector<HudTrack *>::iterator it = mTracks.begin(); it != mTracks.end(); ++it) {
        (*it)->mTextMessage.Show(text, kMessageScale, kMessageHold);
    }
}

void Overlay::OnShowEraseEffect(Message *pMsg) {
    ShowEraseEffectMsg *pErase = static_cast<ShowEraseEffectMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pErase->mPlayer);
    if (pTrack == nullptr) {
        return;
    }

    if (pErase->mEndBar - pErase->mFirstBar < kTrackEraseBars) {
        pTrack->mTextMessage.Show(HxStr("BAR ERASED"), kMessageScale, kMessageHold);
    } else {
        pTrack->mTextMessage.Show(HxStr("TRACK ERASED"), kMessageScale, kMessageHold);
    }
}

void Overlay::OnPlaybackToggle(Message *pMsg) {
    PlaybackToggleMsg *pPlayback = static_cast<PlaybackToggleMsg *>(pMsg);
    mPlaybackOn = pPlayback->mOn;
    if (pPlayback->mOn != 0) {
        mPanel->mMessage.Show(HxStr("Press the SELECT button to edit"));
        mPanel->mAssembly.SetTarget(0.0f);
        mPanel->mLetterbox.SetTarget(1.0f);
    } else {
        mPanel->mMessage.Hide();
        mPanel->mAssembly.SetTarget(1.0f);
        mPanel->mLetterbox.SetTarget(0.0f);
    }
    for (std::vector<HudBadge *>::iterator it = mBadges.begin(); it != mBadges.end(); ++it) {
        // Yes, the binary flips the low bit rather than testing for zero.
        (*it)->mFreq.SetShowing(pPlayback->mOn ^ 1);
    }
    for (std::vector<HudTrack *>::iterator it = mTracks.begin(); it != mTracks.end(); ++it) {
        (*it)->mTextMessage.Hide();
    }
}

void Overlay::OnToggleGhost(Message *pMsg) {
    ToggleGhostMsg *pGhost = static_cast<ToggleGhostMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pGhost->mUnknown04);
    if (pTrack == nullptr || mPlayMode == kPlayModeGame) {
        return;
    }

    pTrack->mEffects.SetLit(kHudItemGuides, pGhost->mOn);
    if (mUnknown44 != 0) {
        CallScriptTemplate(kToggleGhostScriptTemplate);
    }
}

void Overlay::OnCatch(Message *pMsg) {
    if (mPlayMode != kPlayModeGame) {
        return;
    }
    if (static_cast<float>(mLastBar * kTicksPerBarInt) <= mRenderer->mSongTick) {
        return;
    }

    CatchMsg *pCatch = static_cast<CatchMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pCatch->mPlayer);
    if (pTrack == nullptr) {
        return;
    }

    if (mUnknown44 == 0 && pCatch->mTotal != 0 && pCatch->mCaught + 1 < pCatch->mTotal) {
        pTrack->mPoints.Pulse(static_cast<float>(pCatch->mCaught) /
                              static_cast<float>(pCatch->mTotal));
    }
    if (mDifficulty >= kNoRotateHintDifficulty || mGameMode != kGameModeSolo || mUnknown44 != 0) {
        return;
    }

    const int nBar = pCatch->mTick / Mid::MBT(kTicksPerBarInt).mTick;
    const Renderer::Cell *pCell = mRenderer->GetCell(pCatch->mTrack, nBar);
    if (pCatch->mHit != 0) {
        pTrack->mUnknowne0 = 0;
    } else if (pCell->mEnabled == 0 || pCell->mPlayer->IsNull() == 0) {
        ++pTrack->mUnknowne0;
    } else {
        pTrack->mUnknowne0 = 0;
    }
    if (pTrack->mUnknowne0 < kRotateHintCatches) {
        return;
    }

    pTrack->mTextMessage.Show(HxStr("ROTATE TO\nNEW TRACK"), kMessageScale, kMessageHold);
    pTrack->mUnknowne0 = 0;
}

void Overlay::OnPhraseMuffed(Message *pMsg) {
    if (mPlayMode != kPlayModeGame || mUnknown44 != 0) {
        return;
    }

    HudTrack *pTrack = FindTrack(static_cast<PhraseMuffedMsg *>(pMsg)->mPlayer);
    if (pTrack != nullptr) {
        pTrack->mPoints.Bank();
    }
}

void Overlay::OnBeginPhraseCatch(Message *pMsg) {
    if (mPlayMode != kPlayModeGame) {
        return;
    }
    if (static_cast<float>(mLastBar * kTicksPerBarInt) <= mRenderer->mSongTick) {
        return;
    }
    if (mUnknown44 != 0) {
        return;
    }

    BeginPhraseCatchMsg *pBegin = static_cast<BeginPhraseCatchMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pBegin->mPlayer);
    if (pTrack == nullptr || pBegin->mPoints == 0) {
        return;
    }

    pTrack->mPoints.SetPoints(pBegin->mPoints);
    pTrack->mPoints.SetMultiplier(pBegin->mMultiplier);
}

void Overlay::OnFadeGame(Message *pMsg) {
    FadeGameMsg *pFade = static_cast<FadeGameMsg *>(pMsg);
    mPanel->mScreenFlash.Start(static_cast<float>(pFade->mDuration), pFade->mFadeIn);
    if (pFade->mFadeIn == 0) {
        mPanel->mWinMessage.HidePrompt();
    }
}

void Overlay::OnPlayersTrackNeutralized(Message *pMsg) {
    PlayersTrackNeutralizedMsg *pNeutralized = static_cast<PlayersTrackNeutralizedMsg *>(pMsg);
    // Yes, the binary does not test the display for null.
    FindTrack(pNeutralized->mPlayer)
        ->mTextMessage.Show(HxStr(FormatString("NEUTRALIZED!\n%d POINTS", pNeutralized->mPoints)),
                            kMessageScale,
                            kMessageHold);
}

void Overlay::OnMultiplierState(Message *pMsg) {
    if (mUnknown44 != 0) {
        return;
    }
    if (static_cast<float>(mLastBar * kTicksPerBarInt) <= mRenderer->mSongTick) {
        return;
    }

    MultiplierStateMsg *pState = static_cast<MultiplierStateMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pState->mPlayer);
    if (pTrack == nullptr) {
        return;
    }

    pTrack->mPoints.SetMultiplier(pState->mMultiplier + pState->mBonus);
    pTrack->mPoints.mHot = pState->mBonus != 0;
}

void Overlay::OnPowerupFailed(Message *pMsg) {
    PowerupFailedMsg *pFailed = static_cast<PowerupFailedMsg *>(pMsg);
    HudTrack *pTrack = FindTrack(pFailed->mPlayer);
    if (pTrack == nullptr) {
        return;
    }

    HxStr text;
    switch (pFailed->mKind) {
    case kHudItemNeutralizer:
        text = "NEUTRALIZER FAILED\nUSE ON\nCAPTURED TRACK";
        break;
    case kHudItemCrippler:
        text = "CRIPPLER FAILED\nUSE ON\nOTHER PLAYER";
        break;
    case kHudItemFreestyler:
        text = "FREESTYLER FAILED\nUSE ON\nFREESTYLE TRACK";
        break;
    case kHudItemAutocatcher:
        text = "AUTOCATCHER FAILED\nUSE ON\nFREE TRACK";
        break;
    case kHudItemBumper:
        text = "BUMPER FAILED\nUSE ON\nOTHER PLAYER";
        break;
    default:
        break;
    }
    pTrack->mTextMessage.Show(text, kFailureScale, kMessageHold);
}

void Overlay::OnGameOver() {
    if (mUnknown44 != 0) {
        CallScriptTemplate(kGameOverScriptTemplate);
    }
}

void Overlay::OnJamEffect() {
    if (mUnknown44 != 0 && Application::shared()->GetPlayMode() == kPlayModeJam) {
        CallScriptTemplate(kJamEffectScriptTemplate);
    }
}

void Overlay::Draw() {
    mPanel->mWinMessage.Draw();
}

void Overlay::OnLeaderChanged(Player *pOldLeader, Player *pNewLeader) {
    if (pNewLeader == nullptr) {
        mPanel->mScorePulse.Hide();
    } else {
        mPanel->mScorePulse.MoveTo(FindBadge(pNewLeader));
    }
    if (pOldLeader != nullptr) {
        FindBadge(pOldLeader)->mFreq.SetPulsing(0);
    }
    if (pNewLeader != nullptr) {
        FindBadge(pNewLeader)->mFreq.SetPulsing(1);
    }
}

HudBadge *Overlay::FindBadge(Player *pPlayer) {
    for (std::vector<HudBadge *>::iterator it = mBadges.begin(); it != mBadges.end(); ++it) {
        if ((*it)->mPlayer == pPlayer) {
            return *it;
        }
    }
    return nullptr;
}

void Overlay::SetLayoutName(int nLayout) {
    g_hudLayoutName = FormatString("HUD%d", nLayout);
}
