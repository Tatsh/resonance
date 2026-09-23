#include "met/metmultisaveremixscreen.h"

#include <vector>

#include "game/freqappearance.h"
#include "game/globalsettings.h"
#include "memcard/memcardconnectstate.h"
#include "met/metfrontendstate.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "met/metsaveremixscreen.h"
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

// The packed port and slot of the first slot of port 1, which EnterAndShow() credits to player 1.
constexpr int kSecondPortFirstSlot = 0x100;
constexpr int kSecondPortPlayer = 1;
// The mark EnterAndShow() records for a player whose card is ready.
constexpr int kCardReady = 1;

// The controller pads are numbered from one, the players from zero.
constexpr int kFirstPad = 1;

// Slot 36 clears the entered name for the first save, and slot 2 keeps it for the rest.
constexpr int kClearSaveName = 1;
constexpr int kKeepSaveName = 0;

// ReturnToRemixType() lets the renderer resolve the arena view rather than skipping it.
constexpr int kResolveArenaView = 0;

} // namespace

// 0x002fa0b0
MetMultiSaveRemixScreen::MetMultiSaveRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mSaveCount(0) {
}

// 0x002fa280
MetMultiSaveRemixScreen::~MetMultiSaveRemixScreen() {
}

// 0x002fa4e8
void MetMultiSaveRemixScreen::EnterAndShow() {
    SetShowing(0);
    PushNamedScreen(HxStr(kEndRemixScreen));
    mUnknown10->SetActivePanel(this);

    mPlayerCount = static_cast<int>(MetFrontEndState::shared()->mUnknown00.size());
    mSaveCount = 0;
    for (int nPlayer = kMaxPlayers - 1; nPlayer >= 0; --nPlayer) {
        mCardReady[nPlayer] = 0;
    }

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

    if (mSaveCount == 0) {
        ExitScreenByName(HxStr(kEndRemixScreen));
    }
    BeginExit();
}

// 0x002fa7c0
void MetMultiSaveRemixScreen::OnUnknownSlot36() {
    if (mSaveCount == 0) {
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
        appearances.push_back(MetFrontEndState::shared()->mUnknown00[nPlayer]->mUnknown140);
    }

    const int nFirstPlayer = mReadyPlayers[0];
    MetPersonaData *pPersona = MetFrontEndState::shared()->mUnknown00[mReadyPlayers[0]];
    (void)GlobalSettings::shared(); // Yes, the binary discards this call's result.
    MetSaveRemixScreen::Open(pPersona,
                             nFirstPlayer + kFirstPad,
                             this,
                             GlobalSettings::shared()->mCardSlots[0],
                             appearances,
                             kClearSaveName);
    mUnknowne8 = 0;
    mSaveIndex = 0;
}

// 0x002facc0
void MetMultiSaveRemixScreen::OnUnknownSlot2(int) {
    ++mSaveIndex;
    if (static_cast<std::vector<int>::size_type>(mSaveIndex) == mReadyPlayers.size()) {
        if (mUnknowne8 == 0) {
            ExitScreenByName(HxStr(kEndRemixScreen));
        }
        ReturnToRemixType();
        return;
    }

    const int nPlayer = mReadyPlayers[mSaveIndex];
    MetPersonaData *pPersona = MetFrontEndState::shared()->mUnknown00[nPlayer];
    if (mUnknowne8 != 0) {
        PushNamedScreen(HxStr(kEndRemixScreen));
        mUnknowne8 = 0;
    }

    std::vector<FreqAppearance> appearances;
    appearances.reserve(mPlayerCount);
    for (int nIndex = 0; nIndex < mPlayerCount; ++nIndex) {
        appearances.push_back(MetFrontEndState::shared()->mUnknown00[nIndex]->mUnknown140);
    }
    // Yes, the binary picks the card slot by the save index rather than by the player.
    MetSaveRemixScreen::Open(pPersona,
                             nPlayer + kFirstPad,
                             this,
                             GlobalSettings::shared()->mCardSlots[mSaveIndex],
                             appearances,
                             kKeepSaveName);
}

// 0x002fb248
void MetMultiSaveRemixScreen::OnUnknownSlot4(int nFlag) {
    mUnknowne8 = nFlag ^ 1;
    if (nFlag != 0) {
        PushNamedScreen(HxStr(kEndRemixScreen));
    } else {
        ExitScreenByName(HxStr(kEndRemixScreen));
    }
}

// 0x002fb350
void MetMultiSaveRemixScreen::ReturnToRemixType() {
    mUnknown10->ResolveArenaView(kResolveArenaView);
    mUnknown10->OnUnknown00390088();
    mUnknown10->OnUnknown00390090();
    PushNamedScreen(HxStr(kRemixTypeScreen));
    ActivateNamedPanel(HxStr(kRemixTypeScreen));
}

// 0x002fedc0
MetMultiSaveRemixScreen *MetMultiSaveRemixScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiSaveRemixScreen(pRenderer, nPriority);
}

// 0x002fee48
void MetMultiSaveRemixScreen::OnUnknownSlot3() {
    mUnknowne8 = 1;
    ExitScreenByName(HxStr(kEndRemixScreen));
}
