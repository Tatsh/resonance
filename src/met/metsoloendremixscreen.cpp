#include "met/metsoloendremixscreen.h"

#include <vector>

#include "app/application.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/globalsettings.h"
#include "met/metfrontendstate.h"
#include "met/metglobalsettingssaverscreen.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "met/metsaveremixscreen.h"
#include "met/metscreentitlescreen.h"
#include "os/datetime.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/object.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "erss";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Solo";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "end_remix";

// The two textures the first pair flips between, and the two the second flips between.
static const char *const kSongLogoFirstTexture = "gSongLogo1.tex";
static const char *const kSongLogoSecondTexture = "gSongLogo2.tex";
static const char *const kSongLabelFirstTexture = "gSongLabel1.tex";
static const char *const kSongLabelSecondTexture = "gSongLabel2.tex";

// This screen's own registry key, and the key of the panel slot 7 activates.
static const char *const kOwnScreenName = "MetSoloEndRemixScreen";
static const char *const kSaveScreenName = "MetSaveRemixScreen";

// The screens ReturnToTitle() pushes, in order, and the one it then activates.
static const char *const kHelpScreenName = "MetHelpScreen";
static const char *const kTitleScreenName = "MetScreenTitleScreen";
static const char *const kRemixTypeScreenName = "MetRemixTypeScreen";

// The container objects ResolveContainerViews() looks up.
static const char *const kRemixPanelText = "ers_remix_pan.txt";
static const char *const kGenreText = "ers_genre.txt";
static const char *const kBpmText = "ers_bpm.txt";
static const char *const kTitleText = "ers_title.txt";
static const char *const kArtistText = "ers_artist.txt";
static const char *const kDateText = "ers_date.txt";
static const char *const kFreqNameText = "ers_freqname_remix.txt";
static const char *const kFaceMat = "ers_face.mat";
static const char *const kLogoMat = "ers_logo.mat";
static const char *const kPhotoMat = "ers_photo.mat";

// The configuration code and key the remix panel's label is read under.
constexpr int kPromptConfigCode = 0x258;
static const char *const kRemixPanelLabel = "remix_panel_label";

// ReturnToTitle() lets the renderer resolve the arena view rather than skipping it.
constexpr int kResolveArenaView = 0;

// ResolveContainerViews() takes the first persona burn texture.
constexpr int kFirstBurnTexture = 0;

// The value both MetFrontEndState flags must hold for EnterAndShow() to save the settings first.
constexpr int kFrontEndFlagSet = 1;

// The configuration codes ShowResults() reads the song fields under.
constexpr int kArtistConfigCode = 0x320;
constexpr int kGenreConfigCode = 0x321;
constexpr int kBpmConfigCode = 0x322;
constexpr int kTitleConfigCode = 0x325;
constexpr int kShortTitleConfigCode = 0x327;
constexpr int kCaptionConfigCode = 0x269;

static const char *const kBpmSuffix = " bpm";
static const char *const kDefaultDate = "0/00/00, 12:00";
static const char *const kCaptionKey = "solo_remix_over";

// The face material stage the burn texture is shown on.
constexpr int kFaceBurnStage = 1;

// The controller ShowResults() hands the save to, and its request to empty the save name.
constexpr int kSavePad = 1;
constexpr int kClearSaveName = 1;

// Reports the text of a string, or the shared empty string when it has no buffer.
inline const char *TextOf(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Resolves a registry key to an object of type T, or null.
template <typename T>
T *FindObject(const char *pszName) {
    Rnd::Object *pObject = Rnd::g_manager.Find(HxStr(pszName));
    return pObject != nullptr ? dynamic_cast<T *>(pObject) : nullptr;
}

} // namespace

// 0x00399630
MetSoloEndRemixScreen *MetSoloEndRemixScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetSoloEndRemixScreen(pRenderer, nPriority);
}

// 0x00394728
void MetSoloEndRemixScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    Rnd::Text *pPanelText = FindObject<Rnd::Text>(kRemixPanelText);
    {
        HxStr label;
        QueryConfigString(&label, kPromptConfigCode, kRemixPanelLabel);
        pPanelText->SetText(label); // The binary does not test the lookup for null.
    }

    mGenreText = FindObject<Rnd::Text>(kGenreText);
    mBpmText = FindObject<Rnd::Text>(kBpmText);
    mTitleText = FindObject<Rnd::Text>(kTitleText);
    mArtistText = FindObject<Rnd::Text>(kArtistText);
    mDateText = FindObject<Rnd::Text>(kDateText);
    mFreqNameText = FindObject<Rnd::Text>(kFreqNameText);
    mFaceMat = FindObject<Rnd::Mat>(kFaceMat);
    mBurnTex = FreqAppearance::FindPersonaBurnTexture(kFirstBurnTexture);
    mLogoMat = FindObject<Rnd::Mat>(kLogoMat);
    mPhotoMat = FindObject<Rnd::Mat>(kPhotoMat);
}

// 0x00395a20
void MetSoloEndRemixScreen::ReturnToTitle() {
    mUnknown10->ResolveArenaView(kResolveArenaView);
    mUnknown10->OnUnknown00390088();
    mUnknown10->OnUnknown00390090();
    PushNamedScreen(HxStr(kHelpScreenName));
    PushNamedScreen(HxStr(kTitleScreenName));
    PushNamedScreen(HxStr(kRemixTypeScreenName));
    ActivateNamedPanel(HxStr(kRemixTypeScreenName));
}

