#include "met/metfrontendstate.h"

#include <vector>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "met/metpersonadata.h"

namespace {

// The instance Create() allocates. 0x00699d70.
MetFrontEndState *g_pFrontEndState = nullptr;

} // namespace

// NTSC-U/C: 0x00215488, PAL: 0x0021f0b8
MetFrontEndState::MetFrontEndState() {
    Reset();
}

// NTSC-U/C: 0x00215568, PAL: 0x0021f1b0
MetFrontEndState::~MetFrontEndState() {
    if (mPersonas.size() != 0) {
        for (std::vector<MetPersonaData *>::iterator it = mPersonas.begin(); it != mPersonas.end();
             ++it) {
            delete *it;
        }
        mPersonas.erase(mPersonas.begin(), mPersonas.end());
    }
}

// NTSC-U/C: 0x00217f30, PAL: 0x00221bb8
MetFrontEndState *MetFrontEndState::shared() {
    return g_pFrontEndState;
}

// NTSC-U/C: 0x00217f40, PAL: 0x00221bc8
void MetFrontEndState::Create() {
    g_pFrontEndState = new MetFrontEndState();
}

// NTSC-U/C: 0x00217fa0, PAL: 0x00221c28
void MetFrontEndState::Destroy() {
    delete g_pFrontEndState;
    g_pFrontEndState = nullptr;
}

// NTSC-U/C: 0x00217fd8, PAL: 0x00221c60
void MetFrontEndState::Reset() {
    mUnlockAll = 0;
    mPendingTransition = 0;
    mLastTransition = 0;
    mUnusedFlag = 0;
    mUsingMemcard = 0;
    mPlayerCount = 0;
    mSettingsDirty = 0;
}

// NTSC-U/C: 0x002156b0, PAL: 0x0021f310
MetPersonaData *MetFrontEndState::GetFirstPersona() {
    // Yes, the binary copies the whole vector to read its first element.
    std::vector<MetPersonaData *> personas(*Application::shared()->GetGameManager()->GetPersonas());
    if (personas.size() == 0) {
        return nullptr;
    }
    return personas[0];
}

// NTSC-U/C: 0x002159f8, PAL: 0x0021f658
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

// NTSC-U/C: 0x00215b88, PAL: 0x0021f7e8
std::vector<MetPersonaData *> &MetPersonaData::SavedListStorage() {
    static std::vector<MetPersonaData *> list;
    return list;
}

// NTSC-U/C: 0x00215be0, PAL: 0x0021f840
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

// NTSC-U/C: 0x00215ca0, PAL: 0x0021f900
std::vector<MetPersonaData *> &MetPersonaData::LoadListStorage() {
    static std::vector<MetPersonaData *> list;
    return list;
}

// NTSC-U/C: 0x00215cf8, PAL: 0x0021f958
void MetPersonaData::ClearLoadList() {
    if (LoadListStorage().size() == 0) {
        return;
    }
    for (std::vector<MetPersonaData *>::size_type i = 0; i < LoadListStorage().size(); ++i) {
        delete LoadListStorage()[i];
    }
    LoadListStorage().erase(LoadListStorage().begin(), LoadListStorage().end());
}
