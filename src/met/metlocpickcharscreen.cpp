#include "met/metlocpickcharscreen.h"

#include <list>
#include <vector>

#include "app/application.h"
#include "game/controllerconfig.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "game/globalsettings.h"
#include "memcard/memcardconnectstate.h"
#include "memcard/memcardmanager.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metmsgscreen.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "os/r250.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/tex.h"
#include "rnd/text.h"
#include "rnd/view.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mpc";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Local";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "character_loc";
// The help text key.
static const char *const kHelpKey = "loc_pc";

// Per-player object names, formatted with the player counted from 1.
static const char *const kCharacterMatFormat = "char_0%d.mat";
static const char *const kSelectViewFormat = "sel_char_0%d.view";
static const char *const kNameButtonFormat = "mpc_Name0%d.but";
static const char *const kControllerMeshFormat = "mpc_freqcontroller0%d.mesh";
static const char *const kDefaultIconMat = "default_controller_icon.mat";
static const char *const kCustomIconMat = "custom_controller_icon.mat";

// The layout view and enter animation for a player count.
static const char *const kLayoutViewFormat = "character_loc_%dp.view";
static const char *const kEnterAnimFormat = "mpc_%d_EE.anim";

// The label of a player whose persona has no username.
static const char *const kPlayerLabelFormat = "player %d";

// Title keys.
static const char *const kMultiTitleKey = "multi";
static const char *const kPickTitleKey = "mp_char";

// Dialogue names, keys, titles, and button labels.
static const char *const kDetectMultiKey = "mem_detect_multi";
static const char *const kDetectMessage = "mem_detect";
static const char *const kNoCardMessage = "mem_check";
static const char *const kLoadKey = "mem_load";
static const char *const kLoadMessage = "load";
static const char *const kWarningTitle = "WARNING";
static const char *const kLoadingTitle = "LOADING";
static const char *const kRetryButton = "RETRY";
static const char *const kContinueButton = "CONTINUE";

static const char *const kLocNumPlayScreen = "MetLocNumPlayScreen";
static const char *const kLocNumPlayersScreen = "MetLocNumPlayersScreen";
static const char *const kModeScreen = "MetModeScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kMsgScreen = "MetMsgScreen";
static const char *const kOwnScreenName = "MetLocPickCharScreen";
static const char *const kNoName = "";

// Configuration codes the dialogue texts and the titles are read under.
constexpr int kDialogueConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// Button counts MetMsgScreen receives with each dialogue.
constexpr int kNoButtons = 0;
constexpr int kTwoButtons = 2;

// The most players the screen resolves objects for.
constexpr int kMaxPlayers = 4;

// The material stage that shows a player's persona burn texture.
constexpr int kBurnStage = 1;

// The Rnd::Button states the screen uses.
constexpr int kButtonStateNormal = 0;
constexpr int kButtonStatePressed = 2;

// The select view's showing flag while a player's character is locked in.
constexpr int kLockedShowing = 1;

// What MetScreen::mExitChoice records for slot 36 to act on.
constexpr int kExitBack = 0;
constexpr int kExitForward = 2;

// Frames between the last player locking in and the forward exit.
constexpr float kExitDelayFrames = 5.0f;

// mCardPersonaStarts records this for a card whose load failed or held no personas.
constexpr int kNoCardPersonas = -1;

// Player i also matches the card packed as i + 255. For player 1 that is port 2 (0x100), beside
// multitap slot `1-B` (1).
constexpr int kAlternatePortSlotOffset = 255;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Resolve one named object of the renderer as T.
template <class T>
inline T *FindObject(const HxStr &name) {
    return dynamic_cast<T *>(Rnd::TheManager.Find(name));
}

// Append a copy of a persona list to another.
inline void AppendPersonas(std::vector<MetPersonaData *> &list,
                           std::vector<MetPersonaData *> personas) {
    for (unsigned int i = 0; i < personas.size(); ++i) {
        list.push_back(personas[i]);
    }
}

// Show one player's name, or the player number when the persona has no username.
inline void ShowPlayerName(Rnd::Button *pButton, MetPersonaData *pPersona, int nPlayer) {
    if (pPersona->mAppearance.mUserName == kNoName) {
        pButton->mText->SetText(HxStr(Rnd::MakeString(kPlayerLabelFormat, nPlayer + 1)));
    } else {
        pButton->mText->SetText(pPersona->mAppearance.mUserName);
    }
}

} // namespace

