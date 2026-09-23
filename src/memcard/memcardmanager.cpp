#include "memcard/memcardmanager.h"

#include "memcard/memcardps2.h"
#include "memcard/memcardtask.h"

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
