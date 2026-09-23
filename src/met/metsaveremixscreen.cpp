#include "met/metsaveremixscreen.h"

#include <vector>

#include "game/freqappearance.h"
#include "memcard/memcardconnectstate.h"
#include "met/metbuttonlist.h"
#include "met/metremixsaver.h"
#include "met/metscreen.h"
#include "os/hxstr.h"
#include "rnd/text.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "ers";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Solo";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "save_remix";

// The one container object the screen registers.
static const char *const kSaveObjectName = "remix_save";

// The registry keys Open() looks up.
static const char *const kSaveRemixScreen = "MetSaveRemixScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";
static const char *const kEmptyText = "";

// The arguments slots 40 and 41 pass to MetRemixSaver::OnUnknownSlot2(), which neither override
// reads.
constexpr int kSlot40SaverArgument = 0;
constexpr int kSlot41SaverArgument = 1;

} // namespace

// 0x0037ace0
MetSaveRemixScreen::MetSaveRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetSaveRemix(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknowne8(nullptr), mUnknownec(0), mUnknown100(0) {
    mUnknowne8 = new MetButtonList;
    mUnknown38.push_back(HxStr(kSaveObjectName));
    mUnknown5c = 0;
    mUnknowne0 = 0;
}

// 0x00381868
MetSaveRemixScreen::~MetSaveRemixScreen() {
    delete mUnknowne8;
}

// 0x0037a9e0
void MetSaveRemixScreen::Open(int nUnknownec,
                              int nPad,
                              MetRemixSaver *pSaver,
                              const MemcardConnectState &slot,
                              const std::vector<FreqAppearance> &appearances,
                              int bClearName) {
    MetSaveRemixScreen *pScreen =
        dynamic_cast<MetSaveRemixScreen *>(MetScreen::FindScreenByName(HxStr(kSaveRemixScreen)));
    pScreen->SetUnknownec(nUnknownec);
    pScreen->SetOwnerPad(nPad);
    pScreen->SetSaver(pSaver);
    pScreen->SetAppearances(appearances);
    pScreen->mUnknown94 = slot;
    if (bClearName != 0) {
        pScreen->SetUnknown104(HxStr(kEmptyText));
    }
    MetScreen *pLoadGame = MetScreen::FindScreenByName(HxStr(kLoadGameScreen));
    pLoadGame->PushNamedScreen(HxStr(kSaveRemixScreen));
    pLoadGame->ActivateNamedPanel(HxStr(kSaveRemixScreen));
}

// 0x00381910
void MetSaveRemixScreen::SetUnknownec(int nUnknownec) {
    mUnknownec = nUnknownec;
}

// 0x00381918
void MetSaveRemixScreen::SetOwnerPad(int nPad) {
    mUnknownc8 = nPad;
}

// 0x00381920
void MetSaveRemixScreen::SetSaver(MetRemixSaver *pSaver) {
    mUnknownfc = pSaver;
}

// 0x00381928
void MetSaveRemixScreen::SetAppearances(const std::vector<FreqAppearance> &appearances) {
    mUnknownac = appearances;
}

// 0x00381948
void MetSaveRemixScreen::SetUnknown104(const HxStr &text) {
    mUnknown104 = text;
}

// 0x00381968
void MetSaveRemixScreen::PlaySlideSound(int nSelector) {
    if (nSelector == mUnknownc8) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

// 0x00381990
void MetSaveRemixScreen::OnUnknownSlot2(const HxStr &text) {
    mUnknownf8->SetText(text); // The binary dereferences the text object with no null check.
    if (mUnknowne0 != 0) {
        MetSaveRemix::OnUnknownSlot2(text);
        mUnknowne0 = 0;
    }
}

// 0x003819f0
void MetSaveRemixScreen::OnUnknownSlot40() {
    if (mUnknownfc != nullptr) {
        mUnknownfc->OnUnknownSlot2(kSlot40SaverArgument);
    }
}

// 0x00381a28
void MetSaveRemixScreen::OnUnknownSlot41() {
    if (mUnknownfc != nullptr) {
        mUnknownfc->OnUnknownSlot2(kSlot41SaverArgument);
    }
}
