#include "met/metmsgscreen.h"

#include <vector>

#include "met/metbuttonlist.h"
#include "met/metrenderer.h"
#include "msg/message.h"
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
constexpr float kNoExitTime = 0.0f;

// How long after the renderer's current time a dialogue with no buttons exits.
constexpr float kNoButtonExitDelay = 480.0f;

// The objects ResolveContainerViews() builds the button lists from and resolves.
static const char *const kTwoButtonFirst = "dlg2_01.but";
static const char *const kTwoButtonSecond = "dlg2_02.but";
static const char *const kOneButtonOnly = "dlg1_01.but";
static const char *const kNoLabel = "";
static const char *const kTitleTextObject = "dlg_warning.txt";
static const char *const kMessageTextObject = "dlg_message.txt";
static const char *const kButtonViewObject = "dlg_buts.view";

// The dialogue with no buttons, which ignores commands and exits on a timer.
constexpr int kNoButtons = 0;

inline Rnd::View *FindView(const HxStr &name) {
    return dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(name));
}

} // namespace

// NTSC-U/C: 0x002ec290, PAL: 0x0030f9d8
MetMsgScreen::MetMsgScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreenMultiSoundBank(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mOneButtonList(nullptr), mTwoButtonList(nullptr), mExitTime(kNoExitTime),
      mChoice(kNoSelection), mShowing(0), mOwnerPad(kNoPad) {
}

// NTSC-U/C: 0x002ec450, PAL: 0x0030fc00
MetMsgScreen::~MetMsgScreen() {
    delete mTwoButtonList;
    delete mOneButtonList;
}

// NTSC-U/C: 0x002f02c0, PAL: 0x00313d38
MetMsgScreen *MetMsgScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMsgScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x002ebc98, PAL: 0x0030f1d8
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

// NTSC-U/C: 0x002ebf40, PAL: 0x0030f4f8
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

// NTSC-U/C: 0x002f0348, PAL: 0x0030f8f0
void MetMsgScreen::SetOwnerPad(int nPad) {
    dynamic_cast<MetMsgScreen *>(MetScreen::FindScreenByName(HxStr(kMsgScreen)))->mOwnerPad = nPad;
}

// NTSC-U/C: 0x002ec5d8, PAL: 0x0030fdc0
void MetMsgScreen::SetButtons(const std::vector<HxStr> &buttons) {
    if (buttons.size() == 0) {
        return;
    }
    mButtons.clear();
    for (unsigned i = 0; i < buttons.size(); ++i) {
        mButtons.push_back(buttons[i]);
    }
}

// NTSC-U/C: 0x002ecd10, PAL: 0x00310640
void MetMsgScreen::Refresh() {
    mButtonView->RemoveAllDraws();
    Rnd::View *pButtons = FindView(HxStr(Rnd::MakeString(kButtonViewFormat, mButtonCount)));
    mButtonView->AddDraw(pButtons);
    mTitleText->SetText(mTitle);
    mMessageText->SetText(mText);

    const int nLines = mMessageText->CountLines();
    Rnd::View *pFrame;
    if (nLines < kSmallFrameLines) {
        pFrame = FindView(HxStr(kSmallFrameView));
    } else if (nLines < kMediumFrameLines) {
        pFrame = FindView(HxStr(kMediumFrameView));
    } else {
        pFrame = FindView(HxStr(kLargeFrameView));
    }
    Rnd::View *pGroup = FindView(HxStr(kFrameGroupView));
    pGroup->RemoveAllTranses();
    pGroup->RemoveAllDraws();
    pGroup->AddTrans(pFrame);
    pGroup->AddDraw(pFrame);

    if (mButtonCount == kOneButton) {
        mButtonList = mOneButtonList;
    } else if (mButtonCount == kTwoButtons) {
        mButtonList = mTwoButtonList;
    } else {
        mButtonList = nullptr;
    }
    if (mButtonList != nullptr) {
        for (int i = 0; i < static_cast<int>(mButtonList->mButtons.size()); ++i) {
            mButtonList->GetButton(i)->mText->SetText(mButtons[i]);
        }
        mButtonList->SetSelected(kFirstButtonIndex);
    }
    mChoice = kNoSelection;
}

// NTSC-U/C: 0x002f0640, PAL: 0x00313ff0
void MetMsgScreen::DispatchPriv(Message *pMsg) {
    pMsg->Type(); // Yes, the binary discards this call's result.
    ForwardToOwner(pMsg);
}

// NTSC-U/C: 0x002f04d0, PAL: 0x00313e80
void MetMsgScreen::EnterAndShow() {
    mShowing = 0;
    Refresh();
    MetScreen::EnterAndShow();
}

// NTSC-U/C: 0x002f0588, PAL: 0x00313f38
void MetMsgScreen::BeginExit() {
    if (mButtonCount == kNoButtons) {
        mExitTime = mRenderer->mAnimationFrame + kNoButtonExitDelay;
    } else {
        MetScreen::BeginExit();
    }
}

// NTSC-U/C: 0x002ecba0, PAL: 0x003104b0
void MetMsgScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (mButtonCount == kNoButtons) {
        return;
    }
    if (mOwnerPad != kNoPad && mOwnerPad != pCommand->mPadIndex) {
        return;
    }
    switch (pCommand->mCommand) {
    case kMetScreenCommandLeft:
        if (mButtonList != nullptr) {
            mButtonList->SelectPrevious();
        }
        break;

    case kMetScreenCommandRight:
        if (mButtonList != nullptr) {
            mButtonList->SelectNext();
        }
        break;

    case kMetScreenCommandSelect:
        if (mButtonList != nullptr) {
            mChoice = mButtonList->mSelected;
        }
        ActivateNamedPanel(HxStr(kNoLabel));
        BeginExit();
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x002f0410, PAL: 0x00313dc0
void MetMsgScreen::PlaySlideSound(int nSelector) {
    if (mButtonCount != kNoButtons && (mOwnerPad == nSelector || mOwnerPad == kNoPad)) {
        MetScreenMultiSoundBank::PlaySlideSound(nSelector);
    }
}

// NTSC-U/C: 0x002f0450, PAL: 0x00313e00
void MetMsgScreen::PlayCycleLeftSound(int nSelector) {
    if (mButtonCount >= kTwoButtons && (mOwnerPad == nSelector || mOwnerPad == kNoPad)) {
        MetScreenMultiSoundBank::PlayCycleLeftSound(nSelector);
    }
}

// NTSC-U/C: 0x002f0490, PAL: 0x00313e40
void MetMsgScreen::PlayCycleRightSound(int nSelector) {
    if (mButtonCount >= kTwoButtons && (mOwnerPad == nSelector || mOwnerPad == kNoPad)) {
        MetScreenMultiSoundBank::PlayCycleRightSound(nSelector);
    }
}

// NTSC-U/C: 0x002f0540, PAL: 0x00313ef0
void MetMsgScreen::UpdateIdle(float flTime) {
    if (mExitTime != kNoExitTime && mExitTime < flTime) {
        mExitTime = kNoExitTime;
        MetScreen::BeginExit();
    }
}

// NTSC-U/C: 0x002f0500, PAL: 0x00313eb0
void MetMsgScreen::OnEnterFinished() {
    mShowing = 1;
    if (mOwner != nullptr) {
        mOwner->OnMsgScreenShown(mName);
    }
}

// NTSC-U/C: 0x002f05d0, PAL: 0x00313f80
void MetMsgScreen::OnExitFinished() {
    mShowing = 0;
    if (mOwner != nullptr) {
        mOwner->OnMsgScreenDismissed(mName, mChoice);
    }
}

// NTSC-U/C: 0x002ec6f8, PAL: 0x0030ff00
void MetMsgScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mTwoButtonList = new MetButtonList;
    mTwoButtonList->Add(HxStr(kTwoButtonFirst), HxStr(kNoLabel));
    mTwoButtonList->Add(HxStr(kTwoButtonSecond), HxStr(kNoLabel));
    mOneButtonList = new MetButtonList;
    mOneButtonList->Add(HxStr(kOneButtonOnly), HxStr(kNoLabel));
    mTitleText = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(kTitleTextObject)));
    mMessageText = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(kMessageTextObject)));
    mButtonView = FindView(HxStr(kButtonViewObject));
}
