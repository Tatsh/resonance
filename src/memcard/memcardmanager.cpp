#include "memcard/memcardmanager.h"

#include "memcard/deleteremixmct.h"
#include "memcard/formatcardmct.h"
#include "memcard/getallconnectstatesmct.h"
#include "memcard/getconnectstatemct.h"
#include "memcard/listremixesmct.h"
#include "memcard/loadglobalsettingsmct.h"
#include "memcard/loadjukeboxplaylistmct.h"
#include "memcard/loadpersonasmct.h"
#include "memcard/loadremixmct.h"
#include "memcard/memcardps2.h"
#include "memcard/memcardtask.h"
#include "memcard/minimumsavespacemct.h"
#include "memcard/saveglobalsettingsmct.h"
#include "memcard/savejukeboxplaylistmct.h"
#include "memcard/savepersonasmct.h"
#include "memcard/saveremixmct.h"

namespace {

// FormatCardMCT's last constructor argument.
constexpr int kFormat = 0;
constexpr int kUnformat = 1;

} // namespace

MemcardManager *MemcardManager::shared() {
    static MemcardManager instance;
    return &instance;
}

MemcardManager::MemcardManager() : mUser(nullptr), mTicket(0) {
    mCard = new MemcardPS2;
}

MemcardManager::~MemcardManager() {
    delete mCard;
}

void MemcardManager::Update() {
    if (mTasks.empty()) {
        mCard->Update();
        return;
    }
    if (mTasks.front()->mState == kMemcardTaskFinished) {
        delete mTasks.front();
        mTasks.pop_front();
    }
    // Yes, dropping the last task skips the queue's update for this frame.
    if (mTasks.empty()) {
        return;
    }
    MemcardTask *pTask = mTasks.front();
    if (pTask->mState == kMemcardTaskIdle) {
        pTask->Execute();
    }
    mCard->Update();
}

void MemcardManager::CreateGetConnectStateTask(int nPortSlot) {
    mTasks.push_back(new GetConnectStateMCT(mUser, mCard, nPortSlot, ++mTicket));
}

void MemcardManager::CreateGetAllConnectStatesTask(std::vector<MemcardConnectState> *pStates) {
    mTasks.push_back(new GetAllConnectStatesMCT(mUser, mCard, ++mTicket, pStates));
}

void MemcardManager::CreateMinimumSaveSpaceTask(int nPortSlot) {
    mTasks.push_back(new MinimumSaveSpaceMCT(mUser, mCard, nPortSlot, ++mTicket));
}

void MemcardManager::CreateFormatTask(int nPortSlot) {
    mTasks.push_back(new FormatCardMCT(mUser, mCard, nPortSlot, ++mTicket, kFormat));
}

void MemcardManager::CreateUnformatTask(int nPortSlot) {
    mTasks.push_back(new FormatCardMCT(mUser, mCard, nPortSlot, ++mTicket, kUnformat));
}

void MemcardManager::CreateSavePersonasTask(int nPortSlot,
                                            const std::vector<MetPersonaData *> &roster) {
    mTasks.push_back(new SavePersonasMCT(mUser, mCard, nPortSlot, ++mTicket, roster));
}

void MemcardManager::CreateSaveGlobalSettingsTask(int nPortSlot, GlobalSettings *pSettings) {
    mTasks.push_back(new SaveGlobalSettingsMCT(mUser, mCard, nPortSlot, ++mTicket, pSettings));
}

void MemcardManager::CreateSaveJukeboxPlayListTask(int nPortSlot,
                                                   JukeboxPlayList *pPlayList,
                                                   int nIndex) {
    mTasks.push_back(
        new SaveJukeboxPlayListMCT(mUser, mCard, nPortSlot, ++mTicket, pPlayList, nIndex));
}

void MemcardManager::CreateLoadPersonasTask(int nPortSlot, std::vector<MetPersonaData *> *pRoster) {
    mTasks.push_back(new LoadPersonasMCT(mUser, mCard, nPortSlot, ++mTicket, pRoster));
}

void MemcardManager::CreateLoadGlobalSettingsTask(int nPortSlot, GlobalSettings *pSettings) {
    mTasks.push_back(new LoadGlobalSettingsMCT(mUser, mCard, nPortSlot, ++mTicket, pSettings));
}

void MemcardManager::CreateLoadJukeboxPlayListTask(int nPortSlot,
                                                   JukeboxPlayList *pPlayList,
                                                   int nIndex) {
    mTasks.push_back(
        new LoadJukeboxPlayListMCT(mUser, mCard, nPortSlot, ++mTicket, pPlayList, nIndex));
}

void MemcardManager::CreateSaveRemixTask(int nPortSlot,
                                         const HxStr &remixName,
                                         const std::vector<FreqAppearance> &appearances,
                                         const HxStr &levelName,
                                         int nAlbumNum) {
    mTasks.push_back(new SaveRemixMCT(
        mUser, mCard, nPortSlot, ++mTicket, remixName, appearances, levelName, nAlbumNum));
}

void MemcardManager::CreateListRemixesTask(int nPortSlot, std::vector<MetRemixRecord> *pRecords) {
    mTasks.push_back(new ListRemixesMCT(mUser, mCard, nPortSlot, ++mTicket, pRecords));
}

void MemcardManager::CreateLoadRemixTask(int nPortSlot, const HxStr &remixName) {
    mTasks.push_back(new LoadRemixMCT(mUser, mCard, nPortSlot, ++mTicket, remixName));
}

void MemcardManager::CreateDeleteRemixTask(int nPortSlot, const HxStr &remixName) {
    mTasks.push_back(new DeleteRemixMCT(mUser, mCard, nPortSlot, ++mTicket, remixName));
}
