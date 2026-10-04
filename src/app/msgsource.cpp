#include "app/msgsource.h"

#include "app/msgsink.h"
#include "msg/message.h"

namespace {

// Nesting depth of Send(). No instruction outside Send() touches the word. Nothing therefore acts
// on the depth, and the counter survives only as a debugging aid.
// NTSC-U/C: 0x00723208, PAL: 0x00766df8
int gMsgIndentLevel;

} // namespace

MsgSource::~MsgSource() {
}

void MsgSource::ClearSinks() {
    mSinks.clear();
}

void MsgSource::AddSink(MsgSink *pSink) {
    for (std::vector<MsgSink *>::iterator it = mSinks.begin(); it != mSinks.end(); ++it) {
        if (*it == pSink) {
            return;
        }
    }
    mSinks.push_back(pSink);
}

void MsgSource::RemoveSink(MsgSink *pSink) {
    for (std::vector<MsgSink *>::iterator it = mSinks.begin(); it != mSinks.end(); ++it) {
        if (*it == pSink) {
            mSinks.erase(it);
            return;
        }
    }
}

void MsgSource::Send(Message *pMsg) const {
    // The increment is compiled into the branch delay slot of the empty-vector test. It therefore
    // runs whether or not the loop below is entered.
    ++gMsgIndentLevel;
    for (std::vector<MsgSink *>::const_iterator it = mSinks.begin(); it != mSinks.end(); ++it) {
        (*it)->Dispatch(pMsg);
    }
    --gMsgIndentLevel;
}
