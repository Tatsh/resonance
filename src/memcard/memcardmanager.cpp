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

// NTSC-U/C: 0x001f61b8, PAL: 0x001fcbc8
MemcardManager *MemcardManager::shared() {
    static MemcardManager instance;
    return &instance;
}

// NTSC-U/C: 0x001f2960, PAL: 0x001f9330
MemcardManager::MemcardManager() : mUser(nullptr), mTicket(0) {
    mCard = new MemcardPS2;
}

// NTSC-U/C: 0x001f6210, PAL: 0x001fcc20
MemcardManager::~MemcardManager() {
    delete mCard;
}

// NTSC-U/C: 0x001f3cb0, PAL: 0x001fa698
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

// NTSC-U/C: 0x001f2ae0, PAL: 0x001f94b0
void MemcardManager::CreateGetConnectStateTask(int nPortSlot) {
    mTasks.push_back(new GetConnectStateMCT(mUser, mCard, nPortSlot, ++mTicket));
}

// NTSC-U/C: 0x001f2c70, PAL: 0x001f9640
void MemcardManager::CreateGetAllConnectStatesTask(std::vector<MemcardConnectState> *pStates) {
    mTasks.push_back(new GetAllConnectStatesMCT(mUser, mCard, ++mTicket, pStates));
}

// NTSC-U/C: 0x001f2d78, PAL: 0x001f9748
void MemcardManager::CreateMinimumSaveSpaceTask(int nPortSlot) {
    mTasks.push_back(new MinimumSaveSpaceMCT(mUser, mCard, nPortSlot, ++mTicket));
}

// NTSC-U/C: 0x001f2e88, PAL: 0x001f9868
void MemcardManager::CreateFormatTask(int nPortSlot) {
    mTasks.push_back(new FormatCardMCT(mUser, mCard, nPortSlot, ++mTicket, kFormat));
}

// NTSC-U/C: 0x001f2f88, PAL: 0x001f9968
void MemcardManager::CreateUnformatTask(int nPortSlot) {
    mTasks.push_back(new FormatCardMCT(mUser, mCard, nPortSlot, ++mTicket, kUnformat));
}

// NTSC-U/C: 0x001f3090, PAL: 0x001f9a70
void MemcardManager::CreateSavePersonasTask(int nPortSlot,
                                            const std::vector<MetPersonaData *> &roster) {
    mTasks.push_back(new SavePersonasMCT(mUser, mCard, nPortSlot, ++mTicket, roster));
}

// NTSC-U/C: 0x001f3328, PAL: 0x001f9d08
void MemcardManager::CreateSaveGlobalSettingsTask(int nPortSlot, GlobalSettings *pSettings) {
    mTasks.push_back(new SaveGlobalSettingsMCT(mUser, mCard, nPortSlot, ++mTicket, pSettings));
}

// NTSC-U/C: 0x001f3458, PAL: 0x001f9e38
void MemcardManager::CreateSaveJukeboxPlayListTask(int nPortSlot,
                                                   JukeboxPlayList *pPlayList,
                                                   int nIndex) {
    mTasks.push_back(
        new SaveJukeboxPlayListMCT(mUser, mCard, nPortSlot, ++mTicket, pPlayList, nIndex));
}

// NTSC-U/C: 0x001f37f8, PAL: 0x001fa1d8
void MemcardManager::CreateLoadPersonasTask(int nPortSlot, std::vector<MetPersonaData *> *pRoster) {
    mTasks.push_back(new LoadPersonasMCT(mUser, mCard, nPortSlot, ++mTicket, pRoster));
}

// NTSC-U/C: 0x001f3910, PAL: 0x001fa2f8
void MemcardManager::CreateLoadGlobalSettingsTask(int nPortSlot, GlobalSettings *pSettings) {
    mTasks.push_back(new LoadGlobalSettingsMCT(mUser, mCard, nPortSlot, ++mTicket, pSettings));
}

// NTSC-U/C: 0x001f3a40, PAL: 0x001fa428
void MemcardManager::CreateLoadJukeboxPlayListTask(int nPortSlot,
                                                   JukeboxPlayList *pPlayList,
                                                   int nIndex) {
    mTasks.push_back(
        new LoadJukeboxPlayListMCT(mUser, mCard, nPortSlot, ++mTicket, pPlayList, nIndex));
}

// NTSC-U/C: 0x001f31c8, PAL: 0x001f9ba8
void MemcardManager::CreateSaveRemixTask(int nPortSlot,
                                         const HxStr &remixName,
                                         const std::vector<FreqAppearance> &appearances,
                                         const HxStr &levelName,
                                         int nAlbumNum) {
    mTasks.push_back(new SaveRemixMCT(
        mUser, mCard, nPortSlot, ++mTicket, remixName, appearances, levelName, nAlbumNum));
}

// NTSC-U/C: 0x001f3598, PAL: 0x001f9f78
void MemcardManager::CreateListRemixesTask(int nPortSlot, std::vector<MetRemixRecord> *pRecords) {
    mTasks.push_back(new ListRemixesMCT(mUser, mCard, nPortSlot, ++mTicket, pRecords));
}

// NTSC-U/C: 0x001f36c8, PAL: 0x001fa0a8
void MemcardManager::CreateLoadRemixTask(int nPortSlot, const HxStr &remixName) {
    mTasks.push_back(new LoadRemixMCT(mUser, mCard, nPortSlot, ++mTicket, remixName));
}

// NTSC-U/C: 0x001f3b80, PAL: 0x001fa568
void MemcardManager::CreateDeleteRemixTask(int nPortSlot, const HxStr &remixName) {
    mTasks.push_back(new DeleteRemixMCT(mUser, mCard, nPortSlot, ++mTicket, remixName));
}
