#include "met/metremixdatascreen.h"

#include "game/freqappearance.h"
#include "met/albumcache.h"
#include "met/metremixrecord.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcdat";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memcard_remix_data";

// The two texture pairs the constructor builds.
static const char *const kSongLogoFirst = "gSongLogo1.tex";
static const char *const kSongLogoSecond = "gSongLogo2.tex";
static const char *const kSongLabelFirst = "gSongLabel1.tex";
static const char *const kSongLabelSecond = "gSongLabel2.tex";

static const char *const kSongTitleText = "mcrl_songtitle.txt";
static const char *const kDateText = "mcrl_dob.txt";
static const char *const kPhotoMaterial = "mcrl_photo.mat";
static const char *const kLogoMaterial = "mcrl_logo.mat";
static const char *const kUnavailableText = "mcrl_remixunavail.txt";
static const char *const kLabelMesh = "mcrl_label.mesh";
static const char *const kLogoMesh = "mcrl_logo.mesh";
#ifdef VIDEO_STANDARD_PAL
static const char *const kPanelTitleText = "mcrl_remixpan_title.txt";
#endif

// Counted from 1.
static const char *const kNameTextFormat = "mcrl_name_0%d.txt";
static const char *const kPlayerMaterialFormat = "mcrl_player%d.mat";
static const char *const kPlayerMeshFormat = "mcrl_freq_0%d.mesh";

static const char *const kUnavailableKey = "remix_unavail_disc";
static const char *const kNoText = "";

constexpr int kRowCount = 4;

constexpr int kPromptConfigCode = 0x258;
constexpr int kSongNameConfigCode = 0x325;
constexpr int kShortSongNameConfigCode = 0x327;

// The stage of a material that takes a swapped texture, and the stage of a player's material that
// takes the persona picture.
constexpr int kBaseStage = 0;
constexpr int kPictureStage = 1;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

inline Rnd::Text *FindText(const char *pszName) {
    return dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(pszName)));
}

inline Rnd::Mat *FindMaterial(const char *pszName) {
    return dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr(pszName)));
}

inline Rnd::Mesh *FindMesh(const char *pszName) {
    return dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(pszName)));
}

} // namespace

// 0x00344740
MetRemixDataScreen::MetRemixDataScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mLogoTextures(HxStr(kSongLogoFirst), HxStr(kSongLogoSecond)),
      mLabelTextures(HxStr(kSongLabelFirst), HxStr(kSongLabelSecond)) {
}

// 0x00344bd0
MetRemixDataScreen::~MetRemixDataScreen() {
}

// NTSC-U/C: 0x00344d70, PAL: 0x00370198
void MetRemixDataScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mSongTitleText = FindText(kSongTitleText);
    mDateText = FindText(kDateText);
    mLabelMaterial = FindMaterial(kPhotoMaterial);
    mLogoMaterial = FindMaterial(kLogoMaterial);
    mUnavailableText = FindText(kUnavailableText);
    mUnavailableText->SetShowing(0);
    mLabelMesh = FindMesh(kLabelMesh);
    mLogoMesh = FindMesh(kLogoMesh);
#ifdef VIDEO_STANDARD_PAL
    FindText(kPanelTitleText)->SetText(GetMetString(kMetStrMcrfRemixdata)); // Not tested for null.
#endif

    for (int nRow = 1; nRow <= kRowCount; ++nRow) {
        Rnd::Text *pName = FindText(FormatString(kNameTextFormat, nRow));
        mPlayerNames.push_back(pName);
        pName->SetText(HxStr(kNoText));

        Rnd::Mat *pPicture = FindMaterial(FormatString(kPlayerMaterialFormat, nRow));
        mPlayerPictures.push_back(pPicture);

        Rnd::Mesh *pMesh = FindMesh(FormatString(kPlayerMeshFormat, nRow));
        mPlayerMeshes.push_back(pMesh);
    }
}

