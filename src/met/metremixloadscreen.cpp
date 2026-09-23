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
#include "met/scrollinglist.h"
#include "os/formatstring.h"
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

// MetScreen::mUnknown18 records how the screen was left.
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
    HxStr title;
    QueryConfigString(&title,
                      kTitleConfigCode,
                      params.mUnknown1c == kPlayModeJam ? kFactoryRemixTitleKey :
                                                          kFactoryCustomTitleKey);
    return title;
}

inline HxStr CardTitle(const GameParams &params) {
    HxStr slotName = FirstCardSlotName();
    HxStr format;
    QueryConfigString(&format,
                      kTitleConfigCode,
                      params.mUnknown1c == kPlayModeJam ? kCardRemixTitleKey : kCardCustomTitleKey);
    return HxStr(FormatString(TextOrEmpty(format), TextOrEmpty(slotName)));
}

} // namespace

// 0x00349dc0
MetRemixLoadScreen::MetRemixLoadScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown94(nullptr), mUnknowna0(nullptr), mUnknowna4(nullptr), mUnknowna8(nullptr) {
    mUnknown60 = 0;
    mUnknowna8 = new MetButtonList;
}

// 0x00349fc8
void MetRemixLoadScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mUnknowna8->Clear();
    {
        HxStr label;
        QueryConfigString(&label, kPromptConfigCode, kSavedLabelKey);
        mUnknowna8->Add(HxStr(kSavedButton), label);
    }
    {
        HxStr label;
        QueryConfigString(&label, kPromptConfigCode, kFactoryLabelKey);
        mUnknowna8->Add(HxStr(kFactoryButton), label);
    }
    mUnknowna0 = dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(HxStr(kMatchingFont)));
    mUnknowna4 = dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(HxStr(kOtherFont)));
}

// 0x0034a2a8
void MetRemixLoadScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        if (mUnknown90 == nullptr || mUnknown94->getSelected() <= kFirstRow) {
            return;
        }
        mUnknown94->scrollUp();
        ShowRowOnDataScreen(mUnknown94->getSelected());
        break;
    case kMetScreenCommandNext:
        if (mUnknown90 == nullptr ||
            !(static_cast<unsigned>(mUnknown94->getSelected()) < mUnknown90->size() - 1)) {
            return;
        }
        mUnknown94->scrollDown();
        ShowRowOnDataScreen(mUnknown94->getSelected());
        break;
    case kMetScreenCommandLeft:
        mUnknowna8->OnUnknownSlot2();
        OnButtonRingMoved();
        break;
    case kMetScreenCommandRight:
        mUnknowna8->OnUnknownSlot3();
        OnButtonRingMoved();
        break;
    case kMetScreenCommandSelect: {
        if (mUnknown90 == nullptr || mUnknown90->size() == 0) {
            return;
        }
        const MetRemixRecord &record = (*mUnknown90)[mUnknown94->getSelected()];
        if (record.unknown34_ != GetAlbumJukeboxValue()) {
            PlayErrorSound(pCommand->mPadIndex);
            return;
        }
        MetHelpScreen::SetText(HxStr(kNoText), mUnknown10->mUnknown68);
        mUnknown18 = kExitLoad;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kDataScreen));
        BeginExit();
        break;
    }
    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoText), mUnknown10->mUnknown68);
        mUnknown18 = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kDataScreen));
        BeginExit();
        break;
    default:
        break;
    }
}

// 0x0034a748
void MetRemixLoadScreen::ShowRowOnDataScreen(unsigned nIndex) {
    // Yes, the binary takes the registered screen without a cast check.
    MetRemixDataScreen *pDataScreen =
        static_cast<MetRemixDataScreen *>(MetScreen::FindScreenByName(HxStr(kDataScreen)));
    if (mUnknown90 == nullptr || !(nIndex < mUnknown90->size())) {
        pDataScreen->SetRecordShowing(0);
    } else {
        pDataScreen->ShowRecord(&(*mUnknown90)[nIndex]);
    }
}

// 0x0034a838
void MetRemixLoadScreen::OnButtonRingMoved() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    if (mUnknowna8->mSelected == kSavedButtonIndex) {
        mUnknown90 = Catalogue(kCardRemixKey);
        MetScreenTitleScreen::ReplaceTitle(CardTitle(params));
    } else {
        mUnknown90 = Catalogue(MetRemixManager::kFactorySlot);
        MetScreenTitleScreen::ReplaceTitle(FactoryTitle(params));
    }
    mUnknown94->setItemCount(mUnknown90 != nullptr ? mUnknown90->size() : 0);
    mUnknown94->refresh();
    mUnknown94->setSelected(kFirstRow);
    ShowRowOnDataScreen(kFirstRow);
}

