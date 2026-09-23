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
void MetaGameWorld::OnUnknownSlot2(int nUnknown1, int nUnknown2, int nUnknown3, float flUnknown4) {
    mCheatDetector->OnUnknownSlot2(nUnknown1, nUnknown2, nUnknown3, flUnknown4);

    MetControllerReading reading;
    reading.mTag = nUnknown1;
    reading.mPadIndex = nUnknown2;
    reading.mButton = nUnknown3;
    reading.mValue = flUnknown4;

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
void MetaGameWorld::OnUnknownForwarder003d4860() {
    mRenderer->OnUnknownSlot4();
}

// 0x003d4890
void MetaGameWorld::OnUnknownForwarder003d4890() {
    mRenderer->OnUnknownSlot5();
}

// 0x003d48c0
int MetaGameWorld::OnUnknownQuery003d48c0() {
    if (QueryConfigFlag(kNullRendererOption) != 0) {
        return 0;
    }
    return static_cast<MetRenderer *>(mRenderer)->mUnknown60;
}
