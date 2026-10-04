#include "app/msgqueue.h"

#include "msg/message.h"

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

MsgQueue::MsgQueue() : mTarget(&mFirst), mInPoll(0) {
}

MsgQueue::~MsgQueue() {
    DeleteStoredMessages(mFirst);
    DeleteStoredMessages(mSecond);
}

void MsgQueue::Store(Message *pMsg) {
    mTarget->push_back(pMsg->Clone());
}

void MsgQueue::DispatchPriv(Message *pMsg) {
    pMsg->Type(); // Yes, the binary discards this call's result.
    mTarget->push_back(pMsg->Clone());
}

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
