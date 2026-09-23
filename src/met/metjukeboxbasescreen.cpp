#include "met/metjukeboxbasescreen.h"

#include "met/albumcache.h"
#include "met/methelpscreen.h"
#include "met/metremixmanager.h"
#include "met/metrenderer.h"
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

// The configuration codes the detail texts are read under, keyed by the record's first string.
constexpr int kDetailConfigCode1 = 0x321;
constexpr int kDetailConfigCode2 = 0x322;
constexpr int kDetailConfigCode3 = 0x325;
constexpr int kDetailConfigCode3Short = 0x327;

// The number of player appearance texts slot 40 empties.
constexpr int kAppearanceTextCount = 4;

constexpr int kFirstRow = 0;

// 0x0069ad78. The most entries the playlist takes.
int g_nMaxPlayListEntries = 50;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

} // namespace

// 0x0021dcc0
MetJukeboxBaseScreen::MetJukeboxBaseScreen(MetRenderer *pRenderer,
                                           int nPriority,
                                           const HxStr &name,
                                           const HxStr &directory,
                                           const HxStr &file)
    : MetScreen(pRenderer, nPriority, name, directory, file), mUnknown90(kScrollingListRowPitch),
      mUnknown94(kScrollingListRowCount), mUnknown98(nullptr), mUnknown9c(nullptr),
      mUnknowna0(nullptr), mUnknowna4(nullptr), mUnknowna8(nullptr), mUnknownac(nullptr),
      mUnknownb0(nullptr), mUnknownb4(nullptr), mUnknownb8(nullptr), mUnknownbc(nullptr),
      mUnknownc0(nullptr), mUnknownc4(nullptr), mUnknownc8(0), mUnknownd8(nullptr),
      mUnknowndc(nullptr), mUnknowne0(HxStr(kSongLogoTexture1), HxStr(kSongLogoTexture2)),
      mUnknown110(HxStr(kSongLabelTexture1), HxStr(kSongLabelTexture2)), mUnknown140(nullptr),
      mUnknown144(nullptr), mUnknown148(nullptr), mUnknown14c(0) {
    mUnknown60 = 0;
}

// 0x0021dfc0
MetJukeboxBaseScreen::~MetJukeboxBaseScreen() {
    delete mUnknown98;
    delete mUnknown9c;
}

// 0x0021e0f0
void MetJukeboxBaseScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mUnknownd8 = dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(HxStr(kSelectedFont)));
    mUnknowndc = dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(HxStr(kUnselectedFont)));
}

// 0x0021e268
void MetJukeboxBaseScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mUnknown98->scrollUp();
        ShowRemixDetails();
        UpdateHelpText();
        break;

    case kMetScreenCommandNext:
        mUnknown98->scrollDown();
        ShowRemixDetails();
        UpdateHelpText();
        break;

    case kMetScreenCommandSelect: {
        UpdateHelpText();
        std::vector<MetRemixRecord> *pRecords = mUnknowna0;
        if (pRecords->size() == 0) {
            return;
        }
        if (mUnknownc4->entries.size() < static_cast<unsigned>(g_nMaxPlayListEntries)) {
            MetRemixRecord &record = (*pRecords)[mUnknown98->getSelected()];
            if (record.unknown34_ != GetAlbumJukeboxValue()) {
                return;
            }
            mUnknownc4->AddEntry(record);
            mUnknown9c->setItemCount(mUnknownc4->entries.size());
            mUnknown9c->setSelected(mUnknownc4->entries.size() - 1);
        }
        mUnknown98->refresh();
        mUnknown9c->refresh();
        break;
    }

    default:
        break;
    }
}

// 0x0021e3f8
void MetJukeboxBaseScreen::BindLists() {
    mUnknownc4 = &MetRemixManager::shared()->mPlayList;
    mUnknowna0 = &MetRemixManager::shared()->mRemixes[mUnknownc8];
    if (mUnknown98 != nullptr) {
        mUnknown98->setItemCount(mUnknowna0->size());
    }
    mUnknown9c->setItemCount(mUnknownc4->entries.size());
    mUnknown9c->setSelected(mUnknownc4->entries.size() - 1);
}

