#include "app/msgqueue.h"

namespace {

// Delete every stored message and empty the vector. The destructor inlines a copy of this for
// each of its two vectors.
inline void DeleteStoredMessages(std::vector<Message *> &messages) {
    for (std::vector<Message *>::iterator it = messages.begin(); it != messages.end(); ++it) {
        delete *it;
    }
    messages.clear();
}

} // namespace

// 0x0054a738
MsgQueue::MsgQueue() {
    mTarget = &mFirst;
    mInPoll = 0;
}

// 0x0054a7a0
MsgQueue::~MsgQueue() {
    DeleteStoredMessages(mFirst);
    DeleteStoredMessages(mSecond);
}

// 0x0054b220
void MsgQueue::Store(Message *pMsg) {
    mTarget->push_back(pMsg->Clone());
}

// 0x0054b290
void MsgQueue::HandleMessage(Message *pMsg) {
    pMsg->Type(); // Yes, the binary discards this call's result.
    mTarget->push_back(pMsg->Clone());
}

// 0x0054aa58
void MsgQueue::Poll() {
    mInPoll = 1;
    mDraining = mTarget;
    mTarget = (mTarget == &mFirst) ? &mSecond : &mFirst;
    for (std::vector<Message *>::iterator it = mDraining->begin(); it != mDraining->end(); ++it) {
        Send(*it);
        delete *it;
    }
    mDraining->clear();
    mInPoll = 0;
}
