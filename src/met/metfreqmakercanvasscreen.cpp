#include "met/metfreqmakercanvasscreen.h"

#include <vector>

#include "app/application.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metpersonadata.h"
#include "met/metstrings.h"
#include "os/datetime.h"
#include "os/r250.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/view.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "fm_canvas";
// The directory the container loads from.
static const char *const kDirectory = "metagame/persona";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "freq_maker_canvas";

// The view the avatar view hangs from, and the text that shows the avatar's name.
static const char *const kCanvasView = "fm_canvas.view";
static const char *const kNameText = "FREQ_NAME.txt";

#ifdef VIDEO_STANDARD_PAL
// The text that shows the canvas title.
static const char *const kTitleText = "FREQ CANVAS.txt";
#endif

// The editing commands HandleCanvasCommand() applies. The binary's jump table covers 13 through 21,
// and its entries for 15 and 20 do nothing.
enum CanvasCommand {
    kCanvasCommandBringForward = 13,
    kCanvasCommandSendBackward = 14,
    kCanvasCommandLeft = 16,
    kCanvasCommandRight = 17,
    kCanvasCommandDown = 18,
    kCanvasCommandUp = 19,
    kCanvasCommandMirror = 21
};

// LoadPrefab() runs randomize() a random number of times below this.
constexpr float kRandomizePasses = 10.0f;

#ifndef VIDEO_STANDARD_PAL
// NTSC-U/C: 0x00891af0
// The name a new avatar starts with. The European release reads it from its text table instead.
HxStr g_defaultFreqName("player1");
#endif

// The name a new avatar starts with.
inline HxStr DefaultFreqName() {
#ifdef VIDEO_STANDARD_PAL
    return GetMetString(kMetStrFmDefaultName);
#else
    return g_defaultFreqName;
#endif
}

// Resolve the view the avatar view hangs from.
inline Rnd::View *FindCanvasView() {
    return dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kCanvasView)));
}

} // namespace

MetFreqMakerCanvasScreen::MetFreqMakerCanvasScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mPersona(nullptr), mModified(0), mFreqName(DefaultFreqName()), mViewAttached(0) {
}

MetFreqMakerCanvasScreen::~MetFreqMakerCanvasScreen() {
    mViewAttached = 0;
    mPersona = nullptr;
}

MetFreqMakerCanvasScreen *MetFreqMakerCanvasScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetFreqMakerCanvasScreen(pRenderer, nPriority);
}

void MetFreqMakerCanvasScreen::EnterAndShow() {
    MetScreen::EnterAndShow();
    Rnd::View *pView = FindCanvasView();
    mAppearance.detachFrom(pView);
    mAppearance.attachTo(pView);
}

int MetFreqMakerCanvasScreen::PollContainerLoad() {
    if (!MetFreqMakerAssetManager::shared()->PollLoad()) {
        return 0;
    }
    return MetScreen::PollContainerLoad();
}

void MetFreqMakerCanvasScreen::HandleCommand(const MetScreenCommand *) {
}

void MetFreqMakerCanvasScreen::PlayCycleLeftSound(int) {
}

void MetFreqMakerCanvasScreen::PlayCycleRightSound(int) {
}

void MetFreqMakerCanvasScreen::OnRepeatingSoundFinished(Rnd::Button *) {
}

void MetFreqMakerCanvasScreen::OnExitFinished() {
    mAppearance.detachFrom(FindCanvasView());
}

void MetFreqMakerCanvasScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    Rnd::View *pView = FindCanvasView();
#ifdef VIDEO_STANDARD_PAL
    Rnd::Text *pTitle = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(kTitleText)));
    pTitle->SetText(GetMetString(kMetStrFmCanvas));
#endif
    mAppearance.detachFrom(pView);
    mAppearance.attachTo(pView);
    mViewAttached = 1;
}

