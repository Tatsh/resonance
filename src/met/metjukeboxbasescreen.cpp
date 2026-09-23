#include "met/metjukeboxbasescreen.h"

#include "met/albumcache.h"
#include "met/scrollinglist.h"
#include "met/texturepairrecord.h"
#include "os/hxstr.h"
#include "rnd/drawable.h"
#include "rnd/text.h"

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

} // namespace

MetJukeboxBaseScreen::MetJukeboxBaseScreen(MetRenderer *pRenderer,
                                           int nPriority,
                                           const HxStr &name,
                                           const HxStr &directory,
                                           const HxStr &file)
    : MetScreen(pRenderer, nPriority, name, directory, file), mUnknown90(kScrollingListRowPitch),
      mUnknown94(kScrollingListRowCount), mUnknown98(nullptr), mUnknown9c(nullptr),
      mUnknowna0(nullptr), mUnknowna4(nullptr), mUnknowna8(nullptr), mUnknownac(nullptr),
      mUnknownb0(nullptr), mUnknownb4(nullptr), mUnknownb8(nullptr), mUnknownbc(0), mUnknownc0(0),
      mUnknownc4(nullptr), mUnknownc8(0), mUnknownd8(nullptr), mUnknowndc(nullptr),
      mUnknowne0(HxStr(kSongLogoTexture1), HxStr(kSongLogoTexture2)),
      mUnknown110(HxStr(kSongLabelTexture1), HxStr(kSongLabelTexture2)), mUnknown140(0),
      mUnknown144(0), mUnknown148(0), mUnknown14c(0) {
    mUnknown60 = 0;
}

MetJukeboxBaseScreen::~MetJukeboxBaseScreen() {
    delete mUnknown98;
    delete mUnknown9c;
}

void MetJukeboxBaseScreen::SetShowing(int nShowing) {
    MetScreen::SetShowing(nShowing);
    if (mUnknown48 != 0) {
        return;
    }
    OnUnknownSlot42();
    mUnknowne0.invalidate();
    mUnknown110.invalidate();
    if (nShowing != 0) {
        OnUnknownSlot40();
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

void MetJukeboxBaseScreen::PlaySlideSound(int) {
}

void MetJukeboxBaseScreen::PlayLeaveSound() {
}

void MetJukeboxBaseScreen::PlayHighSound(int) {
}

void MetJukeboxBaseScreen::PlayCycleLeftSound(int) {
}

void MetJukeboxBaseScreen::PlayCycleRightSound(int) {
}

int MetJukeboxBaseScreen::ProvideText(int nItem, int, Rnd::Text *pText, int nContext) {
    if (nContext == kCatalogueContext) {
        if (static_cast<unsigned>(nItem) < mUnknowna0->size()) {
            const MetRemixRecord &record = (*mUnknowna0)[nItem];
            pText->SetText(record.unknown08_);
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

int MetJukeboxBaseScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 0;
}
