#include "met/metjukeboxfactoryremixesscreen.h"

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
static const char *const kScreenName = "jbf";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_factory";

// mUnknownc8 for the factory list, where the other two jukebox screens clear the same member.
constexpr int kFactoryListSelector = -1;

static const char *const kCatalogueRow = "jbf_remix_factory_01.view";
static const char *const kHighlight = "jbf_hilite.mesh";
static const char *const kUpArrow = "jbf_arrow_up.mesh";
static const char *const kDownArrow = "jbf_arrow_down.mesh";
static const char *const kPlayListRow = "jbf_remix_playlist_01.view";

static const char *const kDetailText1 = "jbf_genre.txt";
static const char *const kDetailText2 = "jbf_bpm.txt";
static const char *const kDetailText3 = "jbf_Song Title.txt";
static const char *const kDetailText4 = "jbf_date.txt";
static const char *const kDetailText5 = "jbf_Remix Title pre.txt";
static const char *const kPictureMaterial = "jbf_artist pic.mat";
static const char *const kLogoMaterial = "jbf_artist logo.mat";
constexpr int kAppearanceTextCount = 4;
static const char *const kAppearanceTexts[] = {
    "jbf_player_01.txt",
    "jbf_player_02.txt",
    "jbf_player_03.txt",
    "jbf_player_04.txt",
};
static const char *const kPictureMesh = "jbf_artist pic.mesh";
static const char *const kLogoMesh = "jbf_logo.mesh";
static const char *const kWarningText = "jbf_expansion_warning.txt";
static const char *const kWarningPrompt = "remix_unavail_disc";
// Yes, the binary resolves the caption under a `dbf_` prefix.
static const char *const kPlayListCaption = "dbf_CREATE PLAYLIST.txt";

constexpr int kPromptConfigCode = 0x258;

// The two list contexts ProvideText() distinguishes, and the playlist's visible row count.
constexpr int kCatalogueContext = 0;
constexpr int kPlayListContext = 1;
constexpr int kPlayListRowCount = 5;

inline Rnd::Object *Find(const char *pszName) {
    return Rnd::g_manager.Find(HxStr(pszName));
}

} // namespace

// 0x0023ae58
MetJukeboxFactoryRemixesScreen::MetJukeboxFactoryRemixesScreen(MetRenderer *pRenderer,
                                                               int nPriority)
    : MetJukeboxBaseScreen(pRenderer,
                           nPriority,
                           HxStr(kScreenName),
                           HxStr(kContainerDirectory),
                           HxStr(kContainerFile)) {
    mUnknownc8 = kFactoryListSelector;
}

// 0x0023afd8
void MetJukeboxFactoryRemixesScreen::ResolveContainerViews() {
    MetJukeboxBaseScreen::ResolveContainerViews();

    Rnd::View *pCatalogueRow = dynamic_cast<Rnd::View *>(Find(kCatalogueRow));
    pCatalogueRow->SetShowing(1);
    Rnd::Mesh *pHighlight = dynamic_cast<Rnd::Mesh *>(Find(kHighlight));
    Rnd::Mesh *pUpArrow = dynamic_cast<Rnd::Mesh *>(Find(kUpArrow));
    Rnd::Mesh *pDownArrow = dynamic_cast<Rnd::Mesh *>(Find(kDownArrow));
    mUnknown98 = new ScrollingList(this,
                                   mUnknown90,
                                   mUnknown94,
                                   pCatalogueRow,
                                   pHighlight,
                                   pUpArrow,
                                   pDownArrow,
                                   kCatalogueContext);

    Rnd::View *pPlayListRow = dynamic_cast<Rnd::View *>(Find(kPlayListRow));
    mUnknown9c = new ScrollingList(this,
                                   mUnknown90,
                                   kPlayListRowCount,
                                   pPlayListRow,
                                   nullptr,
                                   nullptr,
                                   nullptr,
                                   kPlayListContext);

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
    mUnknown148 = dynamic_cast<Rnd::Text *>(Find(kWarningText));
    HxStr warning;
    QueryConfigString(&warning, kPromptConfigCode, kWarningPrompt);
    mUnknown148->SetText(warning);
    mUnknownb8 = dynamic_cast<Rnd::Text *>(Find(kPlayListCaption));
}

// 0x00240780
MetJukeboxFactoryRemixesScreen::~MetJukeboxFactoryRemixesScreen() {
}

// 0x00240840
int MetJukeboxFactoryRemixesScreen::GetItemCount() {
    return mUnknowna0->size();
}

// 0x00240868
MetJukeboxFactoryRemixesScreen *MetJukeboxFactoryRemixesScreen::New(MetRenderer *pRenderer,
                                                                    int nPriority) {
    return new MetJukeboxFactoryRemixesScreen(pRenderer, nPriority);
}
