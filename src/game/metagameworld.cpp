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

// NTSC-U/C: 0x003d3110, PAL: 0x0040af90
MetaGameWorld::MetaGameWorld() : mRenderer(nullptr), mCheatDetector(nullptr) {
    CreateRenderer();
    mCheatDetector = new InputCheatDetectorMet(&g_metCheatSequences);
}

// NTSC-U/C: 0x003d4790, PAL: 0x0040c680
MetaGameWorld::~MetaGameWorld() {
    delete mCheatDetector;
    KillRenderer();
}

// NTSC-U/C: 0x003d3288, PAL: 0x0040b108
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

// NTSC-U/C: 0x003d31c0, PAL: 0x0040b040
void MetaGameWorld::CreateRenderer() {
    if (QueryConfigFlag(kNullRendererOption) != 0) {
        mRenderer = new MetNullRenderer;
    } else {
        mRenderer = new MetRenderer;
    }
}

// NTSC-U/C: 0x003d4810, PAL: 0x0040c700
void MetaGameWorld::KillRenderer() {
    delete mRenderer;
    mRenderer = nullptr;
}

// NTSC-U/C: 0x003d4858, PAL: 0x0040c748
RendererBase *MetaGameWorld::GetRenderer() {
    return mRenderer;
}

// NTSC-U/C: 0x003d4860, PAL: 0x0040c750
void MetaGameWorld::StartFrontEnd() {
    mRenderer->Start();
}

// NTSC-U/C: 0x003d4890, PAL: 0x0040c780
void MetaGameWorld::StopFrontEnd() {
    mRenderer->Stop();
}

// NTSC-U/C: 0x003d48c0, PAL: 0x0040c7b0
int MetaGameWorld::IsAwaitingStart() {
    if (QueryConfigFlag(kNullRendererOption) != 0) {
        return 0;
    }
    return static_cast<MetRenderer *>(mRenderer)->mTitlePromptShowing;
}
