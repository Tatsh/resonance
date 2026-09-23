#include "met/metmsgscreen.h"

#include <vector>

#include "met/metbuttonlist.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/view.h"

namespace {

// The screen name, the directory the container loads from, and the container name.
static const char *const kScreenName = "dlg";
static const char *const kDirectory = "metagame/Shared";
static const char *const kContainerName = "dialogue";

// The registry keys Show() and SetOwnerPad() look up.
static const char *const kMsgScreen = "MetMsgScreen";
static const char *const kSonyScreen = "MetSonyScreen";

// The button layout view, formatted with the button count.
static const char *const kButtonViewFormat = "dlg_%dbut.view";

// The frame views Refresh() picks from by the line count of the text, and their parent.
static const char *const kSmallFrameView = "dlg_small.view";
static const char *const kMediumFrameView = "dlg_medium.view";
static const char *const kLargeFrameView = "dlg_large.view";
static const char *const kFrameGroupView = "dlg_group.view";
constexpr int kSmallFrameLines = 3;
constexpr int kMediumFrameLines = 6;

// The two button counts with a button list of their own.
constexpr int kOneButton = 1;
constexpr int kTwoButtons = 2;

// The index Refresh() selects, and the values the constructor and Show() reset to.
constexpr int kFirstButtonIndex = 0;
constexpr int kNoPad = -1;
constexpr int kNoSelection = -1;

inline Rnd::View *FindView(const HxStr &name) {
    return dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(name));
}

} // namespace

// 0x002ec290
MetMsgScreen::MetMsgScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreenMultiSoundBank(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown8c(nullptr), mUnknown90(nullptr), mUnknownd0(0), mUnknownd4(kNoSelection),
      mShowing(0), mOwnerPad(kNoPad) {
}

// 0x002ec450
MetMsgScreen::~MetMsgScreen() {
    delete mUnknown90;
    delete mUnknown8c;
}

// 0x002f02c0
MetMsgScreen *MetMsgScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMsgScreen(pRenderer, nPriority);
}

// 0x002ebc98
void MetMsgScreen::Show(const HxStr &name,
                        const HxStr &title,
                        const HxStr &text,
                        int nButtons,
                        const std::vector<HxStr> &buttons,
                        MetScreen *pOwner) {
    MetMsgScreen *pScreen =
        dynamic_cast<MetMsgScreen *>(MetScreen::FindScreenByName(HxStr(kMsgScreen)));
    pScreen->SetName(name);
    pScreen->SetTitle(title);
    pScreen->SetText(text);
    pScreen->mButtonCount = nButtons;
    pScreen->mOwner = pOwner;
    pScreen->mOwnerPad = kNoPad;
    if (buttons.size() != 0) {
        pScreen->SetButtons(buttons);
    }
    if (pScreen->mShowing != 0) {
        pScreen->Refresh();
    } else {
        MetScreen *pSony = MetScreen::FindScreenByName(HxStr(kSonyScreen));
        pSony->PushNamedScreen(HxStr(kMsgScreen));
        pSony->ActivateNamedPanel(HxStr(kMsgScreen));
    }
}

// 0x002ebf40
void MetMsgScreen::ShowActive(const HxStr &name,
                              const HxStr &title,
                              const HxStr &text,
                              int nButtons,
                              const std::vector<HxStr> &buttons,
                              MetScreen *pOwner) {
    MetMsgScreen *pScreen =
        dynamic_cast<MetMsgScreen *>(MetScreen::FindScreenByName(HxStr(kMsgScreen)));
    pScreen->SetName(name);
    pScreen->SetTitle(title);
    pScreen->SetText(text);
    pScreen->mButtonCount = nButtons;
    pScreen->mOwner = pOwner;
    pScreen->mOwnerPad = kNoPad;
    if (buttons.size() != 0) {
        pScreen->SetButtons(buttons);
    }
    if (pScreen->mShowing != 0) {
        pScreen->Refresh();
        MetScreen::FindScreenByName(HxStr(kSonyScreen))->ActivateNamedPanel(HxStr(kMsgScreen));
    } else {
        MetScreen *pSony = MetScreen::FindScreenByName(HxStr(kSonyScreen));
        pSony->PushNamedScreen(HxStr(kMsgScreen));
        pSony->ActivateNamedPanel(HxStr(kMsgScreen));
    }
}

// 0x002f0348
void MetMsgScreen::SetOwnerPad(int nPad) {
    dynamic_cast<MetMsgScreen *>(MetScreen::FindScreenByName(HxStr(kMsgScreen)))->mOwnerPad = nPad;
}

// 0x002ec5d8
void MetMsgScreen::SetButtons(const std::vector<HxStr> &buttons) {
    if (buttons.size() == 0) {
        return;
    }
    mButtons.clear();
    for (unsigned i = 0; i < buttons.size(); ++i) {
        mButtons.push_back(buttons[i]);
    }
}

// 0x002ecd10
void MetMsgScreen::Refresh() {
    mUnknown98->ClearDraws();
    Rnd::View *pButtons = FindView(HxStr(FormatString(kButtonViewFormat, mButtonCount)));
    mUnknown98->AddDraw(pButtons);
    mUnknowna0->SetText(mTitle);
    mUnknown9c->SetText(mText);

    const int nLines = mUnknown9c->CountLines();
    Rnd::View *pFrame;
    if (nLines < kSmallFrameLines) {
        pFrame = FindView(HxStr(kSmallFrameView));
    } else if (nLines < kMediumFrameLines) {
        pFrame = FindView(HxStr(kMediumFrameView));
    } else {
        pFrame = FindView(HxStr(kLargeFrameView));
    }
    Rnd::View *pGroup = FindView(HxStr(kFrameGroupView));
    pGroup->ClearTransList();
    pGroup->ClearDraws();
    pGroup->AddTrans(pFrame);
    pGroup->AddDraw(pFrame);

    if (mButtonCount == kOneButton) {
        mUnknown94 = mUnknown8c;
    } else if (mButtonCount == kTwoButtons) {
        mUnknown94 = mUnknown90;
    } else {
        mUnknown94 = nullptr;
    }
    if (mUnknown94 != nullptr) {
        for (int i = 0; i < static_cast<int>(mUnknown94->mButtons.size()); ++i) {
            mUnknown94->ButtonAt(i)->mText->SetText(mButtons[i]);
        }
        mUnknown94->SetSelected(kFirstButtonIndex);
    }
    mUnknownd4 = kNoSelection;
}
