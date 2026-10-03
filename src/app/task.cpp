#include "app/task.h"

#include "os/log.h"

// NTSC-U/C: 0x006fba4c, PAL: 0x0073f4c4
Task::Node *g_pRunningTasks;

// NTSC-U/C: 0x004b6270, PAL: 0x004f4580
Task::Task() {
    mNode = new Node;
    mNode->mTask = this;
    mNode->mState = kTaskStateIdle;
    mNode->mNext = mNode;
    mNode->mPrev = mNode;
}

// NTSC-U/C: 0x004b6708, PAL: 0x004f4a18
Task::Task(const Task &other) {
    (void)other; // Yes, the binary reads nothing from the source task.
    mNode = new Node;
    mNode->mTask = this;
    mNode->mState = kTaskStateIdle;
    mNode->mNext = mNode;
    mNode->mPrev = mNode;
}

// NTSC-U/C: 0x004b6808, PAL: 0x004f4b18
Task::~Task() {
    if (mNode->mState == kTaskStateRunning) {
        Warn("hx: destroying active task");
        Unlink(mNode);
        Release();
    }
    delete mNode;
}

// NTSC-U/C: 0x004b6928, PAL: 0x004f4c38
Task &Task::operator=(const Task &other) {
    if (this != &other) {
        Reset();
    }
    return *this;
}

// Link the node in ahead of the ring head, which makes it the last position of the pass.
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

// Take the node out of the ring and make it self-referential again, moving the head along when the
// head is the node being removed.
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

// NTSC-U/C: 0x004b6370, PAL: 0x004f4680
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

// NTSC-U/C: 0x004b6968, PAL: 0x004f4c78
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

// NTSC-U/C: 0x004b6a28, PAL: 0x004f4d38
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

// NTSC-U/C: 0x004b6ae8, PAL: 0x004f4df8
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

// NTSC-U/C: 0x004b6958, PAL: 0x004f4c68
int Task::State() {
    return mNode->mState;
}

// NTSC-U/C: 0x004b65e8, PAL: 0x004f48f8
int Task::InProgress() {
    return static_cast<unsigned>(State() - kTaskStateRunning) < 2;
}

// NTSC-U/C: 0x004b6610, PAL: 0x004f4920
float Task::GetProgress() {
    return Progress();
}

// NTSC-U/C: 0x004b6638, PAL: 0x004f4948
HxStr Task::GetStatus() {
    return Name();
}

// NTSC-U/C: 0x004b6670, PAL: 0x004f4980
int Task::GetQuiet() {
    return Quiet();
}

// NTSC-U/C: 0x004b6490, PAL: 0x004f47a0
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

// NTSC-U/C: 0x004b6bb0, PAL: 0x004f4ec0
int Task::Quiet() {
    return 0;
}

// NTSC-U/C: 0x004b6bb8, PAL: 0x004f4ec8
int Task::CheckStateChange(int nFrom, int nTo) {
    (void)nFrom;
    (void)nTo;
    return 1;
}