// 0x0021e908
void MetJukeboxBaseScreen::EnterAndShow() {
    MetScreen::EnterAndShow();
    mUnknowna4->SetShowing(1);
    mUnknowna8->SetShowing(1);
    mUnknownac->SetShowing(1);
    mUnknownb0->SetShowing(1);
    mUnknownb4->SetShowing(1);

    mUnknownc4 = &MetRemixManager::shared()->mPlayList;
    mUnknowna0 = &MetRemixManager::shared()->mRemixes[mUnknownc8];
    if (mUnknown98 != nullptr) {
        mUnknown98->setSelected(kFirstRow);
        mUnknown98->setItemCount(mUnknowna0->size());
    }
    mUnknown9c->setItemCount(mUnknownc4->entries.size());
    mUnknown9c->setSelected(mUnknownc4->entries.size() - 1);
    if (mUnknown98 != nullptr) {
        mUnknown98->refresh();
    }
    mUnknown9c->refresh();

    mUnknown140->SetShowing(0);
    mUnknown144->SetShowing(0);
    ShowRemixDetails();
}

// 0x0021ef30
int MetJukeboxBaseScreen::ProvideText(int nItem, int, Rnd::Text *pText, int nContext) {
    if (nContext == kCatalogueContext) {
        if (static_cast<unsigned>(nItem) < mUnknowna0->size()) {
            const MetRemixRecord &record = (*mUnknowna0)[nItem];
            pText->SetText(record.name);
            pText->SetFont(record.unknown34_ == GetAlbumJukeboxValue() ? mUnknownd8 : mUnknowndc);
        } else {
            pText->SetText(HxStr(kNoText));
        }
    } else if (nContext == kPlayListContext) {
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

// 0x0021f3e8
void MetJukeboxBaseScreen::ShowRemixDetails() {
    mUnknowna4->SetText(HxStr(kNoText));
    mUnknowna8->SetText(HxStr(kNoText));
    mUnknownac->SetText(HxStr(kNoText));
    mUnknownb0->SetText(HxStr(kNoText));
    mUnknownb4->SetText(HxStr(kNoText));
    for (int i = 0; i < kAppearanceTextCount; ++i) {
        mUnknowncc[i]->SetText(HxStr(kNoText));
    }
    mUnknown148->SetShowing(0);

    mUnknown14c = 0;
    if (mUnknowna0 == nullptr || mUnknowna0->size() == 0) {
        return;
    }

    MetRemixRecord &record = (*mUnknowna0)[mUnknown98->getSelected()];
    const int bOtherAlbum = record.unknown34_ != GetAlbumJukeboxValue();
    if (bOtherAlbum) {
        mUnknown148->SetShowing(1);
        mUnknown140->SetShowing(0);
        mUnknown144->SetShowing(0);
    } else {
        mUnknowne0.Load(TexturePairRecord::LogoPath(record.unknown00_));
        mUnknown110.Load(TexturePairRecord::PicturePath(record.unknown00_));
        HxStr first;
        QueryConfigString(&first, kDetailConfigCode1, TextOrEmpty(record.unknown00_));
        HxStr second;
        QueryConfigString(&second, kDetailConfigCode2, TextOrEmpty(record.unknown00_));
        mUnknowna4->SetText(HxStr(TextOrEmpty(first)));
        second += HxStr(kTempoSuffix);
        mUnknowna8->SetText(HxStr(TextOrEmpty(second)));
    }

    mUnknownb4->SetText(record.name);
    const int nAppearances = record.appearances.size();
    for (int i = 0; i < nAppearances; ++i) {
        mUnknowncc[i]->SetShowing(1);
        mUnknowncc[i]->SetText(record.appearances[i].mUnknown00);
    }

    if (!bOtherAlbum) {
        HxStr third;
        QueryConfigString(&third, kDetailConfigCode3, TextOrEmpty(record.unknown00_));
        const float flWrapWidth = mUnknownac->mWrapWidth;
        if (flWrapWidth < mUnknownac->MeasureText(TextOrEmpty(third), third.mLen)) {
            HxStr shorter;
            QueryConfigString(&shorter, kDetailConfigCode3Short, TextOrEmpty(record.unknown00_));
            third = shorter;
        }
        mUnknownac->SetText(third);
    }

    mUnknownb0->SetText(record.unknown18_);
    if (!bOtherAlbum) {
        mUnknown14c = 1;
    }
}

// 0x0021fc88
void MetJukeboxBaseScreen::OnUnknownSlot26([[maybe_unused]] float flTime) {
    if (!mUnknown14->GetShowing() || mUnknown14c == 0) {
        return;
    }

    int bLabelShown = 0;
    int bLogoShown = 0;

    if (mUnknowne0.Advance()) {
        mUnknown144->SetShowing(0);
        Rnd::Tex *pTex = mUnknowne0.Current();
        if (pTex != nullptr) {
            const int nCount = GetItemCount();
            int bJukeboxAlbum = 1;
            if (mUnknown98 != nullptr) {
                bJukeboxAlbum =
                    (*mUnknowna0)[mUnknown98->getSelected()].unknown34_ == GetAlbumJukeboxValue();
            }
            if (nCount > 0 && bJukeboxAlbum) {
                bLogoShown = 1;
                mUnknown144->SetShowing(1);
                mUnknownc0->mStages[0].SetTex(pTex);
            }
        }
    }

    if (mUnknown110.Advance()) {
        mUnknown140->SetShowing(0);
        Rnd::Tex *pTex = mUnknown110.Current();
        if (pTex != nullptr) {
            const int nCount = GetItemCount();
            int bJukeboxAlbum = 1;
            if (mUnknown98 != nullptr) {
                bJukeboxAlbum =
                    (*mUnknowna0)[mUnknown98->getSelected()].unknown34_ == GetAlbumJukeboxValue();
            }
            if (nCount > 0 && bJukeboxAlbum) {
                bLabelShown = 1;
                mUnknown140->SetShowing(1);
                mUnknownbc->mStages[0].SetTex(pTex);
            }
        }
    }

    if (bLabelShown && bLogoShown) {
        mUnknown14c = 0;
    }
}

// 0x0021fe98
void MetJukeboxBaseScreen::UpdateHelpText() {
    if (mUnknownc4->entries.size() < static_cast<unsigned>(g_nMaxPlayListEntries)) {
        MetHelpScreen::SelectPreset(HxStr(kHelpLayout));
        MetHelpScreen::SetText(HxStr(kHelpText), mUnknown10->mUnknown68);
    } else {
        MetHelpScreen::SelectPreset(HxStr(kFullLayout));
        MetHelpScreen::SetText(HxStr(kFullText), mUnknown10->mUnknown68);
    }
}

// 0x00224968
void MetJukeboxBaseScreen::PlaySlideSound(int) {
}

// 0x00224970
void MetJukeboxBaseScreen::PlayLeaveSound() {
}

// 0x00224978
void MetJukeboxBaseScreen::PlayHighSound(int) {
}

// 0x00224980
void MetJukeboxBaseScreen::PlayCycleLeftSound(int) {
}

// 0x00224988
void MetJukeboxBaseScreen::PlayCycleRightSound(int) {
}

// 0x00224990
void MetJukeboxBaseScreen::OnUnknownSlot33() {
    BindLists();
    if (mUnknown98 != nullptr) {
        mUnknown98->refresh();
    }
    mUnknown9c->refresh();
}

// 0x002249e0
void MetJukeboxBaseScreen::OnUnknownSlot36() {
    mUnknowna4->SetShowing(0);
    mUnknowna8->SetShowing(0);
    mUnknownac->SetShowing(0);
    mUnknownb0->SetShowing(0);
    mUnknownb4->SetShowing(0);
}

// 0x00224a90
void MetJukeboxBaseScreen::OnUnknownSlot7() {
    OnUnknownSlot33();
    UpdateHelpText();
    mUnknown9c->setShowing(1);
}

// 0x00224ae8
int MetJukeboxBaseScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 0;
}

// 0x00224af0
void MetJukeboxBaseScreen::SetShowing(int nShowing) {
    MetScreen::SetShowing(nShowing);
    if (mUnknown48 != 0) {
        return;
    }
    BindLists();
    mUnknowne0.invalidate();
    mUnknown110.invalidate();
    if (nShowing != 0) {
        ShowRemixDetails();
    } else {
        // Yes, the binary hides this list without the null check it applies to the same pointer
        // two statements below.
        mUnknown9c->setShowing(0);
    }
    if (mUnknown98 != nullptr) {
        mUnknown98->setEntriesShowing(nShowing);
    }
    if (mUnknown9c != nullptr) {
        mUnknown9c->setEntriesShowing(nShowing);
    }
    if (mUnknownb8 != nullptr) {
        mUnknownb8->SetShowing(nShowing);
    }
}
