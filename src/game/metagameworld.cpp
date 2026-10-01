#include "game/metagameworld.h"

#include "game/inputcheatdetectormet.h"
#include "met/metnullrenderer.h"
#include "met/metrenderer.h"
#include "mid/mbt.h"
#include "msg/metcontrollerreading.h"
#include "msg/rawcontrollermsg.h"
#include "script/configquery.h"

namespace {

// The configuration option that replaces the front-end renderer with MetNullRenderer.
constexpr int kNullRendererOption = 0xcb;

} // namespace

// 0x003d3110
MetaGameWorld::MetaGameWorld() : mRenderer(nullptr), mCheatDetector(nullptr) {
    CreateRenderer();
    mCheatDetector = new InputCheatDetectorMet(&g_metCheatSequences);
}

// 0x003d4790
MetaGameWorld::~MetaGameWorld() {
    delete mCheatDetector;
    DestroyRenderer();
}

// 0x003d3288
void MetaGameWorld::OnControllerReading(int nTag, int nPadIndex, int nButton, float flValue) {
    mCheatDetector->OnControllerReading(nTag, nPadIndex, nButton, flValue);

    MetControllerReading reading;
    reading.mTag = nTag;
    reading.mPadIndex = nPadIndex;
    reading.mButton = nButton;
    reading.mValue = flValue;

    RawControllerMsg message;
    message.mReading = reading;
    message.mPosition = Mid::MBT(0);
    mRenderer->Handle(&message);
}

// 0x003d31c0
void MetaGameWorld::CreateRenderer() {
    if (QueryConfigFlag(kNullRendererOption) != 0) {
        mRenderer = new MetNullRenderer;
    } else {
        mRenderer = new MetRenderer;
    }
}

// 0x003d4810
void MetaGameWorld::DestroyRenderer() {
    delete mRenderer;
    mRenderer = nullptr;
}

// 0x003d4858
RendererBase *MetaGameWorld::GetRenderer() {
    return mRenderer;
}

// 0x003d4860
void MetaGameWorld::StartFrontEnd() {
    mRenderer->Start();
}

// 0x003d4890
void MetaGameWorld::StopFrontEnd() {
    mRenderer->Stop();
}

// 0x003d48c0
int MetaGameWorld::IsAwaitingStart() {
    if (QueryConfigFlag(kNullRendererOption) != 0) {
        return 0;
    }
    return static_cast<MetRenderer *>(mRenderer)->mTitlePromptShowing;
}
