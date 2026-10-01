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

// The configuration codes the detail texts are read under, keyed by the record's level name.
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
    mCatalogueKey = 0;
}

// 0x0022ae78
void MetJukeboxEditPlaylistScreen::ResolveContainerViews() {
    MetJukeboxBaseScreen::ResolveContainerViews();

    Rnd::View *pPlayListRow = dynamic_cast<Rnd::View *>(Find(kPlayListRow));
    Rnd::Mesh *pHighlight = dynamic_cast<Rnd::Mesh *>(Find(kHighlight));
    Rnd::Mesh *pUpArrow = dynamic_cast<Rnd::Mesh *>(Find(kUpArrow));
    Rnd::Mesh *pDownArrow = dynamic_cast<Rnd::Mesh *>(Find(kDownArrow));
    mPlayListList = new ScrollingList(this,
                                      mListRowPitch,
                                      mListRowCount,
                                      pPlayListRow,
                                      pHighlight,
                                      pUpArrow,
                                      pDownArrow,
                                      kPlayListContext);
    mCatalogueList = nullptr;

    mGenreText = dynamic_cast<Rnd::Text *>(Find(kDetailText1));
    mTempoText = dynamic_cast<Rnd::Text *>(Find(kDetailText2));
    mSongTitleText = dynamic_cast<Rnd::Text *>(Find(kDetailText3));
    mDateText = dynamic_cast<Rnd::Text *>(Find(kDetailText4));
    mRemixTitleText = dynamic_cast<Rnd::Text *>(Find(kDetailText5));
    mPictureMaterial = dynamic_cast<Rnd::Mat *>(Find(kPictureMaterial));
    mLogoMaterial = dynamic_cast<Rnd::Mat *>(Find(kLogoMaterial));

    mAppearanceTexts.resize(kAppearanceTextCount);
    // The binary expands this loop into one call per text.
    for (int i = 0; i < kAppearanceTextCount; ++i) {
        mAppearanceTexts[i] = dynamic_cast<Rnd::Text *>(Find(kAppearanceTexts[i]));
    }

    mPictureMesh = dynamic_cast<Rnd::Mesh *>(Find(kPictureMesh));
    mLogoMesh = dynamic_cast<Rnd::Mesh *>(Find(kLogoMesh));
}

// 0x0022ba08
int MetJukeboxEditPlaylistScreen::ProvideText(int nItem,
                                              [[maybe_unused]] int nColumn,
                                              Rnd::Text *pText,
                                              int nContext) {
    if (nContext == kPlayListContext) {
        // Yes, the binary copies the whole entry vector to read one entry.
        std::vector<JukeboxPlayListEntry *> entries(mPlayList->entries);
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
        mPlayListList->scrollUp();
        break;

    case kMetScreenCommandNext:
        mPlayListList->scrollDown();
        break;

    case kMetScreenCommandSelect: {
        std::vector<JukeboxPlayListEntry *> &entries = mPlayList->entries;
        if (entries.size() == 0) {
            return;
        }
        const unsigned nSelected = mPlayListList->getSelected();
        mPlayList->RemoveEntry(nSelected);
        const unsigned nCount = entries.size();
        mPlayListList->setItemCount(mPlayList->entries.size());
        mPlayListList->setSelected(nSelected < nCount ? nSelected : nCount - 1);
        mPlayListList->refresh();
        break;
    }

    case kCommandClearPlayList:
        // Yes, the binary empties the vector without releasing the entries.
        mPlayList->entries.erase(mPlayList->entries.begin(), mPlayList->entries.end());
        mPlayListList->setItemCount(mPlayList->entries.size());
        mPlayListList->setSelected(kFirstRow);
        mPlayListList->refresh();
        break;

    case kCommandMoveUp: {
        if (mPlayList->entries.size() == 0) {
            return;
        }
        const int nSelected = mPlayListList->getSelected();
        if (nSelected == kFirstRow) {
            return;
        }
        mPlayList->SwapEntries(nSelected, nSelected - 1);
        mPlayListList->scrollUp();
        mPlayListList->refresh();
        break;
    }

    case kCommandMoveDown: {
        const int nSelected = mPlayListList->getSelected();
        const int nCount = mPlayList->entries.size();
        if (nCount == 0 || nSelected == nCount - 1) {
            return;
        }
        mPlayList->SwapEntries(nSelected, nSelected + 1);
        mPlayListList->scrollDown();
        mPlayListList->refresh();
        break;
    }

    default:
        return;
    }

    ShowRemixDetails();
}

