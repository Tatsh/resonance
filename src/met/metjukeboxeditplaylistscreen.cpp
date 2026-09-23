#include "met/metjukeboxeditplaylistscreen.h"

#include <vector>

#include "game/jukeboxplaylist.h"
#include "met/methelpscreen.h"
#include "met/metremixmanager.h"
#include "met/metrenderer.h"
#include "met/scrollinglist.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/text.h"
#include "rnd/view.h"
#include "script/configquery.h"

namespace {

// The screen name, the container directory, and the container.
static const char *const kScreenName = "jbep";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_edit_playlist";

static const char *const kPlayListRow = "jbep_remix_factory_01.view";
static const char *const kHighlight = "jbep_hilite.mesh";
static const char *const kUpArrow = "jbep_arrow_up.mesh";
static const char *const kDownArrow = "jbep_arrow_down.mesh";

static const char *const kDetailText1 = "jbep_genre.txt";
static const char *const kDetailText2 = "jbep_bpm.txt";
static const char *const kDetailText3 = "jbep_Song Title.txt";
static const char *const kDetailText4 = "jbep_date.txt";
static const char *const kDetailText5 = "jbep_Remix Title pre.txt";
static const char *const kPictureMaterial = "jbep_artist pic.mat";
static const char *const kLogoMaterial = "jbep_artist logo.mat";
constexpr int kAppearanceTextCount = 4;
static const char *const kAppearanceTexts[] = {
    "jbep_player_01.txt",
    "jbep_player_02.txt",
    "jbep_player_03.txt",
    "jbep_player_04.txt",
};
static const char *const kPictureMesh = "jbep_artist pic.mesh";
static const char *const kLogoMesh = "jbep_logo.mesh";

static const char *const kHelpLayout = "met_jukebox_edit_screen_help_tab";
static const char *const kHelpText = "met_jukebox_edit_screen_ticker_tape";

// The text a row past the end of the playlist shows.
static const char *const kNoText = "";
static const char *const kTempoSuffix = " bpm";

// The configuration codes the detail texts are read under, keyed by the record's first string.
constexpr int kDetailConfigCode1 = 0x321;
constexpr int kDetailConfigCode2 = 0x322;
constexpr int kDetailConfigCode3 = 0x325;
constexpr int kDetailConfigCode3Short = 0x327;

// The one list context this screen gives its list.
constexpr int kPlayListContext = 0;

// Command codes above MetScreenCommandCode's range that this screen gives a meaning.
constexpr int kCommandClearPlayList = 8;
constexpr int kCommandMoveUp = 11;
constexpr int kCommandMoveDown = 12;

constexpr int kFirstRow = 0;

inline Rnd::Object *Find(const char *pszName) {
    return Rnd::g_manager.Find(HxStr(pszName));
}

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

} // namespace

// 0x0022acf8
MetJukeboxEditPlaylistScreen::MetJukeboxEditPlaylistScreen(MetRenderer *pRenderer, int nPriority)
    : MetJukeboxBaseScreen(pRenderer,
                           nPriority,
                           HxStr(kScreenName),
                           HxStr(kContainerDirectory),
                           HxStr(kContainerFile)) {
    mUnknownc8 = 0;
}