// NTSC-U/C: 0x002b1270, PAL: 0x002d0050
MetLocPickCharScreen::MetLocPickCharScreen(MetRenderer *pRenderer, int nPriority)
    : MetMemDetectScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mReadyCount(0), mPlayerCount(0), mExitCountdown(0), mCardIndex(0), mUnusedFirst(0),
      mUnusedSecond(0) {
    mHelpKeys.push_back(MetText(kMetStrHLocPc, kHelpKey));
}

// NTSC-U/C: 0x002b19b0, PAL: 0x002d0808
MetLocPickCharScreen::~MetLocPickCharScreen() {
}

// NTSC-U/C: 0x002b9e60, PAL: 0x002d96c8
MetLocPickCharScreen *MetLocPickCharScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLocPickCharScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x002b1e00, PAL: 0x002d0c58
void MetLocPickCharScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    HxStr name;
    for (int i = 0; i < kMaxPlayers; ++i) {
        name = Rnd::MakeString(kCharacterMatFormat, i + 1);
        Rnd::Mat *pMat = FindObject<Rnd::Mat>(name);
        mCharacterMats.push_back(pMat);

        name = Rnd::MakeString(kSelectViewFormat, i + 1);
        Rnd::View *pView = FindObject<Rnd::View>(name);
        pView->SetShowing(0);
        mSelectViews.push_back(pView);

        name = Rnd::MakeString(kNameButtonFormat, i + 1);
        Rnd::Button *pButton = FindObject<Rnd::Button>(name);
        mNameButtons.push_back(pButton);

        mChoices.push_back(0);

        Rnd::Tex *pTex = FreqAppearance::FindPersonaBurnTexture(i);
        mBurnTextures.push_back(pTex);

        name = Rnd::MakeString(kControllerMeshFormat, i + 1);
        Rnd::Mesh *pMesh = FindObject<Rnd::Mesh>(name);
        mControllerMeshes.push_back(pMesh);
    }
    mDefaultIconMat = FindObject<Rnd::Mat>(HxStr(kDefaultIconMat));
    mCustomIconMat = FindObject<Rnd::Mat>(HxStr(kCustomIconMat));
}

