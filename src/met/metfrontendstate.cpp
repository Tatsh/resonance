#include "met/metfrontendstate.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "met/metpersonadata.h"

namespace {

// The instance Create() allocates. 0x00699d70.
MetFrontEndState *g_pFrontEndState = nullptr;

} // namespace

MetFrontEndState::MetFrontEndState() {
    Reset();
}

MetFrontEndState::~MetFrontEndState() {
    if (mUnknown00.size() != 0) {
        for (std::vector<MetPersonaData *>::iterator it = mUnknown00.begin();
             it != mUnknown00.end();
             ++it) {
            delete *it;
        }
        mUnknown00.erase(mUnknown00.begin(), mUnknown00.end());
    }
}

MetFrontEndState *MetFrontEndState::shared() {
    return g_pFrontEndState;
}

void MetFrontEndState::Create() {
    g_pFrontEndState = new MetFrontEndState();
}

void MetFrontEndState::Destroy() {
    delete g_pFrontEndState;
    g_pFrontEndState = nullptr;
}

void MetFrontEndState::Reset() {
    mUnknown14 = 0;
    mUnknown18 = 0;
    mUnknown1c = 0;
    mUnknown20 = 0;
    mUnknown0c = 0;
    mUnknown2c = 0;
    mUnknown10 = 0;
}

MetPersonaData *MetFrontEndState::GetFirstPersona() {
    // Yes, the binary copies the whole vector to read its first element.
    std::vector<MetPersonaData *> personas(*Application::shared()->GetGameManager()->GetPersonas());
    if (personas.size() == 0) {
        return nullptr;
    }
    return personas[0];
}
