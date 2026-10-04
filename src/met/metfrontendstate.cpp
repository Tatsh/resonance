#include "met/metfrontendstate.h"

#include <vector>

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
    if (mPersonas.size() != 0) {
        for (std::vector<MetPersonaData *>::iterator it = mPersonas.begin(); it != mPersonas.end();
             ++it) {
            delete *it;
        }
        mPersonas.erase(mPersonas.begin(), mPersonas.end());
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
    mUnlockAll = 0;
    mPendingTransition = 0;
    mLastTransition = 0;
    mUnusedFlag = 0;
    mUsingMemcard = 0;
    mPlayerCount = 0;
    mSettingsDirty = 0;
}

MetPersonaData *MetFrontEndState::GetFirstPersona() {
    // Yes, the binary copies the whole vector to read its first element.
    std::vector<MetPersonaData *> personas(*Application::shared()->GetGameManager()->GetPersonas());
    if (personas.size() == 0) {
        return nullptr;
    }
    return personas[0];
}

void MetPersonaData::CopyList(std::vector<MetPersonaData *> *pDestination,
                              const std::vector<MetPersonaData *> *pSource) {
    if (pDestination->size() != 0) {
        for (std::vector<MetPersonaData *>::iterator it = pDestination->begin();
             it != pDestination->end();
             ++it) {
            delete *it;
        }
        pDestination->erase(pDestination->begin(), pDestination->end());
    }

    for (std::vector<MetPersonaData *>::size_type i = 0; i < pSource->size(); ++i) {
        MetPersonaData *pCopy = new MetPersonaData();
        *pCopy = *(*pSource)[i];
        pDestination->push_back(pCopy);
    }
}

std::vector<MetPersonaData *> &MetPersonaData::SavedListStorage() {
    static std::vector<MetPersonaData *> list;
    return list;
}

void MetPersonaData::ClearSavedList() {
    if (SavedListStorage().size() == 0) {
        return;
    }
    for (std::vector<MetPersonaData *>::iterator it = SavedListStorage().begin();
         it != SavedListStorage().end();
         ++it) {
        delete *it;
    }
    SavedListStorage().erase(SavedListStorage().begin(), SavedListStorage().end());
}

std::vector<MetPersonaData *> &MetPersonaData::LoadListStorage() {
    static std::vector<MetPersonaData *> list;
    return list;
}

void MetPersonaData::ClearLoadList() {
    if (LoadListStorage().size() == 0) {
        return;
    }
    for (std::vector<MetPersonaData *>::size_type i = 0; i < LoadListStorage().size(); ++i) {
        delete LoadListStorage()[i];
    }
    LoadListStorage().erase(LoadListStorage().begin(), LoadListStorage().end());
}