// NTSC-U/C: 0x002b2370, PAL: 0x002d1230
void MetLocPickCharScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (pCommand->mPadIndex > mPlayerCount) {
        return;
    }

    const int nPlayer = pCommand->mPadIndex - 1;
    switch (pCommand->mCommand) {
    case kMetScreenCommandLeft:
    case kMetScreenCommandRight:
        if (!mSelectViews[nPlayer]->GetShowing()) {
            CyclePersona(pCommand);
        }
        break;

    case kMetScreenCommandSelect:
        if (mSelectViews[nPlayer]->GetShowing()) {
            break;
        }
        mSelectViews[nPlayer]->SetShowing(kLockedShowing);
        mNameButtons[nPlayer]->SetState(kButtonStatePressed);
        if (++mReadyCount == mPlayerCount) {
            for (int i = 0; i < mPlayerCount; ++i) {
                mChosenPersonas[i] = mPersonas[mChoices[i]];
            }
            ActivateNamedPanel(HxStr(kNoName));
            MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
            mExitCountdown = kExitDelayFrames;
            MetPersonaData::CopyList(&MetFrontEndState::shared()->mPersonas, &mChosenPersonas);
            Application::shared()->GetGameManager()->ClearPersonas();
            for (unsigned int i = 0; i < MetFrontEndState::shared()->mPersonas.size(); ++i) {
                MetPersonaData *pPersona = MetFrontEndState::shared()->mPersonas[i];
                pPersona->mAppearance.mUserName = mNameButtons[i]->mText->mPreWrapText;
                Application::shared()->GetGameManager()->AddPersona(*pPersona);
            }
        }
        break;

    case kMetScreenCommandBack:
        if (mReadyCount != 0 && mSelectViews[nPlayer]->GetShowing()) {
            if (mSelectViews[nPlayer]->GetShowing() == kLockedShowing) {
                mSelectViews[nPlayer]->SetShowing(0);
                mNameButtons[nPlayer]->SetState(kButtonStateNormal);
                --mReadyCount;
            }
        } else {
            MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
            mExitChoice = kExitBack;
            ExitScreenByName(HxStr(kTitleScreen));
            BeginExit();
        }
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x002b27f8, PAL: 0x002d1738
void MetLocPickCharScreen::CyclePersona(const MetScreenCommand *pCommand) {
    int nChoice = mChoices[pCommand->mPadIndex - 1];
    if (pCommand->mCommand == kMetScreenCommandLeft) {
        nChoice = nChoice - 1 > -1 ? nChoice - 1 : static_cast<int>(mPersonas.size()) - 1;
    } else {
        nChoice = nChoice + 1 < static_cast<int>(mPersonas.size()) ? nChoice + 1 : 0;
    }
    mChoices[pCommand->mPadIndex - 1] = nChoice;

    MetPersonaData *pPersona = mPersonas[nChoice];
    pPersona->AttachToBurnSlot(pCommand->mPadIndex - 1);
    mCharacterMats[pCommand->mPadIndex - 1]->mStages[kBurnStage].SetTex(
        mBurnTextures[pCommand->mPadIndex - 1]);
    ShowPlayerName(mNameButtons[pCommand->mPadIndex - 1], pPersona, pCommand->mPadIndex - 1);
}

// NTSC-U/C: 0x002b29e8, PAL: 0x002d1948
void MetLocPickCharScreen::EnterAndShow() {
    if (!(MetFrontEndState::shared()->mReturnScreen == kLocNumPlayScreen)) {
        ShowPickers();
        return;
    }

    mPersonas.clear();
    SetShowing(0);
    mCardPersonaStarts.clear();
    mShowOnDismiss = 0;
    mLoadingCards = 0;
    if (MetFrontEndState::shared()->mUsingMemcard) {
        StartDetect();
        return;
    }
    AppendPersonas(mPersonas, *MetPersonaData::savedList());
    ShowPickers();
}

// NTSC-U/C: 0x002b2e08, PAL: 0x002d1d68
void MetLocPickCharScreen::SelectPersona(MetPersonaData *pPersona, int nPlayer) {
    unsigned int i;
    for (i = 0; i < mPersonas.size(); ++i) {
        if (pPersona->mAppearance.mUserName == mPersonas[i]->mAppearance.mUserName) {
            break;
        }
    }
    if (i < mPersonas.size()) {
        mChoices[nPlayer] = i;
    } else {
        mChoices[nPlayer] = RandomInt(0, mPersonas.size());
    }
}

// NTSC-U/C: 0x002b2ef8, PAL: 0x002d1e58
void MetLocPickCharScreen::ShowPickers() {
    HxStr title;
    // Yes, the binary never reads the title.
    title = MetConfigText(kMetStrTMulti, kTitleConfigCode, kMultiTitleKey);

    const int nPlayers = MetFrontEndState::shared()->mPlayerCount;
    mChosenPersonas.clear();
    mChosenPersonas.resize(nPlayers);

    if (nPlayers != mPlayerCount) {
        mView->RemoveAllAnims();
        mView->RemoveAllDraws();
        mView->RemoveAllTranses();
        mPlayerCount = nPlayers;
        Rnd::View *pLayout =
            FindObject<Rnd::View>(HxStr(Rnd::MakeString(kLayoutViewFormat, nPlayers)));
        mView->AddAnim(pLayout);
        mView->AddTrans(pLayout);
        std::list<Rnd::Drawable *> &draws = mView->GetDraws();
        mView->AddDraw(pLayout, draws.empty() ? nullptr : draws.front());
        mEnterAnim = FindObject<Rnd::View>(HxStr(Rnd::MakeString(kEnterAnimFormat, nPlayers)));
        mAnimEndFrame = mEnterAnim->FilteredFrameEnd();
    }

    AppendPersonas(mPersonas, *MetFreqMakerAssetManager::shared()->GetIdentityList());

    if (MetFrontEndState::shared()->mReturnScreen == kModeScreen) {
        MetFrontEndState::shared()->mReturnScreen = HxStr(kNoName);
        std::vector<MetPersonaData *> personas(MetFrontEndState::shared()->mPersonas);
        for (int i = 0; i < mPlayerCount; ++i) {
            SelectPersona(personas[i], i);
        }
    } else {
        int nFirst = 1;
        MetPersonaData *pFirst = MetFrontEndState::shared()->GetFirstPersona();
        if (pFirst != nullptr) {
            SelectPersona(pFirst, 0);
        } else {
            nFirst = 0;
        }

        unsigned int nCard = nFirst;
        for (int i = nFirst; i < mPlayerCount; ++i) {
            if (nCard < mFormattedCards.size()) {
                const int nPortSlot =
                    GlobalSettings::shared()->mCardSlots[mFormattedCards[nCard]].mPortSlot;
                if (nPortSlot == i || nPortSlot == i + kAlternatePortSlotOffset) {
                    if (mCardPersonaStarts[nCard] != kNoCardPersonas) {
                        mChoices[i] = mCardPersonaStarts[nCard];
                    } else {
                        mChoices[i] = RandomInt(0, mPersonas.size());
                    }
                    ++nCard;
                    continue;
                }
            }
            mChoices[i] = RandomInt(0, mPersonas.size());
        }
    }

    for (int i = 0; i < mPlayerCount; ++i) {
        mSelectViews[i]->SetShowing(0);
        MetPersonaData *pPersona = mPersonas[mChoices[i]];
        pPersona->AttachToBurnSlot(i);
        mCharacterMats[i]->mStages[kBurnStage].SetTex(mBurnTextures[i]);
        ShowPlayerName(mNameButtons[i], pPersona, i);
        mNameButtons[i]->SetState(kButtonStateNormal);
        const ControllerConfig defaults;
        const bool bDefault =
            GlobalSettings::shared()->mControllers[i].mButtons == defaults.mButtons;
        mControllerMeshes[i]->SetMat(bDefault ? mDefaultIconMat : mCustomIconMat);
    }

    mReadyCount = 0;
    MetScreenTitleScreen::SetTitle(MetConfigText(kMetStrTMpChar, kTitleConfigCode, kPickTitleKey));
    PushNamedScreen(HxStr(kHelpScreen));
    MetScreen::EnterAndShow();
    MetHelpScreen::SetText(mHelpKeys[0], mRenderer->mAnimationFrame);
    ActivateNamedPanel(HxStr(kOwnScreenName));
}

// NTSC-U/C: 0x002b3e18, PAL: 0x002d2f90
void MetLocPickCharScreen::OnExitFinished() {
    PushNamedScreen(HxStr(kLeftGizmoScreen));
    if (mExitChoice == kExitBack) {
        PushNamedScreen(HxStr(kLocNumPlayersScreen));
        ActivateNamedPanel(HxStr(kLocNumPlayersScreen));
    } else {
        PushNamedScreen(HxStr(kModeScreen));
        ActivateNamedPanel(HxStr(kModeScreen));
    }
}

// NTSC-U/C: 0x002b4068, PAL: 0x002d3268
void MetLocPickCharScreen::StartDetect() {
    const HxStr format(MetConfigText(kMetStrMemDetectMulti, kDialogueConfigCode, kDetectMultiKey));
    const HxStr text(
        Rnd::MakeString(TextOrEmpty(format), MetFrontEndState::shared()->mPlayerCount));
    std::vector<HxStr> buttons;
    MetMsgScreen::Show(HxStr(kDetectMessage),
                       MetText(kMetStrMsgWARNING, kWarningTitle),
                       text,
                       kNoButtons,
                       buttons,
                       this);
    MetMemDetectScreen::StartDetect();
}

// NTSC-U/C: 0x002b4378, PAL: 0x002d35f0
void MetLocPickCharScreen::OnNoCard() {
    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
    buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
    MetMsgScreen::ShowActive(HxStr(kNoCardMessage),
                             MetText(kMetStrMsgWARNING, kWarningTitle),
                             MetConfigText(kMetStrMemCheck, kDialogueConfigCode, kNoCardMessage),
                             kTwoButtons,
                             buttons,
                             this);
}

// NTSC-U/C: 0x002b4738, PAL: 0x002d3a40
void MetLocPickCharScreen::OnDetectFinished() {
    if (!MetFrontEndState::shared()->mUsingMemcard) {
        AppendPersonas(mPersonas, *MetPersonaData::savedList());
        ShowPickers();
        return;
    }

    mLoadingCards = 1;
    mFormattedCards.clear();
    for (int i = 0; static_cast<unsigned int>(i) < GlobalSettings::shared()->mCardSlots.size();
         ++i) {
        if (GlobalSettings::shared()->mCardSlots[i].mFormatted) {
            mFormattedCards.push_back(i);
        }
    }

    if (mFormattedCards.size() == 0) {
        mShowOnDismiss = 1;
        ExitScreenByName(HxStr(kMsgScreen));
        return;
    }

    mCardIndex = 0;
    std::vector<HxStr> buttons;
    const HxStr format(MetConfigText(kMetStrMemLoad, kDialogueConfigCode, kLoadKey));
    const HxStr cardName(
        GlobalSettings::shared()->mCardSlots[mFormattedCards[mCardIndex]].mSlotName);
    const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(cardName)));
    MetMsgScreen::ShowActive(HxStr(kLoadMessage),
                             MetText(kMetStrMsgLOADING, kLoadingTitle),
                             text,
                             kNoButtons,
                             buttons,
                             this);
    const int nPortSlot =
        GlobalSettings::shared()->mCardSlots[mFormattedCards[mCardIndex]].mPortSlot;
    mCardPersonaStarts.clear();
    mCardPersonaStarts.push_back(mCardIndex);
    MetPersonaData::ClearLoadList();
    mLoadingCards = 1;
    MemcardManager::shared()->CreateLoadPersonasTask(nPortSlot, MetPersonaData::loadList());
}

