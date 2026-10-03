#include "met/metjukeboxbasescreen.h"

#include "met/albumcache.h"
#include "met/methelpscreen.h"
#include "met/metremixmanager.h"
#include "met/metrenderer.h"
#include "met/metstrings.h"
#include "met/scrollinglist.h"
#include "met/texturepairrecord.h"
#include "os/hxstr.h"
#include "rnd/drawable.h"
#include "rnd/font.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/text.h"
#include "rnd/view.h"
#include "script/configquery.h"

namespace {

// The two textures the song logo alternates between.
static const char *const kSongLogoTexture1 = "gSongLogo1.tex";
static const char *const kSongLogoTexture2 = "gSongLogo2.tex";
// The two textures the song label alternates between.
static const char *const kSongLabelTexture1 = "gSongLabel1.tex";
static const char *const kSongLabelTexture2 = "gSongLabel2.tex";

// The row pitch and the visible row count the child slot 38 hands to the ScrollingList
// constructor.
constexpr int kScrollingListRowPitch = 16;
constexpr int kScrollingListRowCount = 10;

// The two list contexts ProvideText() distinguishes.
constexpr int kCatalogueContext = 0;
constexpr int kPlayListContext = 1;

// The text a row past the end of its list shows.
static const char *const kNoText = "";

static const char *const kSelectedFont = "font1_pink_2";
static const char *const kUnselectedFont = "font1_pinkgrey_2";

static const char *const kHelpLayout = "met_jukebox_base_screen_help_tab";
static const char *const kHelpText = "met_jukebox_base_screen_ticker_tape";
static const char *const kFullLayout = "met_jukebox_base_screen_error_tab";
static const char *const kFullText = "met_jukebox_base_screen_error_ticker_tape";

static const char *const kTempoSuffix = " bpm";

// The configuration codes the detail texts are read under, keyed by the record's level name.
constexpr int kDetailConfigCode1 = 0x321;
constexpr int kDetailConfigCode2 = 0x322;
constexpr int kDetailConfigCode3 = 0x325;
constexpr int kDetailConfigCode3Short = 0x327;

// The number of player appearance texts slot 40 empties.
constexpr int kAppearanceTextCount = 4;

constexpr int kFirstRow = 0;

// NTSC-U/C: 0x0069ad78, PAL: 0x006dcf10
// The most entries the playlist takes.
int g_nMaxPlayListEntries = 50;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

} // namespace

// NTSC-U/C: 0x0021dcc0, PAL: 0x00230830
MetJukeboxBaseScreen::MetJukeboxBaseScreen(MetRenderer *pRenderer,
                                           int nPriority,
                                           const HxStr &name,
                                           const HxStr &directory,
                                           const HxStr &file)
    : MetScreen(pRenderer, nPriority, name, directory, file), mListRowPitch(kScrollingListRowPitch),
      mListRowCount(kScrollingListRowCount), mCatalogueList(nullptr), mPlayListList(nullptr),
      mCatalogue(nullptr), mGenreText(nullptr), mTempoText(nullptr), mSongTitleText(nullptr),
      mDateText(nullptr), mRemixTitleText(nullptr), mPlayListCaption(nullptr),
      mPictureMaterial(nullptr), mLogoMaterial(nullptr), mPlayList(nullptr), mCatalogueKey(0),
      mAvailableFont(nullptr), mUnavailableFont(nullptr),
      mLogoTextures(HxStr(kSongLogoTexture1), HxStr(kSongLogoTexture2)),
      mPictureTextures(HxStr(kSongLabelTexture1), HxStr(kSongLabelTexture2)), mPictureMesh(nullptr),
      mLogoMesh(nullptr), mWarningText(nullptr), mPicturesPending(0) {
    mShowsLoadedDrawables = 0;
}

// NTSC-U/C: 0x0021dfc0, PAL: 0x00230bb0
MetJukeboxBaseScreen::~MetJukeboxBaseScreen() {
    delete mCatalogueList;
    delete mPlayListList;
}

