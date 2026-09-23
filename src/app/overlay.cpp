#include "app/overlay.h"

#include "app/application.h"
#include "app/hudbadge.h"
#include "app/hudpanel.h"
#include "app/hudtrack.h"
#include "app/renderer.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/grooveworld.h"
#include "game/leveldata.h"
#include "game/player.h"
#include "game/playmap.h"
#include "game/trackdata.h"
#include "gfx/gfxdevice.h"
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

} // namespace

Overlay *g_pOverlay;
HxStr g_hudLayoutName;

Overlay::Overlay(Renderer *pRenderer) : mPanel(nullptr), mRenderer(pRenderer) {
    mUnknown44 = QueryConfigFlag(kDisplayModeConfigCode);
    mUnknown48 = 0;
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

    mDifficulty = Application::shared()->GetGameManager()->GetUnknown88();
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