// 0x0022ae78
void MetJukeboxEditPlaylistScreen::ResolveContainerViews() {
    MetJukeboxBaseScreen::ResolveContainerViews();

    Rnd::View *pPlayListRow = dynamic_cast<Rnd::View *>(Find(kPlayListRow));
    Rnd::Mesh *pHighlight = dynamic_cast<Rnd::Mesh *>(Find(kHighlight));
    Rnd::Mesh *pUpArrow = dynamic_cast<Rnd::Mesh *>(Find(kUpArrow));
    Rnd::Mesh *pDownArrow = dynamic_cast<Rnd::Mesh *>(Find(kDownArrow));
    mUnknown9c = new ScrollingList(this,
                                   mUnknown90,
                                   mUnknown94,
                                   pPlayListRow,
                                   pHighlight,
                                   pUpArrow,
                                   pDownArrow,
                                   kPlayListContext);
    mUnknown98 = nullptr;

    mUnknowna4 = dynamic_cast<Rnd::Text *>(Find(kDetailText1));
    mUnknowna8 = dynamic_cast<Rnd::Text *>(Find(kDetailText2));
    mUnknownac = dynamic_cast<Rnd::Text *>(Find(kDetailText3));
    mUnknownb0 = dynamic_cast<Rnd::Text *>(Find(kDetailText4));
    mUnknownb4 = dynamic_cast<Rnd::Text *>(Find(kDetailText5));
    mUnknownbc = dynamic_cast<Rnd::Mat *>(Find(kPictureMaterial));
    mUnknownc0 = dynamic_cast<Rnd::Mat *>(Find(kLogoMaterial));

    mUnknowncc.resize(kAppearanceTextCount);
    // The binary expands this loop into one call per text.
    for (int i = 0; i < kAppearanceTextCount; ++i) {
        mUnknowncc[i] = dynamic_cast<Rnd::Text *>(Find(kAppearanceTexts[i]));
    }

    mUnknown140 = dynamic_cast<Rnd::Mesh *>(Find(kPictureMesh));
    mUnknown144 = dynamic_cast<Rnd::Mesh *>(Find(kLogoMesh));
}

// 0x0022ba08
int MetJukeboxEditPlaylistScreen::ProvideText(int nItem,
                                              [[maybe_unused]] int nColumn,
                                              Rnd::Text *pText,
                                              int nContext) {
    if (nContext == kPlayListContext) {
        // Yes, the binary copies the whole entry vector to read one entry.
        std::vector<JukeboxPlayListEntry *> entries(mUnknownc4->entries);
        if (static_cast<unsigned>(nItem) < entries.size()) {
            pText->SetText(entries[nItem]->name);
        } else {
            pText->SetText(HxStr(kNoText));
        }
    }
    return 1;
}

// 0x0022bdb8
void MetJukeboxEditPlaylistScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mUnknown9c->scrollUp();
        break;

    case kMetScreenCommandNext:
        mUnknown9c->scrollDown();
        break;

    case kMetScreenCommandSelect: {
        std::vector<JukeboxPlayListEntry *> &entries = mUnknownc4->entries;
        if (entries.size() == 0) {
            return;
        }
        const unsigned nSelected = mUnknown9c->getSelected();
        mUnknownc4->RemoveEntry(nSelected);
        const unsigned nCount = entries.size();
        mUnknown9c->setItemCount(mUnknownc4->entries.size());
        mUnknown9c->setSelected(nSelected < nCount ? nSelected : nCount - 1);
        mUnknown9c->refresh();
        break;
    }

    case kCommandClearPlayList:
        // Yes, the binary empties the vector without releasing the entries.
        mUnknownc4->entries.erase(mUnknownc4->entries.begin(), mUnknownc4->entries.end());
        mUnknown9c->setItemCount(mUnknownc4->entries.size());
        mUnknown9c->setSelected(kFirstRow);
        mUnknown9c->refresh();
        break;

    case kCommandMoveUp: {
        if (mUnknownc4->entries.size() == 0) {
            return;
        }
        const int nSelected = mUnknown9c->getSelected();
        if (nSelected == kFirstRow) {
            return;
        }
        mUnknownc4->SwapEntries(nSelected, nSelected - 1);
        mUnknown9c->scrollUp();
        mUnknown9c->refresh();
        break;
    }

    case kCommandMoveDown: {
        const int nSelected = mUnknown9c->getSelected();
        const int nCount = mUnknownc4->entries.size();
        if (nCount == 0 || nSelected == nCount - 1) {
            return;
        }
        mUnknownc4->SwapEntries(nSelected, nSelected + 1);
        mUnknown9c->scrollDown();
        mUnknown9c->refresh();
        break;
    }

    default:
        return;
    }

    ShowRemixDetails();
}

