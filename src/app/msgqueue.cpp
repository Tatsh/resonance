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

// NTSC-U/C: 0x0054a738, PAL: 0x0058ac68
MsgQueue::MsgQueue() {
    mTarget = &mFirst;
    mInPoll = 0;
}

// NTSC-U/C: 0x0054a7a0, PAL: 0x0058acd0
MsgQueue::~MsgQueue() {
    DeleteStoredMessages(mFirst);
    DeleteStoredMessages(mSecond);
}

// NTSC-U/C: 0x0054b220, PAL: 0x0058b750
void MsgQueue::Store(Message *pMsg) {
    mTarget->push_back(pMsg->Clone());
}

// NTSC-U/C: 0x0054b290, PAL: 0x0058b7c0
void MsgQueue::DispatchPriv(Message *pMsg) {
    pMsg->Type(); // Yes, the binary discards this call's result.
    mTarget->push_back(pMsg->Clone());
}

// NTSC-U/C: 0x0054aa58, PAL: 0x0058af88
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
