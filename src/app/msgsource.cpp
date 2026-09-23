#include "app/msgsource.h"

#include "app/msgsink.h"
#include "msg/message.h"

namespace {

// 0x00723208. Nesting depth of Send(). No instruction outside Send() touches the word. Nothing
// therefore acts on the depth, and the counter survives only as a debugging aid.
int g_nMsgSendDepth;

} // namespace

// 0x0054a168
MsgSource::~MsgSource() {
}

// 0x0054a218
void MsgSource::ClearSinks() {
    mSinks.clear();
}

// 0x0054a270
void MsgSource::AddSink(MsgSink *pSink) {
    for (std::vector<MsgSink *>::iterator it = mSinks.begin(); it != mSinks.end(); ++it) {
        if (*it == pSink) {
            return;
        }
    }
    mSinks.push_back(pSink);
}

// 0x0054a2f0
void MsgSource::RemoveSink(MsgSink *pSink) {
    for (std::vector<MsgSink *>::iterator it = mSinks.begin(); it != mSinks.end(); ++it) {
        if (*it == pSink) {
            mSinks.erase(it);
            return;
        }
    }
}

// 0x0054a370
void MsgSource::Send(Message *pMsg) {
    // The increment is compiled into the branch delay slot of the empty-vector test. It therefore
    // runs whether or not the loop below is entered.
    ++g_nMsgSendDepth;
    for (std::vector<MsgSink *>::iterator it = mSinks.begin(); it != mSinks.end(); ++it) {
        (*it)->Handle(pMsg);
    }
    --g_nMsgSendDepth;
}