// NTSC-U/C: 0x003455a8, PAL: 0x00370c50
void MetRemixDataScreen::ShowRecord(MetRemixRecord *pRecord) {
    if (mViewsUnresolved != 0) {
        return;
    }
    SetRecordShowing(1);

    if (pRecord->albumNumber == GetAlbumJukeboxValue()) {
        HxStr level(pRecord->levelName);
        HxStr songName = QueryConfigString(kSongNameConfigCode, TextOrEmpty(level));
        const float flWrapWidth = mSongTitleText->mWrapWidth;
        if (flWrapWidth < mSongTitleText->MeasureText(TextOrEmpty(songName), songName.mLen)) {
            HxStr shorter = QueryConfigString(kShortSongNameConfigCode, TextOrEmpty(level));
            songName = shorter;
        }
        mSongTitleText->SetText(songName);
        mLogoTextures.Load(TexturePairRecord::LogoPath(level));
        mLabelTextures.Load(TexturePairRecord::PicturePath(level));
        mUnavailableText->SetShowing(0);
    } else {
        mUnavailableText->SetShowing(1);
        HxStr notice = MetConfigText(kMetStrRemixUnavailDisc, kPromptConfigCode, kUnavailableKey);
        mUnavailableText->SetText(notice);
        mSongTitleText->SetText(HxStr(kNoText));
        mLabelMesh->SetShowing(0);
        mLabelTextures.CancelLoad();
        mLabelTextures.invalidate();
        mLogoMesh->SetShowing(0);
        mLogoTextures.CancelLoad();
        mLogoTextures.invalidate();
    }

    mDateText->SetText(pRecord->dateTime);
    const int nCount = pRecord->appearances.size();
    for (int i = 0; i < nCount; ++i) {
        mPlayerMeshes[i]->SetShowing(1);
        Rnd::Tex *pPicture = FreqAppearance::FindPersonaBurnTexture(i);
        pRecord->appearances[i].AttachToBurnSlot(i);
        mPlayerPictures[i]->mStages[kPictureStage].SetTex(pPicture);
        HxStr name(pRecord->appearances[i].mUserName);
        mPlayerNames[i]->SetText(name);
    }
    for (int i = nCount; i < kRowCount; ++i) {
        mPlayerMeshes[i]->SetShowing(0);
        mPlayerNames[i]->SetText(HxStr(kNoText));
    }
}

// 0x00345ba0
void MetRemixDataScreen::SetRecordShowing(int nShowing) {
    mSongTitleText->SetShowing(nShowing);
    mDateText->SetShowing(nShowing);
    FindMesh(kLabelMesh)->SetShowing(nShowing);
    FindMesh(kLogoMesh)->SetShowing(nShowing);
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mPlayerMeshes[nRow]->SetShowing(nShowing);
        mPlayerNames[nRow]->SetShowing(nShowing);
    }
}

// 0x00345de8
void MetRemixDataScreen::UpdateIdle(float) {
    mLogoTextures.Advance();
    mLogoMesh->SetShowing(0);
    if (mLogoTextures.Current() != nullptr) {
        mLogoMesh->SetShowing(1);
        mLogoMaterial->mStages[kBaseStage].SetTex(mLogoTextures.Current());
    }

    mLabelTextures.Advance();
    mLabelMesh->SetShowing(0);
    if (mLabelTextures.Current() != nullptr) {
        mLabelMesh->SetShowing(1);
        mLabelMaterial->mStages[kBaseStage].SetTex(mLabelTextures.Current());
    }
}

// 0x00349998
MetRemixDataScreen *MetRemixDataScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetRemixDataScreen(pRenderer, nPriority);
}

// 0x00349a20
void MetRemixDataScreen::EnterAndShow() {
    SetRecordShowing(0);
    MetScreen::EnterAndShow();
    mUnavailableText->SetShowing(0);
    mLabelMesh->SetShowing(0);
    mLabelTextures.invalidate();
    mLogoMesh->SetShowing(0);
    mLogoTextures.invalidate();
}

// 0x00349ab8
void MetRemixDataScreen::OnExitFinished() {
    mUnavailableText->SetShowing(0);
}