// 0x0022bfd8
void MetJukeboxEditPlaylistScreen::ShowRemixDetails() {
    mUnknowna4->SetText(HxStr(kNoText));
    mUnknowna8->SetText(HxStr(kNoText));
    mUnknownac->SetText(HxStr(kNoText));
    mUnknownb0->SetText(HxStr(kNoText));
    mUnknownb4->SetText(HxStr(kNoText));
    for (int i = 0; i < kAppearanceTextCount; ++i) {
        mUnknowncc[i]->SetText(HxStr(kNoText));
    }

    mUnknown14c = 0;
    if (mUnknownc4 == nullptr || mUnknownc4->entries.size() == 0) {
        return;
    }
    const int nSelected = mUnknown9c->getSelected();
    // Yes, the binary tests the playlist for emptiness a second time.
    if (mUnknownc4->entries.empty()) {
        return;
    }
    JukeboxPlayListEntry *pEntry = mUnknownc4->entries[nSelected];
    MetRemixRecord *pRecord = MetRemixManager::shared()->LookupRemix(pEntry->name);

    mUnknowne0.Load(TexturePairRecord::LogoPath(pRecord->unknown00_));
    mUnknown110.Load(TexturePairRecord::PicturePath(pRecord->unknown00_));
    HxStr first = QueryConfigString(kDetailConfigCode1, TextOrEmpty(pRecord->unknown00_));
    HxStr second = QueryConfigString(kDetailConfigCode2, TextOrEmpty(pRecord->unknown00_));
    mUnknowna4->SetText(HxStr(TextOrEmpty(first)));
    second += HxStr(kTempoSuffix);
    mUnknowna8->SetText(HxStr(TextOrEmpty(second)));

    mUnknownb4->SetText(pRecord->name);
    const int nAppearances = pRecord->appearances.size();
    for (int i = 0; i < nAppearances; ++i) {
        mUnknowncc[i]->SetShowing(1);
        mUnknowncc[i]->SetText(pRecord->appearances[i].mUnknown00);
    }

    HxStr third = QueryConfigString(kDetailConfigCode3, TextOrEmpty(pRecord->unknown00_));
    const float flWrapWidth = mUnknownac->mWrapWidth;
    if (flWrapWidth < mUnknownac->MeasureText(TextOrEmpty(third), third.mLen)) {
        HxStr shorter =
            QueryConfigString(kDetailConfigCode3Short, TextOrEmpty(pRecord->unknown00_));
        third = shorter;
    }
    mUnknownac->SetText(third);
    mUnknownb0->SetText(pRecord->unknown18_);
    mUnknown14c = 1;
}

// 0x0022c7f0
void MetJukeboxEditPlaylistScreen::UpdateHelpText() {
    MetHelpScreen::SelectPreset(HxStr(kHelpLayout));
    MetHelpScreen::SetText(HxStr(kHelpText), mUnknown10->mUnknown68);
}

// 0x00231228
MetJukeboxEditPlaylistScreen::~MetJukeboxEditPlaylistScreen() {
}

// 0x002312e8
int MetJukeboxEditPlaylistScreen::GetItemCount() {
    return mUnknownc4->entries.size();
}

// 0x00231300
MetJukeboxEditPlaylistScreen *MetJukeboxEditPlaylistScreen::New(MetRenderer *pRenderer,
                                                                int nPriority) {
    return new MetJukeboxEditPlaylistScreen(pRenderer, nPriority);
}

// 0x00231388
void MetJukeboxEditPlaylistScreen::OnUnknownSlot33() {
    mUnknowna0 = nullptr;
    mUnknown9c->setItemCount(mUnknownc4->entries.size());
    mUnknown9c->refresh();
}

// 0x002313d0
void MetJukeboxEditPlaylistScreen::OnUnknownSlot7() {
    MetJukeboxBaseScreen::OnUnknownSlot7();
}
