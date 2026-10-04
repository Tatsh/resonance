#include "met/metfreqmakerdirectionsscreen.h"

#include "met/metfreqmakerassetmanager.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
#include "met/scrollinglist.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/view.h"

namespace {

// Rows and columns of one directions page. The first column is a button glyph and the second the
// text beside it.
constexpr int kPageRowCount = 7;
constexpr int kPageColumnCount = 2;

// The directions pages, in the order the unit's static initialiser builds them at 0x006a3a50.
HxStr g_colorPanelPage[kPageRowCount][kPageColumnCount] = {
    {"c", "switch to color"},
    {"", "panel"},
    {"g", "bring stamp to front"},
    {"h", "push stamp to back"},
    {"p", "flip stamp"},
    {"f", "reset stamp"},
    {"a", "delete stamp"},
};
HxStr g_colorPanelShortPage[kPageRowCount][kPageColumnCount] = {
    {"c", "switch to color"},
    {"", "panel"},
    {"g", "bring stamp to front"},
    {"h", "push stamp to back"},
    {"p", "flip stamp"},
    {"", ""},
    {"", ""},
};
HxStr g_canvasPage[kPageRowCount][kPageColumnCount] = {
    {"c", "switch to freq"},
    {"", "canvas"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_selectStampPage[kPageRowCount][kPageColumnCount] = {
    {"d", "select stamp to"},
    {"", "edit"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_freqFullPage[kPageRowCount][kPageColumnCount] = {
    {"", "your freQ has the"},
    {"", "maximum number"},
    {"", "of stamps"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_editPage[kPageRowCount][kPageColumnCount] = {
    {"d", "edit the color and"},
    {"", "position of your"},
    {"", "freq stamps"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_bodyPage[kPageRowCount][kPageColumnCount] = {
    {"d", "choose from 24"},
    {"", "body stamps"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_headPage[kPageRowCount][kPageColumnCount] = {
    {"d", "choose from 24"},
    {"", "head stamps"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_facePage[kPageRowCount][kPageColumnCount] = {
    {"d", "choose from 24"},
    {"", "face stamps"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_detailsPage[kPageRowCount][kPageColumnCount] = {
    {"d", "choose from 24"},
    {"", "details stamps"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_logosPage[kPageRowCount][kPageColumnCount] = {
    {"d", "choose from 24"},
    {"", "logo stamps"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_mutatePage[kPageRowCount][kPageColumnCount] = {
    {"d", "mutate your freq"},
    {"", "stamps"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_randomizePage[kPageRowCount][kPageColumnCount] = {
    {"d", "randomize your"},
    {"", "entire freq"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_namePage[kPageRowCount][kPageColumnCount] = {
    {"d", "use the keyboard"},
    {"", "to name your freq"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_savePage[kPageRowCount][kPageColumnCount] = {
    {"d", "save to MEMORY"},
    {"", "CARD slot %s"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};
HxStr g_acceptPage[kPageRowCount][kPageColumnCount] = {
    {"d", "Accept changes"},
    {"", "and proceed"},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
};

// The FreQ maker mode names, built after the pages at 0x006a4150. ShowPage() posts the one for the
// page it shows to the help screen. The European release posts a text by identifier instead and
// does not build the names.
#ifdef VIDEO_STANDARD_PAL
const MetStringId g_freqMakerModeTexts[] = {
    kMetStrHFqmakChoosingMode,
    kMetStrHFqmakColoringMode,
    kMetStrHFqmakPositioningMode,
    kMetStrHFqmakEditingMode,
    kMetStrHFqmakRandomize,
    kMetStrHFqmakMutate,
    kMetStrHFqmakBody,
    kMetStrHFqmakHead,
    kMetStrHFqmakFace,
    kMetStrHFqmakDetails,
    kMetStrHFqmakLogos,
    kMetStrHFqmakEdit,
    kMetStrHFqmakName,
    kMetStrHFqmakSave,
    kMetStrHFqmakDone,
    kMetStrHFqmakFreqfull,
};
#else
HxStr g_freqMakerModeNames[] = {
    "fqmak_choosing_mode",
    "fqmak_coloring_mode",
    "fqmak_positioning_mode",
    "fqmak_editing_mode",
    "fqmak_randomize",
    "fqmak_mutate",
    "fqmak_body",
    "fqmak_head",
    "fqmak_face",
    "fqmak_details",
    "fqmak_logos",
    "fqmak_edit",
    "fqmak_name",
    "fqmak_save",
    "fqmak_done",
    "fqmak_freqfull",
};
#endif

// The values of mPage.
enum {
    kSelectStampPage = 0,
    kCanvasPage = 1,
    kColorPanelShortPage = 2,
    kColorPanelPage = 3,
    kRandomizePage = 4,
    kMutatePage = 5,
    kBodyPage = 6,
    kHeadPage = 7,
    kFacePage = 8,
    kDetailsPage = 9,
    kLogosPage = 10,
    kEditPage = 11,
    kNamePage = 12,
    kSavePage = 13,
    kAcceptPage = 14,
    kFreqFullPage = 15,
    kBlankPage = 16,
};

// The row of the save page that receives the card slot name.
constexpr int kSaveSlotRow = 1;

// The screen name, the directory, and the container the constructor supplies.
static const char *const kScreenName = "fm_directions";
static const char *const kDirectory = "metagame/persona";
static const char *const kContainerName = "freq_maker_directions";

// The registry key slot 30 activates.
static const char *const kPanelName = "MetFreqMakerDirectionsScreen";

// The row view the list clones, the pitch between rows, and the context the list passes on.
static const char *const kRowTemplate = "fmd_help_line_prototype.view";
constexpr int kRowPitch = 22;
constexpr int kListContext = 0;

// The help presets ShowPage() selects.
static const char *const kSavePreset = "freq_maker_save_button";
static const char *const kBackOnlyPreset = "only_back_title";
static const char *const kStandardPreset = "standard_title";

// The text the blank page shows.
static const char *const kNoText = "";

#ifdef VIDEO_STANDARD_PAL
// The text object that receives the title, by name.
static const char *const kTitleText = "HELP.txt";

// The text column of each page and the identifier of its first row's text.
constexpr int kTextColumn = 1;
struct PageTexts {
    HxStr (*pTable)[kPageColumnCount];
    int nFirstId;
    int nEndId;
};

// The pages in the order LoadPageTexts() fills them, each from consecutive identifiers.
const PageTexts kPageTexts[] = {
    {g_colorPanelPage, kMetStrFmEditingMode1, kMetStrFmPositioningMode1},
    {g_colorPanelShortPage, kMetStrFmPositioningMode1, kMetStrFmColoringMode1},
    {g_canvasPage, kMetStrFmColoringMode1, kMetStrFmChoosingMode1},
    {g_selectStampPage, kMetStrFmChoosingMode1, kMetStrFmFreqFullMode1},
    {g_freqFullPage, kMetStrFmFreqFullMode1, kMetStrFmEditButton1},
    {g_editPage, kMetStrFmEditButton1, kMetStrFmBodyButton1},
    {g_bodyPage, kMetStrFmBodyButton1, kMetStrFmHeadButton1},
    {g_headPage, kMetStrFmHeadButton1, kMetStrFmFaceButton1},
    {g_facePage, kMetStrFmFaceButton1, kMetStrFmDetailsButton1},
    {g_detailsPage, kMetStrFmDetailsButton1, kMetStrFmLogosButton1},
    {g_logosPage, kMetStrFmLogosButton1, kMetStrFmMutateButton1},
    {g_mutatePage, kMetStrFmMutateButton1, kMetStrFmRandomizeButton1},
    {g_randomizePage, kMetStrFmRandomizeButton1, kMetStrFmNameButton1},
    {g_namePage, kMetStrFmNameButton1, kMetStrFmSaveButton1},
    {g_savePage, kMetStrFmSaveButton1, kMetStrFmDoneButton1},
    {g_acceptPage, kMetStrFmDoneButton1, kMetStrFmDoneButton2 + 1},
};
#endif

// Reports the text of a string, or the shared empty string when it has no buffer.
inline const char *TextOf(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

} // namespace

MetFreqMakerDirectionsScreen::MetFreqMakerDirectionsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mPage(kBlankPage) {
    mShowsLoadedDrawables = 0;
}

MetFreqMakerDirectionsScreen::~MetFreqMakerDirectionsScreen() {
}

MetFreqMakerDirectionsScreen *MetFreqMakerDirectionsScreen::New(MetRenderer *pRenderer,
                                                                int nPriority) {
    return new MetFreqMakerDirectionsScreen(pRenderer, nPriority);
}

void MetFreqMakerDirectionsScreen::EnterAndShow() {
    MetScreen::EnterAndShow();
    mList->setEntriesShowing(1);
}

int MetFreqMakerDirectionsScreen::PollContainerLoad() {
    if (!MetFreqMakerAssetManager::shared()->PollLoad()) {
        return 0;
    }
    return MetScreen::PollContainerLoad();
}

void MetFreqMakerDirectionsScreen::PlayCycleLeftSound(int) {
}

void MetFreqMakerDirectionsScreen::PlayCycleRightSound(int) {
}

void MetFreqMakerDirectionsScreen::OnRepeatingSoundFinished(Rnd::Button *) {
    ActivateNamedPanel(HxStr(kPanelName));
}

void MetFreqMakerDirectionsScreen::OnExitFinished() {
    mList->setEntriesShowing(0);
}

#ifdef VIDEO_STANDARD_PAL
void MetFreqMakerDirectionsScreen::LoadPageTexts() {
    // The binary expands this loop into one loop per page.
    for (const auto &page : kPageTexts) {
        for (int nId = page.nFirstId; nId < page.nEndId; ++nId) {
            page.pTable[nId - page.nFirstId][kTextColumn] = GetMetString(nId);
        }
    }
}
#endif

void MetFreqMakerDirectionsScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
#ifdef VIDEO_STANDARD_PAL
    LoadPageTexts();
    // Yes, the binary does not test the text for null.
    dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(kTitleText)))
        ->SetText(GetMetString(kMetStrFmDirections));
#endif
    mRowTemplate = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kRowTemplate)));
    mList = new ScrollingList(
        this, kRowPitch, kPageRowCount, mRowTemplate, nullptr, nullptr, nullptr, kListContext);
    mList->setItemCount(kPageRowCount);
}

void MetFreqMakerDirectionsScreen::ShowPage(int nPage) {
    mPage = nPage;
    mList->refresh();
#ifdef VIDEO_STANDARD_PAL
    MetHelpScreen::SetText(GetMetString(g_freqMakerModeTexts[nPage]), mRenderer->mAnimationFrame);
#else
    MetHelpScreen::SetText(g_freqMakerModeNames[nPage], mRenderer->mAnimationFrame);
#endif
    if (nPage == kSavePage) {
        MetHelpScreen::SelectPreset(MetText(kMetStrHFreqMakerSaveButton, kSavePreset));
    } else if (nPage == kFreqFullPage) {
        MetHelpScreen::SelectPreset(MetText(kMetStrHOnlyBackTitle, kBackOnlyPreset));
    } else {
        MetHelpScreen::SelectPreset(MetText(kMetStrHStandardTitle, kStandardPreset));
    }
}

const HxStr &
MetFreqMakerDirectionsScreen::PageCell(int nRow, int nColumn, const HxStr (*pTable)[2]) {
    return pTable[nRow][nColumn];
}

int MetFreqMakerDirectionsScreen::ProvideText(int nItem, int nColumn, Rnd::Text *pText, int) {
    switch (mPage) {
    case kSelectStampPage:
        pText->SetText(PageCell(nItem, nColumn, g_selectStampPage));
        break;
    case kCanvasPage:
        pText->SetText(PageCell(nItem, nColumn, g_canvasPage));
        break;
    case kColorPanelShortPage:
        pText->SetText(PageCell(nItem, nColumn, g_colorPanelShortPage));
        break;
    case kColorPanelPage:
        pText->SetText(PageCell(nItem, nColumn, g_colorPanelPage));
        break;
    case kRandomizePage:
        pText->SetText(PageCell(nItem, nColumn, g_randomizePage));
        break;
    case kMutatePage:
        pText->SetText(PageCell(nItem, nColumn, g_mutatePage));
        break;
    case kBodyPage:
        pText->SetText(PageCell(nItem, nColumn, g_bodyPage));
        break;
    case kHeadPage:
        pText->SetText(PageCell(nItem, nColumn, g_headPage));
        break;
    case kFacePage:
        pText->SetText(PageCell(nItem, nColumn, g_facePage));
        break;
    case kDetailsPage:
        pText->SetText(PageCell(nItem, nColumn, g_detailsPage));
        break;
    case kLogosPage:
        pText->SetText(PageCell(nItem, nColumn, g_logosPage));
        break;
    case kEditPage:
        pText->SetText(PageCell(nItem, nColumn, g_editPage));
        break;
    case kNamePage:
        pText->SetText(PageCell(nItem, nColumn, g_namePage));
        break;
    case kSavePage: {
        const HxStr &cell = PageCell(nItem, nColumn, g_savePage);
        if (nItem == kSaveSlotRow) {
            HxStr slotName = FirstCardSlotName();
            pText->SetText(HxStr(Rnd::MakeString(TextOf(cell), TextOf(slotName))));
        } else {
            pText->SetText(cell);
        }
        break;
    }
    case kAcceptPage:
        pText->SetText(PageCell(nItem, nColumn, g_acceptPage));
        break;
    case kFreqFullPage:
        pText->SetText(PageCell(nItem, nColumn, g_freqFullPage));
        break;
    case kBlankPage:
        pText->SetText(HxStr(kNoText));
        break;
    default:
        break;
    }
    return 1;
}

int MetFreqMakerDirectionsScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 0;
}
