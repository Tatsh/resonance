#include "met/metmultisaveremixscreen.h"

#include <vector>

#include "app/application.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "game/globalsettings.h"
#include "game/inputpoller.h"
#include "memcard/memcardconnectstate.h"
#include "met/metfrontendstate.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "met/metsaveremixscreen.h"
#include "met/metsonglists.h"
#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "dlg";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix. MetGlobalSettingsSaverScreen and MetRemixManager
// load the same container.
static const char *const kContainerName = "dialogue";

// Screens the class pushes, exits, and returns to by registry key.
static const char *const kEndRemixScreen = "MetMultiEndRemixScreen";
static const char *const kRemixTypeScreen = "MetRemixTypeScreen";
#ifdef VIDEO_STANDARD_PAL
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kHelpScreen = "MetHelpScreen";
#endif

// The packed port and slot of the first slot of port 1, which EnterAndShow() credits to player 1.
constexpr int kSecondPortFirstSlot = 0x100;
#ifndef VIDEO_STANDARD_PAL
constexpr int kSecondPortPlayer = 1;
#endif
// The mark EnterAndShow() records for a player whose card is ready.
constexpr int kCardReady = 1;

#ifdef VIDEO_STANDARD_PAL
// The location each save goes to. The first save uses the first slot of port 0, and the save
// index selects the multitap slot of port 0 after that. Without a multitap the second save uses
// port 1.
constexpr int kFirstPortSlot = 0;
constexpr int kSecondSave = 1;
constexpr int kThirdSave = 2;
constexpr int kFourthSave = 3;
static const char *const kSecondPortSlotName = "2";
static const char *const kMultitapSlotBName = "1-B";
static const char *const kMultitapSlotCName = "1-C";
static const char *const kMultitapSlotDName = "1-D";
#endif

// The controller pads are numbered from one, the players from zero.
constexpr int kFirstPad = 1;

// Slot 36 clears the entered name for the first save, and slot 2 keeps it for the rest.
constexpr int kClearSaveName = 1;
constexpr int kKeepSaveName = 0;

#ifndef VIDEO_STANDARD_PAL
// ReturnToRemixType() lets the renderer resolve the arena view rather than skipping it.
constexpr int kResolveArenaView = 0;
#endif

} // namespace

MetMultiSaveRemixScreen::MetMultiSaveRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mSaveCount(0) {
}

MetMultiSaveRemixScreen::~MetMultiSaveRemixScreen() {
}

void MetMultiSaveRemixScreen::EnterAndShow() {
    SetShowing(0);
    PushNamedScreen(HxStr(kEndRemixScreen));
    mRenderer->SetActivePanel(this);

    mPlayerCount = static_cast<int>(MetFrontEndState::shared()->mPersonas.size());
    mSaveCount = 0;
    for (int nPlayer = kMaxPlayers - 1; nPlayer >= 0; --nPlayer) {
        mCardReady[nPlayer] = 0;
    }

#ifdef VIDEO_STANDARD_PAL
    mSaveCount = mPlayerCount;
    for (int nPlayer = 0; nPlayer < mSaveCount; ++nPlayer) {
        mCardReady[nPlayer] = kCardReady;
    }
#else
    for (std::vector<MemcardConnectState>::size_type nSlot = 0;
         nSlot < GlobalSettings::shared()->mCardSlots.size();
         ++nSlot) {
        const MemcardConnectState state = GlobalSettings::shared()->mCardSlots[nSlot];
        if (state.mFormatted == 0) {
            continue;
        }
        if (state.mPortSlot < kSecondPortFirstSlot && state.mPortSlot < mPlayerCount) {
            mCardReady[state.mPortSlot] = kCardReady;
            ++mSaveCount;
        } else if (state.mPortSlot == kSecondPortFirstSlot) {
            mCardReady[kSecondPortPlayer] = kCardReady;
            ++mSaveCount;
        }
    }
#endif

    if (mSaveCount == 0) {
        ExitScreenByName(HxStr(kEndRemixScreen));
    }
    BeginExit();
}

void MetMultiSaveRemixScreen::OnExitFinished() {
    if (mSaveCount == 0) {
#ifdef VIDEO_STANDARD_PAL
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kHelpScreen));
#endif
        ReturnToRemixType();
        return;
    }

    mReadyPlayers.clear();
    std::vector<FreqAppearance> appearances;
    appearances.reserve(mPlayerCount);
    for (int nPlayer = 0; nPlayer < mPlayerCount; ++nPlayer) {
        if (mCardReady[nPlayer] == kCardReady) {
            mReadyPlayers.push_back(nPlayer);
        }
        appearances.push_back(MetFrontEndState::shared()->mPersonas[nPlayer]->mAppearance);
    }

    const int nFirstPlayer = mReadyPlayers[0];
    MetPersonaData *pPersona = MetFrontEndState::shared()->mPersonas[mReadyPlayers[0]];
