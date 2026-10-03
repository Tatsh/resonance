#include "app/msgsource.h"

#include "app/msgsink.h"
#include "msg/message.h"

namespace {

// Nesting depth of Send(). No instruction outside Send() touches the word. Nothing therefore acts
// on the depth, and the counter survives only as a debugging aid.
// NTSC-U/C: 0x00723208, PAL: 0x00766df8
int g_nMsgSendDepth;

} // namespace

// NTSC-U/C: 0x0054a168, PAL: 0x0058a698
MsgSource::~MsgSource() {
}

// NTSC-U/C: 0x0054a218, PAL: 0x0058a748
void MsgSource::ClearSinks() {
    mSinks.clear();
}

// NTSC-U/C: 0x0054a270, PAL: 0x0058a7a0
void MsgSource::AddSink(MsgSink *pSink) {
    for (std::vector<MsgSink *>::iterator it = mSinks.begin(); it != mSinks.end(); ++it) {
        if (*it == pSink) {
            return;
        }
    }
    mSinks.push_back(pSink);
}

// NTSC-U/C: 0x0054a2f0, PAL: 0x0058a820
void MsgSource::RemoveSink(MsgSink *pSink) {
    for (std::vector<MsgSink *>::iterator it = mSinks.begin(); it != mSinks.end(); ++it) {
        if (*it == pSink) {
            mSinks.erase(it);
            return;
        }
    }
}

// NTSC-U/C: 0x0054a370, PAL: 0x0058a8a0
void MsgSource::Send(Message *pMsg) {
    // The increment is compiled into the branch delay slot of the empty-vector test. It therefore
    // runs whether or not the loop below is entered.
    ++g_nMsgSendDepth;
    for (std::vector<MsgSink *>::iterator it = mSinks.begin(); it != mSinks.end(); ++it) {
        (*it)->Handle(pMsg);
    }
    --g_nMsgSendDepth;
}
