#include "met/metjukeboxcustomremixesscreen.h"

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
static const char *const kScreenName = "jbs";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_saved";

static const char *const kCatalogueRow = "jbs_remix_factory_01.view";
static const char *const kHighlight = "jbs_hilite.mesh";
static const char *const kUpArrow = "jbs_arrow_up.mesh";
static const char *const kDownArrow = "jbs_arrow_down.mesh";
static const char *const kPlayListRow = "jbs_remix_playlist_01.view";

static const char *const kDetailText1 = "jbs_genre.txt";
static const char *const kDetailText2 = "jbs_bpm.txt";
static const char *const kDetailText3 = "jbs_Song Title.txt";
static const char *const kDetailText4 = "jbs_date.txt";
static const char *const kDetailText5 = "jbs_Remix Title pre.txt";
static const char *const kPictureMaterial = "jbs_artist pic.mat";
static const char *const kLogoMaterial = "jbs_artist logo.mat";
constexpr int kAppearanceTextCount = 4;
static const char *const kAppearanceTexts[] = {
    "jbs_player_01.txt",
    "jbs_player_02.txt",
    "jbs_player_03.txt",
    "jbs_player_04.txt",
};
static const char *const kPictureMesh = "jbs_artist pic.mesh";
static const char *const kLogoMesh = "jbs_logo.mesh";
static const char *const kWarningText = "jbs_expansion_warning.txt";
static const char *const kWarningPrompt = "remix_unavail_disc";
static const char *const kPlayListCaption = "jbs_CREATE PLAYLIST.txt";

constexpr int kPromptConfigCode = 0x258;

// The two list contexts ProvideText() distinguishes, and the playlist's visible row count.
constexpr int kCatalogueContext = 0;
constexpr int kPlayListContext = 1;
constexpr int kPlayListRowCount = 5;

inline Rnd::Object *Find(const char *pszName) {
    return Rnd::g_manager.Find(HxStr(pszName));
}

} // namespace

// 0x00224f40
MetJukeboxCustomRemixesScreen::MetJukeboxCustomRemixesScreen(MetRenderer *pRenderer, int nPriority)
    : MetJukeboxBaseScreen(pRenderer,
                           nPriority,
                           HxStr(kScreenName),
                           HxStr(kContainerDirectory),
                           HxStr(kContainerFile)) {
    mCatalogueKey = 0;
}

// 0x002250c0
void MetJukeboxCustomRemixesScreen::ResolveContainerViews() {
    MetJukeboxBaseScreen::ResolveContainerViews();

    Rnd::View *pCatalogueRow = dynamic_cast<Rnd::View *>(Find(kCatalogueRow));
    Rnd::Mesh *pHighlight = dynamic_cast<Rnd::Mesh *>(Find(kHighlight));
    Rnd::Mesh *pUpArrow = dynamic_cast<Rnd::Mesh *>(Find(kUpArrow));
    Rnd::Mesh *pDownArrow = dynamic_cast<Rnd::Mesh *>(Find(kDownArrow));
    mCatalogueList = new ScrollingList(this,
                                       mListRowPitch,
                                       mListRowCount,
                                       pCatalogueRow,
                                       pHighlight,
                                       pUpArrow,
                                       pDownArrow,
                                       kCatalogueContext);

    Rnd::View *pPlayListRow = dynamic_cast<Rnd::View *>(Find(kPlayListRow));
    mPlayListList = new ScrollingList(this,
                                      mListRowPitch,
                                      kPlayListRowCount,
                                      pPlayListRow,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      kPlayListContext);

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
    mWarningText = dynamic_cast<Rnd::Text *>(Find(kWarningText));
    HxStr warning = QueryConfigString(kPromptConfigCode, kWarningPrompt);
    mWarningText->SetText(warning);
    mPlayListCaption = dynamic_cast<Rnd::Text *>(Find(kPlayListCaption));
}

// 0x0022a850
MetJukeboxCustomRemixesScreen::~MetJukeboxCustomRemixesScreen() {
}

// 0x0022a910
int MetJukeboxCustomRemixesScreen::GetItemCount() {
    return mCatalogue->size();
}

// 0x0022a938
MetJukeboxCustomRemixesScreen *MetJukeboxCustomRemixesScreen::New(MetRenderer *pRenderer,
                                                                  int nPriority) {
    return new MetJukeboxCustomRemixesScreen(pRenderer, nPriority);
}