#ifdef VIDEO_STANDARD_PAL
    MemcardConnectState slot;
    slot.mPortSlot = kFirstPortSlot;
    slot.mSlotName = FirstCardSlotName();
    MetSaveRemixScreen::Open(
        pPersona, nFirstPlayer + kFirstPad, this, slot, appearances, kClearSaveName);
#else
    (void)GlobalSettings::shared(); // Yes, the binary discards this call's result.
    MetSaveRemixScreen::Open(pPersona,
                             nFirstPlayer + kFirstPad,
                             this,
                             GlobalSettings::shared()->mCardSlots[0],
                             appearances,
                             kClearSaveName);
#endif
    mEndScreenExited = 0;
    mSaveIndex = 0;
}

void MetMultiSaveRemixScreen::OnSaveFinished(int) {
    ++mSaveIndex;
    if (static_cast<std::vector<int>::size_type>(mSaveIndex) == mReadyPlayers.size()) {
        if (mEndScreenExited == 0) {
            ExitScreenByName(HxStr(kEndRemixScreen));
        }
        ReturnToRemixType();
        return;
    }

    const int nPlayer = mReadyPlayers[mSaveIndex];
    MetPersonaData *pPersona = MetFrontEndState::shared()->mPersonas[nPlayer];
    if (mEndScreenExited != 0) {
        PushNamedScreen(HxStr(kEndRemixScreen));
        mEndScreenExited = 0;
    }

    std::vector<FreqAppearance> appearances;
    appearances.reserve(mPlayerCount);
    for (int nIndex = 0; nIndex < mPlayerCount; ++nIndex) {
        appearances.push_back(MetFrontEndState::shared()->mPersonas[nIndex]->mAppearance);
    }
#ifdef VIDEO_STANDARD_PAL
    MemcardConnectState slot;
    switch (mSaveIndex) {
    case kSecondSave:
        if (Application::shared()->GetGameManager()->GetPoller()->GetMultitap0()) {
            slot.mSlotName = kMultitapSlotBName;
            slot.mPortSlot = mSaveIndex;
        } else {
            slot.mSlotName = kSecondPortSlotName;
            slot.mPortSlot = kSecondPortFirstSlot;
        }
        break;
    case kThirdSave:
        slot.mSlotName = kMultitapSlotCName;
        slot.mPortSlot = mSaveIndex;
        break;
    case kFourthSave:
        slot.mSlotName = kMultitapSlotDName;
        slot.mPortSlot = mSaveIndex;
        break;
    default:
        break;
    }
    MetSaveRemixScreen::Open(pPersona, nPlayer + kFirstPad, this, slot, appearances, kKeepSaveName);
#else
    // Yes, the binary picks the card slot by the save index rather than by the player.
    MetSaveRemixScreen::Open(pPersona,
                             nPlayer + kFirstPad,
                             this,
                             GlobalSettings::shared()->mCardSlots[mSaveIndex],
                             appearances,
                             kKeepSaveName);
#endif
}

void MetMultiSaveRemixScreen::SetOwnerScreenShowing(int bShowing) {
    mEndScreenExited = bShowing ^ 1;
    if (bShowing != 0) {
        PushNamedScreen(HxStr(kEndRemixScreen));
    } else {
        ExitScreenByName(HxStr(kEndRemixScreen));
    }
}

void MetMultiSaveRemixScreen::ReturnToRemixType() {
#ifndef VIDEO_STANDARD_PAL
    mRenderer->ResolveArenaView(kResolveArenaView);
    mRenderer->OnReturnFromGame();
    mRenderer->OnReturnToMenus();
#endif
    PushNamedScreen(HxStr(kRemixTypeScreen));
    ActivateNamedPanel(HxStr(kRemixTypeScreen));
}

MetMultiSaveRemixScreen *MetMultiSaveRemixScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiSaveRemixScreen(pRenderer, nPriority);
}

void MetMultiSaveRemixScreen::OnHelpRequested() {
    mEndScreenExited = 1;
    ExitScreenByName(HxStr(kEndRemixScreen));
}
