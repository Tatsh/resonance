#include "met/metfreqmakercanvasscreen.h"

#include <vector>

#include "app/application.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metpersonadata.h"
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

// 0x00891af0. The name a new avatar starts with.
HxStr g_defaultFreqName("player1");

// Resolve the view the avatar view hangs from.
inline Rnd::View *FindCanvasView() {
    return dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kCanvasView)));
}

} // namespace

// 0x0025e5a8
MetFreqMakerCanvasScreen::MetFreqMakerCanvasScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mPersona(nullptr), mModified(0), mFreqName(g_defaultFreqName), mViewAttached(0) {
}

// 0x00262030
MetFreqMakerCanvasScreen::~MetFreqMakerCanvasScreen() {
    mViewAttached = 0;
    mPersona = nullptr;
}

// 0x00261fa8
MetFreqMakerCanvasScreen *MetFreqMakerCanvasScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetFreqMakerCanvasScreen(pRenderer, nPriority);
}

// 0x0025e978
void MetFreqMakerCanvasScreen::EnterAndShow() {
    MetScreen::EnterAndShow();
    Rnd::View *pView = FindCanvasView();
    mAppearance.detachFrom(pView);
    mAppearance.attachTo(pView);
}

// 0x002620d8
int MetFreqMakerCanvasScreen::PollContainerLoad() {
    if (!MetFreqMakerAssetManager::shared()->PollLoad()) {
        return 0;
    }
    return MetScreen::PollContainerLoad();
}

// 0x002620c8
void MetFreqMakerCanvasScreen::HandleCommand(const MetScreenCommand *) {
}

// 0x00261f80
void MetFreqMakerCanvasScreen::PlayCycleLeftSound(int) {
}

// 0x00261f88
void MetFreqMakerCanvasScreen::PlayCycleRightSound(int) {
}

// 0x002620d0
void MetFreqMakerCanvasScreen::OnUnknownSlot30(Rnd::Button *) {
}

// 0x0025e898
void MetFreqMakerCanvasScreen::OnUnknownSlot36() {
    mAppearance.detachFrom(FindCanvasView());
}

// 0x0025e798
void MetFreqMakerCanvasScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    Rnd::View *pView = FindCanvasView();
    mAppearance.detachFrom(pView);
    mAppearance.attachTo(pView);
    mViewAttached = 1;
}

// 0x0025ea68
void MetFreqMakerCanvasScreen::CommitPersona() {
    if (mPersona != nullptr) {
        mPersona->mUnknown140.mDetail->clear();
        mPersona->mUnknown140.mDetail->copyFrom(mAppearance);
        mPersona->mUnknown140.mUnknown00 = mFreqName;
        Application::shared()->GetGameManager()->ClearPersonas();
        Application::shared()->GetGameManager()->AddPersona(*mPersona);
    } else {
        MetPersonaData *pPersona = new MetPersonaData();
        pPersona->mUnknown140.mUnknown00 = mFreqName;
        HxStr date;
        if (FormatCurrentDateTime(date)) {
            pPersona->mUnknown154 = date;
        }
        pPersona->mUnknown140.mDetail->copyFrom(mAppearance);
        Application::shared()->GetGameManager()->ClearPersonas();
        Application::shared()->GetGameManager()->AddPersona(*pPersona);
        // The game manager keeps its own copy, and that copy becomes the persona being edited.
        mPersona = (*Application::shared()->GetGameManager()->GetPersonas())[0];
        delete pPersona;
    }
    mModified = 0;
}

// 0x0025ec80
void MetFreqMakerCanvasScreen::SetFreqName(const HxStr &name) {
    mFreqName = name;
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kNameText)));
    pText->SetText(mFreqName);
    mModified = 1;
}

// 0x00262120
void MetFreqMakerCanvasScreen::SelectTemplate(const HxStr &name) {
    mAppearance.selectTemplate(name);
}

// 0x00262140
void MetFreqMakerCanvasScreen::PlaceCursor() {
    mAppearance.placeCursor();
    mModified = 1;
}

// 0x00262170
void MetFreqMakerCanvasScreen::ResetCursor() {
    mAppearance.resetCursor();
}

// 0x00262190
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

// 0x00262250
void MetFreqMakerCanvasScreen::SetColor(const Color &color, const Vector2 &palettePosition) {
    mAppearance.setColor(color, palettePosition);
}

// 0x00262270
std::list<FreqPart *> &MetFreqMakerCanvasScreen::GetParts() {
    return mAppearance.parts();
}

// 0x00262290
FreqPart *MetFreqMakerCanvasScreen::SelectPart(int nIndex) {
    mModified = 1;
    return mAppearance.selectPart(nIndex);
}

// 0x002622b8
void MetFreqMakerCanvasScreen::LoadPersona(MetPersonaData *pPersona) {
    mPersona = pPersona;
    mAppearance.clear();
    mAppearance.resetCursor();
    if (pPersona != nullptr) {
        mAppearance.copyFrom(*pPersona->mUnknown140.mDetail);
    }
    mFreqName = g_defaultFreqName;
    if (mPersona != nullptr) {
        mFreqName = mPersona->mUnknown140.mUnknown00;
    }
    SetFreqName(mFreqName);
    mModified = 0;
}

// 0x00262350
void MetFreqMakerCanvasScreen::LoadPrefab(MetPersonaData *pSource, int nRandomize) {
    mPersona = nullptr;
    mAppearance.clear();
    mAppearance.resetCursor();
    mAppearance.copyFrom(*pSource->mUnknown140.mDetail);
    mFreqName = g_defaultFreqName;
    if (mPersona != nullptr) { // Yes, mPersona was cleared above, so the source name is never read.
        mFreqName = pSource->mUnknown140.mUnknown00;
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

// 0x00262440
void MetFreqMakerCanvasScreen::RevertSelection() {
    mAppearance.revertSelection();
}

// 0x00262460
HxStr *MetFreqMakerCanvasScreen::GetFreqName() {
    return &mFreqName;
}

// 0x00262468
void MetFreqMakerCanvasScreen::DeletePart(int nIndex) {
    mAppearance.deletePart(nIndex);
    mModified = 1;
}

// 0x00262498
void MetFreqMakerCanvasScreen::Randomize() {
    mAppearance.randomize();
    mModified = 1;
}

// 0x002624c8
void MetFreqMakerCanvasScreen::RecentrePart(int nIndex) {
    mAppearance.recentrePart(nIndex);
    mModified = 1;
}