// 0x0022bfd8
void MetJukeboxEditPlaylistScreen::ShowRemixDetails() {
    mGenreText->SetText(HxStr(kNoText));
    mTempoText->SetText(HxStr(kNoText));
    mSongTitleText->SetText(HxStr(kNoText));
    mDateText->SetText(HxStr(kNoText));
    mRemixTitleText->SetText(HxStr(kNoText));
    for (int i = 0; i < kAppearanceTextCount; ++i) {
        mAppearanceTexts[i]->SetText(HxStr(kNoText));
    }

    mPicturesPending = 0;
    if (mPlayList == nullptr || mPlayList->entries.size() == 0) {
        return;
    }
    const int nSelected = mPlayListList->getSelected();
    // Yes, the binary tests the playlist for emptiness a second time.
    if (mPlayList->entries.empty()) {
        return;
    }
    JukeboxPlayListEntry *pEntry = mPlayList->entries[nSelected];
    MetRemixRecord *pRecord = MetRemixManager::shared()->LookupRemix(pEntry->name);

    mLogoTextures.Load(TexturePairRecord::LogoPath(pRecord->levelName));
    mPictureTextures.Load(TexturePairRecord::PicturePath(pRecord->levelName));
    HxStr first = QueryConfigString(kDetailConfigCode1, TextOrEmpty(pRecord->levelName));
    HxStr second = QueryConfigString(kDetailConfigCode2, TextOrEmpty(pRecord->levelName));
    mGenreText->SetText(HxStr(TextOrEmpty(first)));
    second += HxStr(kTempoSuffix);
    mTempoText->SetText(HxStr(TextOrEmpty(second)));

    mRemixTitleText->SetText(pRecord->name);
    const int nAppearances = pRecord->appearances.size();
    for (int i = 0; i < nAppearances; ++i) {
        mAppearanceTexts[i]->SetShowing(1);
        mAppearanceTexts[i]->SetText(pRecord->appearances[i].mUserName);
    }

    HxStr third = QueryConfigString(kDetailConfigCode3, TextOrEmpty(pRecord->levelName));
    const float flWrapWidth = mSongTitleText->mWrapWidth;
    if (flWrapWidth < mSongTitleText->MeasureText(TextOrEmpty(third), third.mLen)) {
        HxStr shorter = QueryConfigString(kDetailConfigCode3Short, TextOrEmpty(pRecord->levelName));
        third = shorter;
    }
    mSongTitleText->SetText(third);
    mDateText->SetText(pRecord->dateTime);
    mPicturesPending = 1;
}

// 0x0022c7f0
void MetJukeboxEditPlaylistScreen::UpdateHelpText() {
    MetHelpScreen::SelectPreset(HxStr(kHelpLayout));
    MetHelpScreen::SetText(HxStr(kHelpText), mRenderer->mAnimationFrame);
}

// 0x00231228
MetJukeboxEditPlaylistScreen::~MetJukeboxEditPlaylistScreen() {
}

// 0x002312e8
int MetJukeboxEditPlaylistScreen::GetItemCount() {
    return mPlayList->entries.size();
}

// 0x00231300
MetJukeboxEditPlaylistScreen *MetJukeboxEditPlaylistScreen::New(MetRenderer *pRenderer,
                                                                int nPriority) {
    return new MetJukeboxEditPlaylistScreen(pRenderer, nPriority);
}

// 0x00231388
void MetJukeboxEditPlaylistScreen::OnEnterFinished() {
    mCatalogue = nullptr;
    mPlayListList->setItemCount(mPlayList->entries.size());
    mPlayListList->refresh();
}

// 0x002313d0
void MetJukeboxEditPlaylistScreen::OnPanelActivated() {
    MetJukeboxBaseScreen::OnPanelActivated();
}