// 0x00394e10
void MetSoloEndRemixScreen::EnterAndShow() {
    if ((MetFrontEndState::shared()->mUnknown0c == kFrontEndFlagSet) &&
        (MetFrontEndState::shared()->mUnknown10 == kFrontEndFlagSet)) {
        MetFrontEndState::shared()->mUnknown10 = 0;
        std::vector<HxStr> screens(1, HxStr());
        screens[0] = kOwnScreenName;
        MetGlobalSettingsSaverScreen::StartSave(screens);
        mUnknown50 = 0;
    } else {
        ShowResults();
    }
}

// 0x00395058
void MetSoloEndRemixScreen::ShowResults() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    MetPersonaData *pPersona = MetFrontEndState::shared()->GetFirstPersona();
    mUnknownbc.Load(TexturePairRecord::LogoPath(params.mLevelName));
    mUnknownec.Load(TexturePairRecord::PicturePath(params.mLevelName));

    HxStr genre;
    QueryConfigString(&genre, kGenreConfigCode, TextOf(params.mLevelName));
    HxStr bpm;
    QueryConfigString(&bpm, kBpmConfigCode, TextOf(params.mLevelName));
    bpm += HxStr(kBpmSuffix);
    mBpmText->SetText(HxStr(TextOf(bpm)));
    mGenreText->SetText(HxStr(TextOf(genre)));

    HxStr artist;
    QueryConfigString(&artist, kArtistConfigCode, TextOf(params.mLevelName));
    mArtistText->SetText(artist);

    HxStr title;
    QueryConfigString(&title, kTitleConfigCode, TextOf(params.mLevelName));
    const float flWrapWidth = mTitleText->mWrapWidth;
    if (flWrapWidth < mTitleText->MeasureText(TextOf(title), title.mLen)) {
        HxStr shorter;
        QueryConfigString(&shorter, kShortTitleConfigCode, TextOf(params.mLevelName));
        title = shorter;
    }
    mTitleText->SetText(title);

    HxStr date;
    if (!FormatCurrentDateTime(date)) {
        date = kDefaultDate;
    }
    mDateText->SetText(date);

    mFreqNameText->SetText(pPersona->mUnknown140.mUnknown00);
    pPersona->AttachToBurnSlot(kFirstBurnTexture);
    mFaceMat->mStages[kFaceBurnStage].SetTex(mBurnTex);
    mUnknownb8 = 0;

    std::vector<FreqAppearance> appearances;
    appearances.push_back(pPersona->mUnknown140);
    {
        HxStr caption;
        QueryConfigString(&caption, kCaptionConfigCode, kCaptionKey);
        MetScreenTitleScreen::SetTitle(caption);
    }
    PushNamedScreen(HxStr(kHelpScreenName));
    (void)GlobalSettings::shared(); // Yes, the binary discards this call's result.
    MetSaveRemixScreen::Open(pPersona,
                             kSavePad,
                             this,
                             GlobalSettings::shared()->mCardSlots[0],
                             appearances,
                             kClearSaveName);
    MetScreen::EnterAndShow();
}

// 0x00399748
void MetSoloEndRemixScreen::OnUnknownSlot26([[maybe_unused]] float flTime) {
    // Both Advance() results are discarded, as in the binary.
    mUnknownbc.Advance();
    mLogoMat->mStages[0].SetTex(mUnknownbc.Current());
    mUnknownec.Advance();
    mPhotoMat->mStages[0].SetTex(mUnknownec.Current());
}

// 0x003943f8
MetSoloEndRemixScreen::MetSoloEndRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknownbc(HxStr(kSongLogoFirstTexture), HxStr(kSongLogoSecondTexture)),
      mUnknownec(HxStr(kSongLabelFirstTexture), HxStr(kSongLabelSecondTexture)) {
}

// 0x003996b8
MetSoloEndRemixScreen::~MetSoloEndRemixScreen() {
}

// 0x003997d0
void MetSoloEndRemixScreen::OnUnknownSlot7() {
    ActivateNamedPanel(HxStr(kSaveScreenName));
}

// 0x00399870
void MetSoloEndRemixScreen::OnUnknownSlot36() {
    if (mUnknownb8 == 0) {
        ReturnToTitle();
    }
}

// 0x003998c8
void MetSoloEndRemixScreen::OnUnknownSlot2([[maybe_unused]] int nUnknown) {
    if (mUnknownb8 == 0) {
        BeginExit();
    } else {
        ReturnToTitle();
    }
}

// 0x00399898
void MetSoloEndRemixScreen::OnUnknownSlot3() {
    mUnknownb8 = 1;
    BeginExit();
}

// 0x00395918
void MetSoloEndRemixScreen::OnUnknownSlot4(int nFlag) {
    // The binary negates with `xori` against 1, which is what a bool argument compiles to, so only
    // 0 and 1 round-trip through the member.
    mUnknownb8 = nFlag ^ 1;
    if (nFlag != 0) {
        PushNamedScreen(HxStr(kOwnScreenName));
    } else {
        ExitScreenByName(HxStr(kOwnScreenName));
    }
}
