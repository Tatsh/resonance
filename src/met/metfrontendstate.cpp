#include "met/metfrontendstate.h"

#include <vector>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "met/metpersonadata.h"

namespace {

// The instance Create() allocates. 0x00699d70.
MetFrontEndState *g_pFrontEndState = nullptr;

} // namespace

// 0x00215488
MetFrontEndState::MetFrontEndState() {
    Reset();
}

// 0x00215568
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

// 0x00217f30
MetFrontEndState *MetFrontEndState::shared() {
    return g_pFrontEndState;
}

// 0x00217f40
void MetFrontEndState::Create() {
    g_pFrontEndState = new MetFrontEndState();
}

// 0x00217fa0
void MetFrontEndState::Destroy() {
    delete g_pFrontEndState;
    g_pFrontEndState = nullptr;
}

// 0x00217fd8
void MetFrontEndState::Reset() {
    mUnknown14 = 0;
    mUnknown18 = 0;
    mUnknown1c = 0;
    mUnknown20 = 0;
    mUnknown0c = 0;
    mUnknown2c = 0;
    mUnknown10 = 0;
}

// 0x002156b0
MetPersonaData *MetFrontEndState::GetFirstPersona() {
    // Yes, the binary copies the whole vector to read its first element.
    std::vector<MetPersonaData *> personas(*Application::shared()->GetGameManager()->GetPersonas());
    if (personas.size() == 0) {
        return nullptr;
    }
    return personas[0];
}

// 0x002159f8
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

// 0x00215b88
std::vector<MetPersonaData *> &MetPersonaData::SavedListStorage() {
    static std::vector<MetPersonaData *> list;
    return list;
}

// 0x00215be0
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

// 0x00215ca0
std::vector<MetPersonaData *> &MetPersonaData::LoadListStorage() {
    static std::vector<MetPersonaData *> list;
    return list;
}

// 0x00215cf8
void MetPersonaData::ClearLoadList() {
    if (LoadListStorage().size() == 0) {
        return;
    }
    for (std::vector<MetPersonaData *>::size_type i = 0; i < LoadListStorage().size(); ++i) {
        delete LoadListStorage()[i];
    }
    LoadListStorage().erase(LoadListStorage().begin(), LoadListStorage().end());
}