// 0x0034b568
void MetRemixLoadScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    mUnknown38.clear();
    mUnknown38.push_back(
        HxStr(params.mUnknown1c == kPlayModeJam ? kCardRemixTitleKey : kCardCustomTitleKey));
    mUnknowna8->ButtonAt(kSavedButtonIndex)->SetShowing(1);
    mUnknowna8->ButtonAt(kFactoryButtonIndex)->SetShowing(1);

    if (MetFrontEndState::shared()->mUnknown0c != 0) {
        mUnknown90 = Catalogue(kCardRemixKey);
        mUnknowna8->SetSelected(kSavedButtonIndex);
        if (mUnknown90->size() != 0) {
            MetScreenTitleScreen::SetTitle(CardTitle(params));
        } else {
            mUnknowna8->SetSelected(kFactoryButtonIndex);
            mUnknown90 = Catalogue(MetRemixManager::kFactorySlot);
            MetScreenTitleScreen::SetTitle(FactoryTitle(params));
        }
    } else {
        mUnknowna8->SetSelected(kFactoryButtonIndex);
        mUnknown90 = Catalogue(MetRemixManager::kFactorySlot);
        MetScreenTitleScreen::SetTitle(FactoryTitle(params));
    }

    if (mUnknown94 == nullptr) {
        Rnd::View *pLine = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kLineView)));
        Rnd::Mesh *pHighlight =
            dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kHighlightMesh)));
        Rnd::Mesh *pUpArrow = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kUpArrowMesh)));
        Rnd::Mesh *pDownArrow =
            dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kDownArrowMesh)));
        mUnknown94 = new ScrollingList(
            this, kRowPitch, kRowCount, pLine, pHighlight, pUpArrow, pDownArrow, kListContext);
    } else {
        mUnknown94->setEntriesShowing(1);
    }
    mUnknown94->setItemCount(mUnknown90->size());
    mUnknown94->setSelected(kFirstRow);
    mUnknown94->refresh();
    MetScreen::EnterAndShow();
}

// 0x0034cbd0
void MetRemixLoadScreen::OnUnknownSlot36() {
    mUnknowna8->ButtonAt(kSavedButtonIndex)->SetShowing(0);
    mUnknowna8->ButtonAt(kFactoryButtonIndex)->SetShowing(0);
    GameParams params(*Application::shared()->GetGameManager()->GetParams());

    if (mUnknown18 == kExitBack) {
        if (params.mUnknown1c == kPlayModeJam) {
            PushNamedScreen(HxStr(kLeftGizmoScreen));
            PushNamedScreen(HxStr(kRemixTypeScreen));
            ActivateNamedPanel(HxStr(kRemixTypeScreen));
        } else {
            PushNamedScreen(HxStr(kSoloStagesScreen));
            ActivateNamedPanel(HxStr(kSoloStagesScreen));
        }
    } else {
        MetRemixRecord record((*mUnknown90)[mUnknown94->getSelected()]);
        params.mLevelName = record.unknown00_;
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
            MetFrontEndState::shared()->mUnknown24 = HxStr(kOwnScreenName);
            nextScreens.push_back(HxStr(kArenasScreen));
            nextScreens.push_back(HxStr(kHelpScreen));
        }

        const int nFactory = mUnknowna8->mSelected == kFactoryButtonIndex;
        std::vector<HxStr> restoreScreens;
        restoreScreens.push_back(HxStr(kOwnScreenName));
        restoreScreens.push_back(HxStr(kTitleScreen));
        restoreScreens.push_back(HxStr(kDataScreen));
        restoreScreens.push_back(HxStr(kHelpScreen));
        MetRemixManager::shared()->BeginRemixLoad(nextScreens, restoreScreens, record, nFactory);
    }

    if (mUnknown94 != nullptr) {
        mUnknown94->setEntriesShowing(0);
    }
}

// 0x0034d8b0
int MetRemixLoadScreen::ProvideText(int nItem, int, Rnd::Text *pText, int) {
    // The catalogue pointer is not tested for null here, unlike in the two sound overrides.
    if (static_cast<unsigned>(nItem) < mUnknown90->size()) {
        MetRemixRecord record((*mUnknown90)[nItem]);
        HxStr name(record.name);
        pText->SetText(name);
        pText->SetFont(record.unknown34_ == GetAlbumJukeboxValue() ? mUnknowna0 : mUnknowna4);
    } else {
        pText->SetText(HxStr(kNoText));
    }
    return 1;
}

// 0x00352588
int MetRemixLoadScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 1;
}

// 0x00352590
MetRemixLoadScreen *MetRemixLoadScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetRemixLoadScreen(pRenderer, nPriority);
}

// 0x00352618
MetRemixLoadScreen::~MetRemixLoadScreen() {
    delete mUnknown94;
    mUnknown94 = nullptr;
    delete mUnknowna8;
}

// 0x003526d0
void MetRemixLoadScreen::PlaySlideSound(int nSelector) {
    if (mUnknown90 != nullptr && mUnknown90->size() != 0) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

// 0x00352720
void MetRemixLoadScreen::PlayHighSound(int nSelector) {
    if (mUnknown90 != nullptr && mUnknown90->size() != 0) {
        MetScreen::PlayHighSound(nSelector);
    }
}

// 0x00352770
void MetRemixLoadScreen::OnUnknownSlot33() {
    ShowRowOnDataScreen(kFirstRow);
    MetHelpScreen::SelectPreset(HxStr(kHelpLayout));
    MetHelpScreen::SetText(mUnknown38[0], mUnknown10->mUnknown68);
}
