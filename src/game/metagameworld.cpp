#include "game/metagameworld.h"

#include "game/inputcheatdetectormet.h"
#include "met/metnullrenderer.h"
#include "met/metrenderer.h"
#include "mid/tick.h"
#include "msg/metcontrollerreading.h"
#include "msg/rawcontrollermsg.h"
#include "script/configquery.h"

namespace {

// The configuration option that replaces the front-end renderer with MetNullRenderer.
constexpr int kNullRendererOption = 0xcb;

} // namespace

MetaGameWorld::MetaGameWorld() : mRenderer(nullptr), mCheatDetector(nullptr) {
    CreateRenderer();
    mCheatDetector = new InputCheatDetectorMet(&g_metCheatSequences);
}

MetaGameWorld::~MetaGameWorld() {
    delete mCheatDetector;
    KillRenderer();
}

void MetaGameWorld::OnControllerReading(int nTag, int nPadIndex, int nButton, float flValue) {
    mCheatDetector->OnControllerReading(nTag, nPadIndex, nButton, flValue);

    MetControllerReading reading;
    reading.mTag = nTag;
    reading.mPadIndex = nPadIndex;
    reading.mButton = nButton;
    reading.mValue = flValue;

    RawControllerMsg message;
    message.mReading = reading;
    message.mPosition = Sch::Tick(0);
    mRenderer->Dispatch(&message);
}

void MetaGameWorld::CreateRenderer() {
    if (QueryConfigFlag(kNullRendererOption) != 0) {
        mRenderer = new MetNullRenderer;
    } else {
        mRenderer = new MetRenderer;
    }
}

void MetaGameWorld::KillRenderer() {
    delete mRenderer;
    mRenderer = nullptr;
}

RendererBase *MetaGameWorld::GetRenderer() {
    return mRenderer;
}

void MetaGameWorld::StartFrontEnd() {
    mRenderer->Start();
}

void MetaGameWorld::StopFrontEnd() {
    mRenderer->Stop();
}

int MetaGameWorld::IsAwaitingStart() {
    if (QueryConfigFlag(kNullRendererOption) != 0) {
        return 0;
    }
    return static_cast<MetRenderer *>(mRenderer)->mTitlePromptShowing;
}
