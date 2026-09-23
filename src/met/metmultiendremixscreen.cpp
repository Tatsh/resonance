#include "met/metmultiendremixscreen.h"

#include <vector>

#include "app/application.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metpersonadata.h"
#include "met/metscreentitlescreen.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

// The screen name, the directory the container loads from, and the container name.
static const char *const kScreenName = "erm";
static const char *const kDirectory = "metagame/Shared";
static const char *const kContainerName = "end_multi_remix";

// The two texture pairs the constructor builds.
static const char *const kSongLogoFirstTexture = "gSongLogo1.tex";
static const char *const kSongLogoSecondTexture = "gSongLogo2.tex";
static const char *const kSongLabelFirstTexture = "gSongLabel1.tex";
static const char *const kSongLabelSecondTexture = "gSongLabel2.tex";

// The objects ResolveContainerViews() resolves.
static const char *const kRemixPanelText = "erm_remixpan_title.txt";
static const char *const kGenreText = "erm_genre.txt";
static const char *const kBpmText = "erm_bpm.txt";
static const char *const kLogoMat = "erm_logo.mat";
static const char *const kPhotoMat = "erm_photo.mat";
static const char *const kNameTextFormat = "erm_name_0%d.txt";
static const char *const kPlayerMatFormat = "erm_player%d.mat";
static const char *const kFreqMeshFormat = "erm_freq_0%d.mesh";

// The label of the remix panel, and the formats the genre and tempo are shown through.
static const char *const kRemixPanelLabel = "remix_panel_label";
static const char *const kGenreFormat = "%s";
static const char *const kBpmFormat = "%s bpm";

// The title key, the help layout, and the screen EnterAndShow() pushes.
static const char *const kTitleKey = "multi_remix_over";
static const char *const kPromptLayout = "remix_save_options";
static const char *const kHelpScreen = "MetHelpScreen";

// The empty literal an unused player slot's name is set to.
static const char *const kNoName = "";

// Configuration codes the labels, the level's genre and tempo, and the title are read under.
constexpr int kPromptConfigCode = 0x258;
constexpr int kGenreConfigCode = 0x321;
constexpr int kBpmConfigCode = 0x322;
constexpr int kTitleConfigCode = 0x269;

// The player slots, and the material stage each slot's burn texture goes on.
constexpr int kPlayerSlotCount = 4;
constexpr int kBurnStage = 1;

// The material stage the texture pairs show on.
constexpr int kPairStage = 0;

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

// 0x002f0918
MetMultiEndRemixScreen::MetMultiEndRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mLogoTextures(HxStr(kSongLogoFirstTexture), HxStr(kSongLogoSecondTexture)),
      mLabelTextures(HxStr(kSongLabelFirstTexture), HxStr(kSongLabelSecondTexture)) {
}

// 0x002f0da8
void MetMultiEndRemixScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    Rnd::Text *pPanelText = FindObject<Rnd::Text>(kRemixPanelText);
    {
        HxStr label;
        QueryConfigString(&label, kPromptConfigCode, kRemixPanelLabel);
        pPanelText->SetText(label); // The binary does not test the lookup for null.
    }

    mGenreText = FindObject<Rnd::Text>(kGenreText);
    mBpmText = FindObject<Rnd::Text>(kBpmText);
    mLogoMat = FindObject<Rnd::Mat>(kLogoMat);
    mPhotoMat = FindObject<Rnd::Mat>(kPhotoMat);

    for (int nSlot = 1; nSlot <= kPlayerSlotCount; ++nSlot) {
        Rnd::Text *pName = FindObject<Rnd::Text>(FormatString(kNameTextFormat, nSlot));
        mNameTexts.push_back(pName);
        pName->SetText(HxStr(kNoName));
        mPlayerMats.push_back(FindObject<Rnd::Mat>(FormatString(kPlayerMatFormat, nSlot)));
        mFreqMeshes.push_back(FindObject<Rnd::Mesh>(FormatString(kFreqMeshFormat, nSlot)));
    }
}

// 0x002f1500
MetMultiEndRemixScreen::~MetMultiEndRemixScreen() {
}

// 0x002f16a0
void MetMultiEndRemixScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    (void)MetFrontEndState::shared()->GetFirstPersona(); // Yes, the binary discards this result.
    mLogoTextures.Load(TexturePairRecord::LogoPath(params.mLevelName));
    mLabelTextures.Load(TexturePairRecord::PicturePath(params.mLevelName));

    HxStr genre;
    QueryConfigString(&genre, kGenreConfigCode, TextOf(params.mLevelName));
    HxStr bpm;
    QueryConfigString(&bpm, kBpmConfigCode, TextOf(params.mLevelName));
    mGenreText->SetText(HxStr(FormatString(kGenreFormat, TextOf(genre))));
    mBpmText->SetText(HxStr(FormatString(kBpmFormat, TextOf(bpm))));

    for (int nSlot = 0; nSlot < kPlayerSlotCount; ++nSlot) {
        if (static_cast<unsigned int>(nSlot) < MetFrontEndState::shared()->mUnknown00.size()) {
            MetPersonaData *pPersona = MetFrontEndState::shared()->mUnknown00[nSlot];
            Rnd::Tex *pBurn = FreqAppearance::FindPersonaBurnTexture(nSlot);
            pPersona->AttachToBurnSlot(nSlot);
            mFreqMeshes[nSlot]->SetShowing(1);
            mPlayerMats[nSlot]->mStages[kBurnStage].SetTex(pBurn);
            mNameTexts[nSlot]->SetText(pPersona->mUnknown140.mUnknown00);
        } else {
            mNameTexts[nSlot]->SetText(HxStr(kNoName));
            mFreqMeshes[nSlot]->SetShowing(0);
        }
    }

    {
        HxStr title;
        QueryConfigString(&title, kTitleConfigCode, kTitleKey);
        MetScreenTitleScreen::SetTitle(title);
    }
    MetHelpScreen::SelectPreset(HxStr(kPromptLayout));
    PushNamedScreen(HxStr(kHelpScreen));
    MetScreen::EnterAndShow();
}

// 0x002f5540
MetMultiEndRemixScreen *MetMultiEndRemixScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiEndRemixScreen(pRenderer, nPriority);
}

// 0x002f55c8
void MetMultiEndRemixScreen::OnUnknownSlot26(float) {
    // Both Advance() results are discarded, as in the binary.
    mLogoTextures.Advance();
    mLogoMat->mStages[kPairStage].SetTex(mLogoTextures.Current());
    mLabelTextures.Advance();
    mPhotoMat->mStages[kPairStage].SetTex(mLabelTextures.Current());
}

// 0x002f5650
void MetMultiEndRemixScreen::OnUnknownSlot36() {
}
