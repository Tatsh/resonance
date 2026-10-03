#include "met/metremixloadscreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/albumcache.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metremixdatascreen.h"
#include "met/metremixmanager.h"
#include "met/metremixrecord.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
#include "met/scrollinglist.h"
#include "os/formatstring.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/font.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/text.h"
#include "rnd/view.h"
#include "script/configquery.h"
#include "script/scripthost.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcrl";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memcard_remix_load";

// The text a row past the end of the catalogue shows.
static const char *const kNoText = "";

static const char *const kSavedButton = "mcrl_SAVED.but";
static const char *const kSavedLabelKey = "rl_saved";
static const char *const kFactoryButton = "mcrl_FACTORY.but";
static const char *const kFactoryLabelKey = "rl_factory";
static const char *const kMatchingFont = "font1_pink_2";
static const char *const kOtherFont = "font1_pinkgrey_2";

static const char *const kCardRemixTitleKey = "mem_load_remix";
static const char *const kCardCustomTitleKey = "mem_load_custom";
static const char *const kFactoryRemixTitleKey = "fact_load_remix";
static const char *const kFactoryCustomTitleKey = "fact_load_custom";
static const char *const kHelpLayout = "remix_load_opt";

static const char *const kLineView = "mcrl_line.view";
static const char *const kHighlightMesh = "mcrl_hilite.mesh";
static const char *const kUpArrowMesh = "mcrl_up.mesh";
static const char *const kDownArrowMesh = "mcrl_down.mesh";

static const char *const kOwnScreenName = "MetRemixLoadScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kDataScreen = "MetRemixDataScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kRemixTypeScreen = "MetRemixTypeScreen";
static const char *const kSoloStagesScreen = "MetSoloStagesScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";
static const char *const kArenasScreen = "MetArenasScreen";

// Script template 0x267 runs with this argument when a remix is chosen.
constexpr int kLoadingGameTemplate = 615;
static const char *const kLoadingGameArgument = "1";

constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// The two buttons, and the MetRemixManager::mRemixes key of the first card slot.
constexpr int kSavedButtonIndex = 0;
constexpr int kFactoryButtonIndex = 1;
constexpr int kCardRemixKey = 0;

// The list geometry.
constexpr int kRowPitch = 24;
constexpr int kRowCount = 13;
constexpr int kListContext = 0;
constexpr int kFirstRow = 0;

// MetScreen::mExitChoice records how the screen departed.
constexpr int kExitBack = 0;
constexpr int kExitLoad = 2;

// With this many personas the last arena is taken rather than chosen.
constexpr int kLastArenaPersonaCount = 3;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

inline std::vector<MetRemixRecord> *Catalogue(int nKey) {
    return &MetRemixManager::shared()->mRemixes[nKey];
}

inline HxStr FactoryTitle(const GameParams &params) {
    HxStr title =
        params.mPlayMode == kPlayModeJam ?
            MetConfigText(kMetStrTFactLoadRemix, kTitleConfigCode, kFactoryRemixTitleKey) :
            MetConfigText(kMetStrTFactLoadCustom, kTitleConfigCode, kFactoryCustomTitleKey);
    return title;
}

// The European release shows the Spanish title without the card name, and the French one too when
// bPlainInFrench is set.
inline HxStr CardTitle(const GameParams &params, [[maybe_unused]] bool bPlainInFrench) {
    HxStr slotName = FirstCardSlotName();
    HxStr format = params.mPlayMode == kPlayModeJam ?
                       MetConfigText(kMetStrTMemLoadRemix, kTitleConfigCode, kCardRemixTitleKey) :
                       MetConfigText(kMetStrTMemLoadCustom, kTitleConfigCode, kCardCustomTitleKey);
    HxStr title(FormatString(TextOrEmpty(format), TextOrEmpty(slotName)));
#ifdef VIDEO_STANDARD_PAL
    if (GetLanguage() == SCE_SPANISH_LANGUAGE ||
        (bPlainInFrench && GetLanguage() == SCE_FRENCH_LANGUAGE)) {
        title = format;
    }
#endif
    return title;
}

// OnButtonRingMoved() shows the French title without the card name, and EnterAndShow() with it.
constexpr bool kRingTitlePlainInFrench = true;
constexpr bool kEnterTitlePlainInFrench = false;

} // namespace

// NTSC-U/C: 0x00349dc0, PAL: 0x00375678
MetRemixLoadScreen::MetRemixLoadScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mList(nullptr), mThisDiscFont(nullptr), mOtherFont(nullptr), mButtons(nullptr) {
    mShowsLoadedDrawables = 0;
    mButtons = new MetButtonList;
}

// NTSC-U/C: 0x00349fc8, PAL: 0x003758e0
void MetRemixLoadScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mButtons->Clear();
    {
        HxStr label = MetConfigText(kMetStrRlSaved, kPromptConfigCode, kSavedLabelKey);
        mButtons->Add(HxStr(kSavedButton), label);
    }
    {
        HxStr label = MetConfigText(kMetStrRlFactory, kPromptConfigCode, kFactoryLabelKey);
        mButtons->Add(HxStr(kFactoryButton), label);
    }
    mThisDiscFont = dynamic_cast<Rnd::Font *>(Rnd::TheManager.Find(HxStr(kMatchingFont)));
    mOtherFont = dynamic_cast<Rnd::Font *>(Rnd::TheManager.Find(HxStr(kOtherFont)));
}