// NTSC-U/C: 0x002b4ff0, PAL: 0x002d4398
void MetLocPickCharScreen::OnPersonasLoaded(int, int nStatus) {
    if (mCardIndex == 0) {
        mPersonas.clear();
        if (MetPersonaData::loadList()->size() == 0) {
            mCardPersonaStarts[0] = kNoCardPersonas;
        } else {
            AppendPersonas(mPersonas, *MetPersonaData::loadList());
        }
    }

    if (nStatus == 0) {
        mCardPersonaStarts.push_back(mPersonas.size());
    } else {
        mCardPersonaStarts.push_back(kNoCardPersonas);
    }

    if (static_cast<unsigned int>(++mCardIndex) < mFormattedCards.size()) {
        const int nPortSlot =
            GlobalSettings::shared()->mCardSlots[mFormattedCards[mCardIndex]].mPortSlot;
#ifdef VIDEO_STANDARD_PAL
        std::vector<HxStr> buttons;
        const HxStr format(GetMetString(kMetStrMemLoad));
        const HxStr cardName(
            GlobalSettings::shared()->mCardSlots[mFormattedCards[mCardIndex]].mSlotName);
        const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(cardName)));
        MetMsgScreen::ShowActive(
            HxStr(kLoadMessage), GetMetString(kMetStrMsgLOADING), text, kNoButtons, buttons, this);