// NTSC-U/C: 0x0021e0f0, PAL: 0x00230ce0
void MetJukeboxBaseScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mAvailableFont = dynamic_cast<Rnd::Font *>(Rnd::TheManager.Find(HxStr(kSelectedFont)));
    mUnavailableFont = dynamic_cast<Rnd::Font *>(Rnd::TheManager.Find(HxStr(kUnselectedFont)));
}

// NTSC-U/C: 0x0021e268, PAL: 0x00230ea0
void MetJukeboxBaseScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mCatalogueList->scrollUp();
        ShowRemixDetails();
        UpdateHelpText();
        break;

    case kMetScreenCommandNext:
        mCatalogueList->scrollDown();
        ShowRemixDetails();
        UpdateHelpText();
        break;

    case kMetScreenCommandSelect: {
        UpdateHelpText();
        std::vector<MetRemixRecord> *pRecords = mCatalogue;
        if (pRecords->size() == 0) {
            return;
        }
        if (mPlayList->entries.size() < static_cast<unsigned>(g_nMaxPlayListEntries)) {
            MetRemixRecord &record = (*pRecords)[mCatalogueList->getSelected()];
            if (record.albumNumber != GetAlbumJukeboxValue()) {
                return;
            }
            mPlayList->AddEntry(record);
            mPlayListList->setItemCount(mPlayList->entries.size());
            mPlayListList->setSelected(mPlayList->entries.size() - 1);
        }
        mCatalogueList->refresh();
        mPlayListList->refresh();
        break;
    }

    default:
        break;
    }
}

// NTSC-U/C: 0x0021e3f8, PAL: 0x00231030
void MetJukeboxBaseScreen::BindLists() {
    mPlayList = &MetRemixManager::shared()->mPlayList;
    mCatalogue = &MetRemixManager::shared()->mRemixes[mCatalogueKey];
    if (mCatalogueList != nullptr) {
        mCatalogueList->setItemCount(mCatalogue->size());
    }
    mPlayListList->setItemCount(mPlayList->entries.size());
    mPlayListList->setSelected(mPlayList->entries.size() - 1);
}

// NTSC-U/C: 0x0021e908, PAL: 0x00231540
void MetJukeboxBaseScreen::EnterAndShow() {
    MetScreen::EnterAndShow();
    mGenreText->SetShowing(1);
    mTempoText->SetShowing(1);
    mSongTitleText->SetShowing(1);
    mDateText->SetShowing(1);
    mRemixTitleText->SetShowing(1);

    mPlayList = &MetRemixManager::shared()->mPlayList;
    mCatalogue = &MetRemixManager::shared()->mRemixes[mCatalogueKey];
    if (mCatalogueList != nullptr) {
        mCatalogueList->setSelected(kFirstRow);
        mCatalogueList->setItemCount(mCatalogue->size());
    }
    mPlayListList->setItemCount(mPlayList->entries.size());
    mPlayListList->setSelected(mPlayList->entries.size() - 1);
    if (mCatalogueList != nullptr) {
        mCatalogueList->refresh();
    }
    mPlayListList->refresh();

    mPictureMesh->SetShowing(0);
    mLogoMesh->SetShowing(0);
    ShowRemixDetails();
}