// NTSC-U/C: 0x0034a2a8, PAL: 0x00375c68
void MetRemixLoadScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        if (mRemixes == nullptr || mList->getSelected() <= kFirstRow) {
            return;
        }
        mList->scrollUp();
        ShowRowOnDataScreen(mList->getSelected());
        break;
    case kMetScreenCommandNext:
        if (mRemixes == nullptr ||
            !(static_cast<unsigned>(mList->getSelected()) < mRemixes->size() - 1)) {
            return;
        }
        mList->scrollDown();
        ShowRowOnDataScreen(mList->getSelected());
        break;
    case kMetScreenCommandLeft:
        mButtons->SelectPrevious();
        OnButtonRingMoved();
        break;
    case kMetScreenCommandRight:
        mButtons->SelectNext();
        OnButtonRingMoved();
        break;
    case kMetScreenCommandSelect: {
        if (mRemixes == nullptr || mRemixes->size() == 0) {
            return;
        }
        const MetRemixRecord &record = (*mRemixes)[mList->getSelected()];
        if (record.albumNumber != GetAlbumJukeboxValue()) {
            PlayErrorSound(pCommand->mPadIndex);
            return;
        }
        MetHelpScreen::SetText(HxStr(kNoText), mRenderer->mAnimationFrame);
        mExitChoice = kExitLoad;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kDataScreen));
        BeginExit();
        break;
    }
    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoText), mRenderer->mAnimationFrame);
        mExitChoice = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kDataScreen));
        BeginExit();
        break;
    default:
        break;
    }
}

// NTSC-U/C: 0x0034a748, PAL: 0x003761d0
void MetRemixLoadScreen::ShowRowOnDataScreen(unsigned nIndex) {
    // Yes, the binary takes the registered screen without a cast check.
    MetRemixDataScreen *pDataScreen =
        static_cast<MetRemixDataScreen *>(MetScreen::FindScreenByName(HxStr(kDataScreen)));
    if (mRemixes == nullptr || !(nIndex < mRemixes->size())) {
        pDataScreen->SetRecordShowing(0);
    } else {
        pDataScreen->ShowRecord(&(*mRemixes)[nIndex]);
    }
}

// NTSC-U/C: 0x0034a838, PAL: 0x003762e0
void MetRemixLoadScreen::OnButtonRingMoved() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    if (mButtons->mSelected == kSavedButtonIndex) {
        mRemixes = Catalogue(kCardRemixKey);
        MetScreenTitleScreen::ReplaceTitle(CardTitle(params, kRingTitlePlainInFrench));
    } else {
        mRemixes = Catalogue(MetRemixManager::kFactorySlot);
        MetScreenTitleScreen::ReplaceTitle(FactoryTitle(params));
    }
    mList->setItemCount(mRemixes != nullptr ? mRemixes->size() : 0);
    mList->refresh();
    mList->setSelected(kFirstRow);
    ShowRowOnDataScreen(kFirstRow);
}

// NTSC-U/C: 0x0034b568, PAL: 0x00377138
void MetRemixLoadScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    mHelpKeys.clear();
    mHelpKeys.push_back(params.mPlayMode == kPlayModeJam ?
                            MetText(kMetStrHMemLoadRemix, kCardRemixTitleKey) :
                            MetText(kMetStrHMemLoadCustom, kCardCustomTitleKey));
    mButtons->GetButton(kSavedButtonIndex)->SetShowing(1);
    mButtons->GetButton(kFactoryButtonIndex)->SetShowing(1);

    if (MetFrontEndState::shared()->mUsingMemcard != 0) {
        mRemixes = Catalogue(kCardRemixKey);
        mButtons->SetSelected(kSavedButtonIndex);
        if (mRemixes->size() != 0) {
            MetScreenTitleScreen::SetTitle(CardTitle(params, kEnterTitlePlainInFrench));
        } else {
            mButtons->SetSelected(kFactoryButtonIndex);
            mRemixes = Catalogue(MetRemixManager::kFactorySlot);
            MetScreenTitleScreen::SetTitle(FactoryTitle(params));
        }
    } else {
        mButtons->SetSelected(kFactoryButtonIndex);
        mRemixes = Catalogue(MetRemixManager::kFactorySlot);
        MetScreenTitleScreen::SetTitle(FactoryTitle(params));
    }

    if (mList == nullptr) {
        Rnd::View *pLine = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kLineView)));
        Rnd::Mesh *pHighlight =
            dynamic_cast<Rnd::Mesh *>(Rnd::TheManager.Find(HxStr(kHighlightMesh)));
        Rnd::Mesh *pUpArrow = dynamic_cast<Rnd::Mesh *>(Rnd::TheManager.Find(HxStr(kUpArrowMesh)));
        Rnd::Mesh *pDownArrow =
            dynamic_cast<Rnd::Mesh *>(Rnd::TheManager.Find(HxStr(kDownArrowMesh)));
        mList = new ScrollingList(
            this, kRowPitch, kRowCount, pLine, pHighlight, pUpArrow, pDownArrow, kListContext);
    } else {
        mList->setEntriesShowing(1);
    }
    mList->setItemCount(mRemixes->size());
    mList->setSelected(kFirstRow);
    mList->refresh();
    MetScreen::EnterAndShow();
}

