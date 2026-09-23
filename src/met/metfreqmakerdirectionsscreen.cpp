#include "met/metfreqmakerdirectionsscreen.h"

#include "met/metsonglists.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/text.h"

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

// The FreQ maker mode names, built after the pages at 0x006a4150. Nothing in the image reads the
// array.
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

// The text the blank page shows.
static const char *const kNoText = "";

// Reports the text of a string, or the shared empty string when it has no buffer.
inline const char *TextOf(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

} // namespace

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
            pText->SetText(HxStr(FormatString(TextOf(cell), TextOf(slotName))));
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
