#include "met/metremixdelscreen.h"

#include "met/metremixrecord.h"
#include "met/scrollinglist.h"
#include "os/hxstr.h"
#include "rnd/text.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcrd";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memcard_remix_del";

// The one container object the screen registers.
static const char *const kDeleteObjectName = "mem_del_remix";

// This screen's own registry key, and the key of the panel slot 16 activates.
static const char *const kOwnScreenName = "MetRemixDelScreen";
static const char *const kMsgScreenName = "MetMsgScreen";

// The text a row past the end of the catalogue shows.
static const char *const kNoText = "";

} // namespace

MetRemixDelScreen::MetRemixDelScreen(MetRenderer *pRenderer, int nPriority)
    : MetSaveRemix(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknownf4(nullptr), mUnknownfc(0), mUnknown100(0), mUnknown104(0), mUnknown138(0),
      mUnknown13c(0) {
    mUnknown60 = 0;
    mUnknown38.push_back(HxStr(kDeleteObjectName));
}

MetRemixDelScreen::~MetRemixDelScreen() {
    delete mUnknownf4;
}

int MetRemixDelScreen::ProvideText(int nItem, int, Rnd::Text *pText, int) {
    if (static_cast<unsigned>(nItem) < mUnknownf0->size()) {
        MetRemixRecord record((*mUnknownf0)[nItem]);
        pText->SetText(HxStr(record.unknown08_));
    } else {
        pText->SetText(HxStr(kNoText));
    }
    return 1;
}

int MetRemixDelScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 1;
}

void MetRemixDelScreen::OnUnknownSlot7() {
    if (mUnknowne0 != 0) {
        mUnknowne0 = 0;
        OnUnknownSlot40();
    }
}

void MetRemixDelScreen::OnMsgScreenShown(const HxStr &) {
    ActivateNamedPanel(HxStr(kMsgScreenName));
}

void MetRemixDelScreen::OnUnknownSlot33() {
    ShowRowOnDataScreen(0);
}

void MetRemixDelScreen::OnUnknownSlot40() {
    PushNamedScreen(HxStr(kOwnScreenName));
    ActivateNamedPanel(HxStr(kOwnScreenName));
}

void MetRemixDelScreen::OnUnknownSlot41() {
    PushNamedScreen(HxStr(kOwnScreenName));
    ActivateNamedPanel(HxStr(kOwnScreenName));
}

void MetRemixDelScreen::OnUnknownSlot2(const HxStr &text) {
    if (mUnknowne0 != 0) {
        MetSaveRemix::OnUnknownSlot2(text);
        mUnknowne0 = 0;
    }
}
