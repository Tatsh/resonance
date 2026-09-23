#include "met/metsaveremixscreen.h"

#include <vector>

#include "game/freqappearance.h"
#include "memcard/memcardconnectstate.h"
#include "met/metbuttonlist.h"
#include "met/methelpscreen.h"
#include "met/metremixsaver.h"
#include "met/metrenderer.h"
#include "met/metscreen.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
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

// The flag slots 7 and 15 pass to MetRemixSaver::OnUnknownSlot4() when the save screen returns.
constexpr int kSaverReturned = 1;

// The screens the save screen exits or brings back.
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";

// The objects ResolveContainerViews() resolves, and the one button it adds.
static const char *const kFreqNameTextObject = "ers_freqname_player.txt";
static const char *const kInstructionsTextObject = "ers_instructions.txt";
static const char *const kRemixNameTextObject = "ers_remix_title_val.txt";
static const char *const kSaveCopyButton = "save_copy.but";

// The dialogue OnMsgScreenDismissed() handles itself, and its accepting choice.
static const char *const kDiscardDialogue = "discard_remix";
constexpr int kChoiceDiscard = 1;

// What MetScreen::mUnknown18 records for the exit hook to act on.
constexpr int kExitDiscard = 0;
constexpr int kExitToHelp = 2;

inline Rnd::Text *FindText(const char *pszName) {
    return dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(pszName)));
}

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
    mRemixNameText->SetText(text); // The binary dereferences the text object with no null check.
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

// 0x0037af98
void MetSaveRemixScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mFreqNameText = FindText(kFreqNameTextObject);
    mInstructionsText = FindText(kInstructionsTextObject);
    mRemixNameText = FindText(kRemixNameTextObject);
    mUnknowne8->Add(HxStr(kSaveCopyButton), HxStr(kEmptyText));
}

// 0x0037c110
void MetSaveRemixScreen::OnUnknownSlot30([[maybe_unused]] Rnd::Object *pObject) {
    mUnknown18 = kExitToHelp;
    if (mUnknownfc != nullptr) {
        mUnknownfc->OnUnknownSlot3();
    }
    ExitScreenByName(HxStr(kHelpScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    BeginExit();
}

// 0x0037cac8
void MetSaveRemixScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (!(name == kDiscardDialogue)) {
        MetSaveRemix::OnMsgScreenDismissed(name, nChoice);
        return;
    }
    if (nChoice == kChoiceDiscard) {
        mUnknown18 = kExitDiscard;
        mUnknown100 = 0;
        OnUnknownSlot36();
    } else {
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kSaveRemixScreen));
        mUnknownfc->OnUnknownSlot4(kSaverReturned);
        ActivateNamedPanel(HxStr(kSaveRemixScreen));
    }
}

// 0x0037b718
void MetSaveRemixScreen::OnUnknownSlot7() {
    if (mUnknowne0 == 0) {
        MetHelpScreen::SetText(mUnknown38[0], mUnknown10->mUnknown68);
        return;
    }
    mUnknowne0 = 0;
    if (mUnknownfc != nullptr) {
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kSaveRemixScreen));
        mUnknownfc->OnUnknownSlot4(kSaverReturned);
        ActivateNamedPanel(HxStr(kSaveRemixScreen));
    }
}
