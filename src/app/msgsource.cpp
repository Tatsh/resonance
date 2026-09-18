#include "app/msgsource.h"

// 0x0054a168
MsgSource::~MsgSource() {
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