// NTSC-U/C: 0x0021ef30, PAL: 0x00231b68
int MetJukeboxBaseScreen::ProvideText(int nItem, int, Rnd::Text *pText, int nContext) {
    if (nContext == kCatalogueContext) {
        if (static_cast<unsigned>(nItem) < mCatalogue->size()) {
            const MetRemixRecord &record = (*mCatalogue)[nItem];
            pText->SetText(record.name);
            pText->SetFont(record.albumNumber == GetAlbumJukeboxValue() ? mAvailableFont :
                                                                          mUnavailableFont);
        } else {
            pText->SetText(HxStr(kNoText));
        }
    } else if (nContext == kPlayListContext) {
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

// NTSC-U/C: 0x0021f3e8, PAL: 0x00232060
void MetJukeboxBaseScreen::ShowRemixDetails() {
    mGenreText->SetText(HxStr(kNoText));
    mTempoText->SetText(HxStr(kNoText));
    mSongTitleText->SetText(HxStr(kNoText));
    mDateText->SetText(HxStr(kNoText));
    mRemixTitleText->SetText(HxStr(kNoText));
    for (int i = 0; i < kAppearanceTextCount; ++i) {
        mAppearanceTexts[i]->SetText(HxStr(kNoText));
    }
    mWarningText->SetShowing(0);

    mPicturesPending = 0;
    if (mCatalogue == nullptr || mCatalogue->size() == 0) {
        return;
    }

    MetRemixRecord &record = (*mCatalogue)[mCatalogueList->getSelected()];
    const int bOtherAlbum = record.albumNumber != GetAlbumJukeboxValue();
    if (bOtherAlbum) {
        mWarningText->SetShowing(1);
        mPictureMesh->SetShowing(0);
        mLogoMesh->SetShowing(0);
    } else {
        mLogoTextures.Load(TexturePairRecord::LogoPath(record.levelName));
        mPictureTextures.Load(TexturePairRecord::PicturePath(record.levelName));
        HxStr first = QueryConfigString(kDetailConfigCode1, TextOrEmpty(record.levelName));
        HxStr second = QueryConfigString(kDetailConfigCode2, TextOrEmpty(record.levelName));
        mGenreText->SetText(HxStr(TextOrEmpty(first)));
        second += HxStr(kTempoSuffix);
        mTempoText->SetText(HxStr(TextOrEmpty(second)));
    }

    mRemixTitleText->SetText(record.name);
    const int nAppearances = record.appearances.size();
    for (int i = 0; i < nAppearances; ++i) {
        mAppearanceTexts[i]->SetShowing(1);
        mAppearanceTexts[i]->SetText(record.appearances[i].mUserName);
    }

    if (!bOtherAlbum) {
        HxStr third = QueryConfigString(kDetailConfigCode3, TextOrEmpty(record.levelName));
        const float flWrapWidth = mSongTitleText->mWrapWidth;
        if (flWrapWidth < mSongTitleText->GetFontWidth(TextOrEmpty(third), third.mLen)) {
            HxStr shorter =
                QueryConfigString(kDetailConfigCode3Short, TextOrEmpty(record.levelName));
            third = shorter;
        }
        mSongTitleText->SetText(third);
    }

    mDateText->SetText(record.dateTime);
    if (!bOtherAlbum) {
        mPicturesPending = 1;
    }
}

// NTSC-U/C: 0x0021fc88, PAL: 0x00232a80
void MetJukeboxBaseScreen::UpdateIdle([[maybe_unused]] float flTime) {
    if (!mView->GetShowing() || mPicturesPending == 0) {
        return;
    }

    int bLabelShown = 0;
    int bLogoShown = 0;

    if (mLogoTextures.Advance()) {
        mLogoMesh->SetShowing(0);
        Rnd::Tex *pTex = mLogoTextures.Current();
        if (pTex != nullptr) {
            const int nCount = GetItemCount();
            int bJukeboxAlbum = 1;
            if (mCatalogueList != nullptr) {
                bJukeboxAlbum = (*mCatalogue)[mCatalogueList->getSelected()].albumNumber ==
                                GetAlbumJukeboxValue();
            }
            if (nCount > 0 && bJukeboxAlbum) {
                bLogoShown = 1;
                mLogoMesh->SetShowing(1);
                mLogoMaterial->mStages[0].SetTex(pTex);
            }
        }
    }

    if (mPictureTextures.Advance()) {
        mPictureMesh->SetShowing(0);
        Rnd::Tex *pTex = mPictureTextures.Current();
        if (pTex != nullptr) {
            const int nCount = GetItemCount();
            int bJukeboxAlbum = 1;
            if (mCatalogueList != nullptr) {
                bJukeboxAlbum = (*mCatalogue)[mCatalogueList->getSelected()].albumNumber ==
                                GetAlbumJukeboxValue();
            }
            if (nCount > 0 && bJukeboxAlbum) {
                bLabelShown = 1;
                mPictureMesh->SetShowing(1);
                mPictureMaterial->mStages[0].SetTex(pTex);
            }
        }
    }

    if (bLabelShown && bLogoShown) {
        mPicturesPending = 0;
    }
}

// NTSC-U/C: 0x0021fe98, PAL: 0x00232c90
void MetJukeboxBaseScreen::UpdateHelpText() {
    if (mPlayList->entries.size() < static_cast<unsigned>(g_nMaxPlayListEntries)) {
        MetHelpScreen::SelectPreset(MetText(kMetStrHMetJukeboxBaseScreenHelpTab, kHelpLayout));
        MetHelpScreen::SetText(MetText(kMetStrHMetJukeboxBaseScreenTickerTape, kHelpText),
                               mRenderer->mAnimationFrame);
    } else {
        MetHelpScreen::SelectPreset(MetText(kMetStrHMetJukeboxBaseScreenErrorTab, kFullLayout));
        MetHelpScreen::SetText(MetText(kMetStrHMetJukeboxBaseScreenErrorTickerTape, kFullText),
                               mRenderer->mAnimationFrame);
    }
}

// NTSC-U/C: 0x00224968, PAL: 0x002378f8
void MetJukeboxBaseScreen::PlaySlideSound(int) {
}

// NTSC-U/C: 0x00224970, PAL: 0x00237900
void MetJukeboxBaseScreen::PlayLeaveSound(int) {
}

// NTSC-U/C: 0x00224978, PAL: 0x00237908
void MetJukeboxBaseScreen::PlayHighSound(int) {
}

// NTSC-U/C: 0x00224980, PAL: 0x00237910
void MetJukeboxBaseScreen::PlayCycleLeftSound(int) {
}

// NTSC-U/C: 0x00224988, PAL: 0x00237918
void MetJukeboxBaseScreen::PlayCycleRightSound(int) {
}

// NTSC-U/C: 0x00224990, PAL: 0x00237920
void MetJukeboxBaseScreen::OnEnterFinished() {
    BindLists();
    if (mCatalogueList != nullptr) {
        mCatalogueList->refresh();
    }
    mPlayListList->refresh();
}

// NTSC-U/C: 0x002249e0, PAL: 0x00237970
void MetJukeboxBaseScreen::OnExitFinished() {
    mGenreText->SetShowing(0);
    mTempoText->SetShowing(0);
    mSongTitleText->SetShowing(0);
    mDateText->SetShowing(0);
    mRemixTitleText->SetShowing(0);
}

// NTSC-U/C: 0x00224a90, PAL: 0x00237a20
void MetJukeboxBaseScreen::OnPanelActivated() {
    OnEnterFinished();
    UpdateHelpText();
    mPlayListList->setShowing(1);
}

// NTSC-U/C: 0x00224ae8, PAL: 0x00237a78
int MetJukeboxBaseScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 0;
}

// NTSC-U/C: 0x00224af0, PAL: 0x00237a80
void MetJukeboxBaseScreen::SetShowing(int nShowing) {
    MetScreen::SetShowing(nShowing);
    if (mViewsUnresolved != 0) {
        return;
    }
    BindLists();
    mLogoTextures.invalidate();
    mPictureTextures.invalidate();
    if (nShowing != 0) {
        ShowRemixDetails();
    } else {
        // Yes, the binary hides this list without the null check it applies to the same pointer
        // two statements below.
        mPlayListList->setShowing(0);
    }
    if (mCatalogueList != nullptr) {
        mCatalogueList->setEntriesShowing(nShowing);
    }
    if (mPlayListList != nullptr) {
        mPlayListList->setEntriesShowing(nShowing);
    }
    if (mPlayListCaption != nullptr) {
        mPlayListCaption->SetShowing(nShowing);
    }
}