void MetFreqMakerCanvasScreen::CommitPersona() {
    if (mPersona != nullptr) {
        mPersona->mAppearance.mDetail->clear();
        mPersona->mAppearance.mDetail->copyFrom(mAppearance);
        mPersona->mAppearance.mUserName = mFreqName;
        Application::shared()->GetGameManager()->ClearPersonas();
        Application::shared()->GetGameManager()->AddPersona(*mPersona);
    } else {
        MetPersonaData *pPersona = new MetPersonaData();
        pPersona->mAppearance.mUserName = mFreqName;
        HxStr date;
        if (FormatCurrentDateTime(date)) {
            pPersona->mBirthday = date;
        }
        pPersona->mAppearance.mDetail->copyFrom(mAppearance);
        Application::shared()->GetGameManager()->ClearPersonas();
        Application::shared()->GetGameManager()->AddPersona(*pPersona);
        // The game manager keeps its own copy, and that copy becomes the persona being edited.
        mPersona = (*Application::shared()->GetGameManager()->GetPersonas())[0];
        delete pPersona;
    }
    mModified = 0;
}

void MetFreqMakerCanvasScreen::SetFreqName(const HxStr &name) {
    mFreqName = name;
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(kNameText)));
    pText->SetText(mFreqName);
    mModified = 1;
}

void MetFreqMakerCanvasScreen::SelectTemplate(const HxStr &name) {
    mAppearance.selectTemplate(name);
}

void MetFreqMakerCanvasScreen::PlaceCursor() {
    mAppearance.placeCursor();
    mModified = 1;
}

void MetFreqMakerCanvasScreen::ResetCursor() {
    mAppearance.resetCursor();
}

void MetFreqMakerCanvasScreen::HandleCanvasCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kCanvasCommandBringForward:
        mAppearance.bringForward();
        break;
    case kCanvasCommandSendBackward:
        mAppearance.sendBackward();
        break;
    case kCanvasCommandLeft:
        mAppearance.nudgeCursor(-1, 0);
        break;
    case kCanvasCommandRight:
        mAppearance.nudgeCursor(1, 0);
        break;
    case kCanvasCommandDown:
        mAppearance.nudgeCursor(0, -1);
        break;
    case kCanvasCommandUp:
        mAppearance.nudgeCursor(0, 1);
        break;
    case kCanvasCommandMirror:
        mAppearance.toggleMirror();
        break;
    default:
        break;
    }
}

void MetFreqMakerCanvasScreen::SetColor(const Color &color, const Vector2 &palettePosition) {
    mAppearance.setColor(color, palettePosition);
}

std::list<FreqPart *> &MetFreqMakerCanvasScreen::GetParts() {
    return mAppearance.parts();
}

FreqPart *MetFreqMakerCanvasScreen::SelectPart(int nIndex) {
    mModified = 1;
    return mAppearance.selectPart(nIndex);
}

void MetFreqMakerCanvasScreen::LoadPersona(MetPersonaData *pPersona) {
    mPersona = pPersona;
    mAppearance.clear();
    mAppearance.resetCursor();
    if (pPersona != nullptr) {
        mAppearance.copyFrom(*pPersona->mAppearance.mDetail);
    }
    mFreqName = DefaultFreqName();
    if (mPersona != nullptr) {
        mFreqName = mPersona->mAppearance.mUserName;
    }
    SetFreqName(mFreqName);
    mModified = 0;
}

void MetFreqMakerCanvasScreen::LoadPrefab(MetPersonaData *pSource, int nRandomize) {
    mPersona = nullptr;
    mAppearance.clear();
    mAppearance.resetCursor();
    mAppearance.copyFrom(*pSource->mAppearance.mDetail);
    mFreqName = DefaultFreqName();
    if (mPersona != nullptr) { // Yes, mPersona was cleared above, so the source name is never read.
        mFreqName = pSource->mAppearance.mUserName;
    }
    SetFreqName(mFreqName);
    if (nRandomize != 0) {
        for (int nPasses = static_cast<int>(RandomFloat() * kRandomizePasses); nPasses > 0;
             --nPasses) {
            mAppearance.randomize();
        }
    }
    mModified = 1;
}

void MetFreqMakerCanvasScreen::RevertSelection() {
    mAppearance.revertSelection();
}

HxStr *MetFreqMakerCanvasScreen::GetFreqName() {
    return &mFreqName;
}

void MetFreqMakerCanvasScreen::DeletePart(int nIndex) {
    mAppearance.deletePart(nIndex);
    mModified = 1;
}

void MetFreqMakerCanvasScreen::Randomize() {
    mAppearance.randomize();
    mModified = 1;
}

void MetFreqMakerCanvasScreen::RecentrePart(int nIndex) {
    mAppearance.recentrePart(nIndex);
    mModified = 1;
}