#endif
        MemcardManager::shared()->CreateLoadPersonasTask(nPortSlot, &mPersonas);
    } else {
        ExitScreenByName(HxStr(kMsgScreen));
    }
}

// NTSC-U/C: 0x002b9fe8, PAL: 0x002d9850
void MetLocPickCharScreen::PlaySlideSound(int nSelector) {
    if (!mSelectViews[nSelector - 1]->GetShowing() && nSelector <= mPlayerCount) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

// NTSC-U/C: 0x002b9f48, PAL: 0x002d97b0
void MetLocPickCharScreen::PlayCycleLeftSound(int nSelector) {
    if (!mSelectViews[nSelector - 1]->GetShowing() && nSelector <= mPlayerCount) {
        MetScreen::PlayCycleLeftSound(nSelector);
    }
}

// NTSC-U/C: 0x002b9f98, PAL: 0x002d9800
void MetLocPickCharScreen::PlayCycleRightSound(int nSelector) {
    if (!mSelectViews[nSelector - 1]->GetShowing() && nSelector <= mPlayerCount) {
        MetScreen::PlayCycleRightSound(nSelector);
    }
}

// NTSC-U/C: 0x002ba038, PAL: 0x002d2e60
void MetLocPickCharScreen::UpdateIdle(float flTime) {
    if (mExitCountdown != 0) {
        mExitCountdown -= 1.0f;
        if (mExitCountdown == 0) {
            mExitChoice = kExitForward;
            ExitScreenByName(HxStr(kTitleScreen));
            BeginExit();
        }
    }
    MetMemDetectScreen::UpdateIdle(flTime);
}

// NTSC-U/C: 0x002ba148, PAL: 0x002d98a0
void MetLocPickCharScreen::StartLoadPersonas() {
    mPersonaLoadRequested = 1;
    StartSaveSpaceCheck();
}

// NTSC-U/C: 0x002ba178, PAL: 0x002d98d0
void MetLocPickCharScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (mShowOnDismiss || mLoadingCards) {
        mShowOnDismiss = 0;
        mLoadingCards = 0;
        ShowPickers();
    } else {
        MetMemDetectScreen::OnMsgScreenDismissed(name, nChoice);
    }
}
