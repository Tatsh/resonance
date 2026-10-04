#include "app/task.h"

#include "os/log.h"

// NTSC-U/C: 0x006fba4c, PAL: 0x0073f4c4
Task::Node *g_pRunningTasks;

Task::Task() {
    mNode = new Node;
    mNode->mTask = this;
    mNode->mState = kTaskStateIdle;
    mNode->mNext = mNode;
    mNode->mPrev = mNode;
}

Task::Task(const Task &other) {
    (void)other; // Yes, the binary reads nothing from the source task.
    mNode = new Node;
    mNode->mTask = this;
    mNode->mState = kTaskStateIdle;
    mNode->mNext = mNode;
    mNode->mPrev = mNode;
}

Task::~Task() {
    if (mNode->mState == kTaskStateRunning) {
        Warn("hx: destroying active task");
        Unlink(mNode);
        Release();
    }
    delete mNode;
}

Task &Task::operator=(const Task &other) {
    if (this != &other) {
        Reset();
    }
    return *this;
}

void Task::Link() {
    Node *pHead = g_pRunningTasks;
    if (pHead == nullptr) {
        g_pRunningTasks = mNode;
        return;
    }
    Node *pTail = pHead->mPrev;
    mNode->mNext = pHead;
    pHead->mPrev = mNode;
    mNode->mPrev = pTail;
    pTail->mNext = mNode;
}

void Task::Unlink(Node *pNode) {
    Node *pNext = pNode->mNext;
    if (pNext == pNode) {
        g_pRunningTasks = nullptr;
        return;
    }
    Node *pHead = g_pRunningTasks;
    Node *pPrev = pNode->mPrev;
    pNext->mPrev = pPrev;
    pNode->mNext = pNode;
    pPrev->mNext = pNext;
    pNode->mPrev = pNode;
    if (pHead == pNode) {
        g_pRunningTasks = pNext;
    }
}

int Task::Start(int bBlocking) {
    int nState = mNode->mState;
    if (nState == kTaskStateRunning && bBlocking == 0) {
        return 1;
    }
    if (nState == kTaskStateFinished) {
        return 0;
    }
    if (nState != kTaskStateRunning && CheckStateChange(nState, kTaskStateRunning) == 0) {
        return 0;
    }
    if (bBlocking == 1) {
        mNode->mState = kTaskStateRunning;
        while (Poll() != 0) {
        }
        CheckStateChange(kTaskStateRunning, kTaskStateFinished);
        mNode->mState = kTaskStateFinished;
    } else {
        Link();
        mNode->mState = kTaskStateRunning;
        ++mRefs;
    }
    return 1;
}

int Task::Suspend() {
    int nState = mNode->mState;
    if (nState == kTaskStateSuspended) {
        return 1;
    }
    if (nState != kTaskStateRunning) {
        return 0;
    }
    if (CheckStateChange(kTaskStateRunning, kTaskStateSuspended) == 0) {
        return 0;
    }
    Unlink(mNode);
    mNode->mState = kTaskStateSuspended;
    Release();
    return 1;
}

int Task::Reset() {
    int nState = mNode->mState;
    if (nState == kTaskStateIdle) {
        return 1;
    }
    if (CheckStateChange(nState, kTaskStateIdle) == 0) {
        return 0;
    }
    if (nState == kTaskStateRunning) {
        Unlink(mNode);
        Release();
    }
    mNode->mState = kTaskStateIdle;
    return 1;
}

int Task::Finish() {
    int nState = mNode->mState;
    if (nState == kTaskStateFinished) {
        return 1;
    }
    if (CheckStateChange(nState, kTaskStateFinished) == 0) {
        return 0;
    }
    if (nState == kTaskStateRunning) {
        Unlink(mNode);
        Release();
    }
    mNode->mState = kTaskStateFinished;
    return 1;
}

int Task::State() const {
    return mNode->mState;
}

int Task::InProgress() const {
    return static_cast<unsigned>(State() - kTaskStateRunning) < 2;
}

float Task::GetProgress() {
    return Progress();
}

HxStr Task::GetStatus() const {
    return Name();
}

int Task::GetQuiet() {
    return Quiet();
}

int Task::PollTasks() {
    Node *pNode = g_pRunningTasks;
    if (pNode == nullptr) {
        return 0;
    }
    Task *pTask = pNode->mTask;
    g_pRunningTasks = pNode->mNext;
    if (pTask->Poll() != 0) {
        if (pTask->Quiet() == 0) {
            pTask->Progress(); // Yes, the binary discards this call's result.
        }
        return 1;
    }
    pTask->CheckStateChange(kTaskStateRunning, kTaskStateFinished);
    Unlink(pTask->mNode);
    pTask->mNode->mState = kTaskStateFinished;
    pTask->Release();
    return g_pRunningTasks != nullptr;
}

int Task::Quiet() {
    return 0;
}

int Task::CheckStateChange(int nFrom, int nTo) {
    (void)nFrom;
    (void)nTo;
    return 1;
}
