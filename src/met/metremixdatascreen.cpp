#include "met/metremixdatascreen.h"

#include "game/freqappearance.h"
#include "met/albumcache.h"
#include "met/metremixrecord.h"
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

// 0x00344d70
void MetRemixDataScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mUnknownb0 = FindText(kSongTitleText);
    mUnknownb4 = FindText(kDateText);
    mUnknownb8 = FindMaterial(kPhotoMaterial);
    mUnknownbc = FindMaterial(kLogoMaterial);
    mUnknownc0 = FindText(kUnavailableText);
    mUnknownc0->SetShowing(0);
    mUnknownc4 = FindMesh(kLabelMesh);
    mUnknownc8 = FindMesh(kLogoMesh);

    for (int nRow = 1; nRow <= kRowCount; ++nRow) {
        Rnd::Text *pName = FindText(FormatString(kNameTextFormat, nRow));
        mUnknown8c.push_back(pName);
        pName->SetText(HxStr(kNoText));

        Rnd::Mat *pPicture = FindMaterial(FormatString(kPlayerMaterialFormat, nRow));
        mUnknown98.push_back(pPicture);

        Rnd::Mesh *pMesh = FindMesh(FormatString(kPlayerMeshFormat, nRow));
        mUnknowna4.push_back(pMesh);
    }
}

// 0x003455a8
void MetRemixDataScreen::ShowRecord(MetRemixRecord *pRecord) {
    if (mUnknown48 != 0) {
        return;
    }
    SetRecordShowing(1);

    if (pRecord->unknown34_ == GetAlbumJukeboxValue()) {
        HxStr level(pRecord->unknown00_);
        HxStr songName = QueryConfigString(kSongNameConfigCode, TextOrEmpty(level));
        const float flWrapWidth = mUnknownb0->mWrapWidth;
        if (flWrapWidth < mUnknownb0->MeasureText(TextOrEmpty(songName), songName.mLen)) {
            HxStr shorter = QueryConfigString(kShortSongNameConfigCode, TextOrEmpty(level));
            songName = shorter;
        }
        mUnknownb0->SetText(songName);
        mLogoTextures.Load(TexturePairRecord::LogoPath(level));
        mLabelTextures.Load(TexturePairRecord::PicturePath(level));
        mUnknownc0->SetShowing(0);
    } else {
        mUnknownc0->SetShowing(1);
        HxStr notice = QueryConfigString(kPromptConfigCode, kUnavailableKey);
        mUnknownc0->SetText(notice);
        mUnknownb0->SetText(HxStr(kNoText));
        mUnknownc4->SetShowing(0);
        mLabelTextures.CancelLoad();
        mLabelTextures.invalidate();
        mUnknownc8->SetShowing(0);
        mLogoTextures.CancelLoad();
        mLogoTextures.invalidate();
    }

    mUnknownb4->SetText(pRecord->unknown18_);
    const int nCount = pRecord->appearances.size();
    for (int i = 0; i < nCount; ++i) {
        mUnknowna4[i]->SetShowing(1);
        Rnd::Tex *pPicture = FreqAppearance::FindPersonaBurnTexture(i);
        pRecord->appearances[i].AttachToBurnSlot(i);
        mUnknown98[i]->mStages[kPictureStage].SetTex(pPicture);
        HxStr name(pRecord->appearances[i].mUnknown00);
        mUnknown8c[i]->SetText(name);
    }
    for (int i = nCount; i < kRowCount; ++i) {
        mUnknowna4[i]->SetShowing(0);
        mUnknown8c[i]->SetText(HxStr(kNoText));
    }
}

// 0x00345ba0
void MetRemixDataScreen::SetRecordShowing(int nShowing) {
    mUnknownb0->SetShowing(nShowing);
    mUnknownb4->SetShowing(nShowing);
    FindMesh(kLabelMesh)->SetShowing(nShowing);
    FindMesh(kLogoMesh)->SetShowing(nShowing);
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mUnknowna4[nRow]->SetShowing(nShowing);
        mUnknown8c[nRow]->SetShowing(nShowing);
    }
}

// 0x00345de8
void MetRemixDataScreen::OnUnknownSlot26(float) {
    mLogoTextures.Advance();
    mUnknownc8->SetShowing(0);
    if (mLogoTextures.Current() != nullptr) {
        mUnknownc8->SetShowing(1);
        mUnknownbc->mStages[kBaseStage].SetTex(mLogoTextures.Current());
    }

    mLabelTextures.Advance();
    mUnknownc4->SetShowing(0);
    if (mLabelTextures.Current() != nullptr) {
        mUnknownc4->SetShowing(1);
        mUnknownb8->mStages[kBaseStage].SetTex(mLabelTextures.Current());
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
    mUnknownc0->SetShowing(0);
    mUnknownc4->SetShowing(0);
    mLabelTextures.invalidate();
    mUnknownc8->SetShowing(0);
    mLogoTextures.invalidate();
}

// 0x00349ab8
void MetRemixDataScreen::OnUnknownSlot36() {
    mUnknownc0->SetShowing(0);
}
