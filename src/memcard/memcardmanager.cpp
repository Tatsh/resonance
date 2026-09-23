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

// 0x001f61b8
MemcardManager *MemcardManager::shared() {
    static MemcardManager instance;
    return &instance;
}

// 0x001f2960
MemcardManager::MemcardManager() : mUser(nullptr), mTicket(0) {
    mCard = new MemcardPS2;
}

// 0x001f6210
MemcardManager::~MemcardManager() {
    delete mCard;
}

// 0x001f3cb0
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

// 0x001f2ae0
void MemcardManager::CreateGetConnectStateTask(int nPortSlot) {
    mTasks.push_back(new GetConnectStateMCT(mUser, mCard, nPortSlot, ++mTicket));
}

// 0x001f2c70
void MemcardManager::CreateGetAllConnectStatesTask(std::vector<MemcardConnectState> *pStates) {
    mTasks.push_back(new GetAllConnectStatesMCT(mUser, mCard, ++mTicket, pStates));
}

// 0x001f2d78
void MemcardManager::CreateMinimumSaveSpaceTask(int nPortSlot) {
    mTasks.push_back(new MinimumSaveSpaceMCT(mUser, mCard, nPortSlot, ++mTicket));
}

// 0x001f2e88
void MemcardManager::CreateFormatTask(int nPortSlot) {
    mTasks.push_back(new FormatCardMCT(mUser, mCard, nPortSlot, ++mTicket, kFormat));
}

// 0x001f2f88
void MemcardManager::CreateUnformatTask(int nPortSlot) {
    mTasks.push_back(new FormatCardMCT(mUser, mCard, nPortSlot, ++mTicket, kUnformat));
}

// 0x001f3090
void MemcardManager::CreateSavePersonasTask(int nPortSlot,
                                            const std::vector<MetPersonaData *> &roster) {
    mTasks.push_back(new SavePersonasMCT(mUser, mCard, nPortSlot, ++mTicket, roster));
}

// 0x001f3328
void MemcardManager::CreateSaveGlobalSettingsTask(int nPortSlot, GlobalSettings *pSettings) {
    mTasks.push_back(new SaveGlobalSettingsMCT(mUser, mCard, nPortSlot, ++mTicket, pSettings));
}

// 0x001f3458
void MemcardManager::CreateSaveJukeboxPlayListTask(int nPortSlot,
                                                   JukeboxPlayList *pPlayList,
                                                   int nIndex) {
    mTasks.push_back(
        new SaveJukeboxPlayListMCT(mUser, mCard, nPortSlot, ++mTicket, pPlayList, nIndex));
}

// 0x001f37f8
void MemcardManager::CreateLoadPersonasTask(int nPortSlot, std::vector<MetPersonaData *> *pRoster) {
    mTasks.push_back(new LoadPersonasMCT(mUser, mCard, nPortSlot, ++mTicket, pRoster));
}

// 0x001f3910
void MemcardManager::CreateLoadGlobalSettingsTask(int nPortSlot, GlobalSettings *pSettings) {
    mTasks.push_back(new LoadGlobalSettingsMCT(mUser, mCard, nPortSlot, ++mTicket, pSettings));
}

// 0x001f3a40
void MemcardManager::CreateLoadJukeboxPlayListTask(int nPortSlot,
                                                   JukeboxPlayList *pPlayList,
                                                   int nIndex) {
    mTasks.push_back(
        new LoadJukeboxPlayListMCT(mUser, mCard, nPortSlot, ++mTicket, pPlayList, nIndex));
}

// 0x001f31c8
void MemcardManager::CreateSaveRemixTask(int nPortSlot,
                                         const HxStr &remixName,
                                         const std::vector<FreqAppearance> &appearances,
                                         const HxStr &levelName,
                                         int nAlbumNum) {
    mTasks.push_back(new SaveRemixMCT(
        mUser, mCard, nPortSlot, ++mTicket, remixName, appearances, levelName, nAlbumNum));
}

// 0x001f3598
void MemcardManager::CreateListRemixesTask(int nPortSlot, std::vector<MetRemixRecord> *pRecords) {
    mTasks.push_back(new ListRemixesMCT(mUser, mCard, nPortSlot, ++mTicket, pRecords));
}

// 0x001f36c8
void MemcardManager::CreateLoadRemixTask(int nPortSlot, const HxStr &remixName) {
    mTasks.push_back(new LoadRemixMCT(mUser, mCard, nPortSlot, ++mTicket, remixName));
}

// 0x001f3b80
void MemcardManager::CreateDeleteRemixTask(int nPortSlot, const HxStr &remixName) {
    mTasks.push_back(new DeleteRemixMCT(mUser, mCard, nPortSlot, ++mTicket, remixName));
}