// NTSC-U/C: 0x0034cbd0, PAL: 0x00378920
void MetRemixLoadScreen::OnExitFinished() {
    mButtons->GetButton(kSavedButtonIndex)->SetShowing(0);
    mButtons->GetButton(kFactoryButtonIndex)->SetShowing(0);
    GameParams params(*Application::shared()->GetGameManager()->GetParams());

    if (mExitChoice == kExitBack) {
        if (params.mPlayMode == kPlayModeJam) {
            PushNamedScreen(HxStr(kLeftGizmoScreen));
            PushNamedScreen(HxStr(kRemixTypeScreen));
            ActivateNamedPanel(HxStr(kRemixTypeScreen));
        } else {
            PushNamedScreen(HxStr(kSoloStagesScreen));
            ActivateNamedPanel(HxStr(kSoloStagesScreen));
        }
    } else {
        MetRemixRecord record((*mRemixes)[mList->getSelected()]);
        params.mLevelName = record.levelName;
        params.mLoadingGame = 1;
        CallScriptTemplate(kLoadingGameTemplate, kLoadingGameArgument);
        MetRemixManager::shared()->SetRecord(record);
        Application::shared()->GetGameManager()->SetParams(params);

        std::vector<HxStr> nextScreens;
        if (Application::shared()->GetGameManager()->GetPersonas()->size() >=
            kLastArenaPersonaCount) {
            GameParams arenaParams(*Application::shared()->GetGameManager()->GetParams());
            const int nLastArena = GetArenaList()->size() - 1;
            arenaParams.mArenaName = (*GetArenaList())[nLastArena].mName;
            Application::shared()->GetGameManager()->SetParams(arenaParams);
            nextScreens.push_back(HxStr(kLoadGameScreen));
        } else {
            MetFrontEndState::shared()->mReturnScreen = HxStr(kOwnScreenName);
            nextScreens.push_back(HxStr(kArenasScreen));
            nextScreens.push_back(HxStr(kHelpScreen));
        }

        const int nFactory = mButtons->mSelected == kFactoryButtonIndex;
        std::vector<HxStr> restoreScreens;
        restoreScreens.push_back(HxStr(kOwnScreenName));
        restoreScreens.push_back(HxStr(kTitleScreen));
        restoreScreens.push_back(HxStr(kDataScreen));
        restoreScreens.push_back(HxStr(kHelpScreen));
        MetRemixManager::shared()->BeginRemixLoad(nextScreens, restoreScreens, record, nFactory);
    }

    if (mList != nullptr) {
        mList->setEntriesShowing(0);
    }
}

// NTSC-U/C: 0x0034d8b0, PAL: 0x00379888
int MetRemixLoadScreen::ProvideText(int nItem, int, Rnd::Text *pText, int) {
    // The catalogue pointer is not tested for null here, unlike in the two sound overrides.
    if (static_cast<unsigned>(nItem) < mRemixes->size()) {
        MetRemixRecord record((*mRemixes)[nItem]);
        HxStr name(record.name);
        pText->SetText(name);
        pText->SetFont(record.albumNumber == GetAlbumJukeboxValue() ? mThisDiscFont : mOtherFont);
    } else {
        pText->SetText(HxStr(kNoText));
    }
    return 1;
}

// NTSC-U/C: 0x00352588, PAL: 0x0037e6f8
int MetRemixLoadScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 1;
}

// NTSC-U/C: 0x00352590, PAL: 0x0037e700
MetRemixLoadScreen *MetRemixLoadScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetRemixLoadScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x00352618, PAL: 0x0037e788
MetRemixLoadScreen::~MetRemixLoadScreen() {
    delete mList;
    mList = nullptr;
    delete mButtons;
}

// NTSC-U/C: 0x003526d0, PAL: 0x0037e840
void MetRemixLoadScreen::PlaySlideSound(int nSelector) {
    if (mRemixes != nullptr && mRemixes->size() != 0) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

// NTSC-U/C: 0x00352720, PAL: 0x0037e890
void MetRemixLoadScreen::PlayHighSound(int nSelector) {
    if (mRemixes != nullptr && mRemixes->size() != 0) {
        MetScreen::PlayHighSound(nSelector);
    }
}

// NTSC-U/C: 0x00352770, PAL: 0x0037e8e0
void MetRemixLoadScreen::OnEnterFinished() {
    ShowRowOnDataScreen(kFirstRow);
    MetHelpScreen::SelectPreset(MetText(kMetStrHRemixLoadOpt, kHelpLayout));
    MetHelpScreen::SetText(mHelpKeys[0], mRenderer->mAnimationFrame);
}
