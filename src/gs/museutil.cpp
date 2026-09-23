#include "gs/museutil.h"

#include "app/msgsink.h"
#include "gs/multimuse.h"
#include "mid/mbt.h"
#include "msg/musemsg.h"
#include "msg/notemsg.h"
#include "msg/stdmidimsg.h"

namespace {

constexpr unsigned char kStatusKindMask = 0xf0;
constexpr unsigned char kStatusNoteOff = 0x80;
constexpr unsigned char kStatusNoteOn = 0x90;

// MultiMuse::Add() tries to append before it searches, because the entries arrive in order.
constexpr int kAppendFirst = 1;

// `_GLOBAL_$N$GsMuseUtil.cpp::Shifter` in the RTTI, with MsgSink as its one base. TransposeMuse()
// builds one on its stack and visits every entry of the sequence through it.
class Shifter : public MsgSink {
public:
    // Builds a new sequence holding a copy of every entry, each transposed as HandleMessage()
    // decides, and returns it with one reference.
    // 0x001ab4c8
    MultiMuse *Transpose(MultiMuse *pMuse, int nTrans) {
        mResult = new MultiMuse;
        mTrans = nTrans;
        for (const auto &entry : pMuse->mEntries) {
            mPosition = entry.mPosition;
            Handle(entry.mValue);
        }
        return mResult;
    }

    // 0x001ab970
    virtual void HandleMessage(Message *pMsg) {
        const int nType = pMsg->Type();
        if (nType == static_cast<int>(g_dwNoteMsgType)) {
            OnNote(static_cast<NoteMsg *>(pMsg));
        } else if (nType == static_cast<int>(g_dwStdMidiMsgType)) {
            OnStdMidi(static_cast<StdMidiMsg *>(pMsg));
        } else if (nType >= g_nFirstMuseMsgType && nType < g_nEndMuseMsgType) {
            AddUnchanged(static_cast<MuseMsg *>(pMsg));
        }
    }

private:
    // Adds a copy of the note with its number raised by mTrans, wrapping at 256.
    // 0x001ab588
    void OnNote(NoteMsg *pMsg) {
        NoteMsg shifted(*pMsg);
        shifted.mNote = static_cast<unsigned char>(pMsg->mNote + mTrans);
        mResult->Add(&shifted, mPosition.mTick, kAppendFirst);
    }

    // Adds a copy of the message, with the note number of a note-on or note-off raised by mTrans.
    // 0x001ab620
    void OnStdMidi(StdMidiMsg *pMsg) {
        StdMidiMsg shifted(*pMsg);
        const unsigned char nKind = pMsg->mUnknown08 & kStatusKindMask;
        if (nKind == kStatusNoteOn || nKind == kStatusNoteOff) {
            shifted.mUnknown09 = static_cast<unsigned char>(shifted.mUnknown09 + mTrans);
        }
        mResult->Add(&shifted, mPosition.mTick, kAppendFirst);
    }

    // The out-of-line copy of the branch HandleMessage() expands inline for any other message in
    // the MuseMsg identity range. The image has no caller of this copy.
    // 0x001ab948
    void AddUnchanged(MuseMsg *pMsg) {
        mResult->Add(pMsg, mPosition.mTick, kAppendFirst);
    }

    int mTrans;         // +0x04, read back as its low byte
    MultiMuse *mResult; // +0x08
    Mid::MBT mPosition; // +0x0c, the position of the entry being visited
};

} // namespace

// 0x001ab6d8
MultiMuse *TransposeMuse(MultiMuse *pMuse, int nTrans) {
    Shifter shifter;
    return shifter.Transpose(pMuse, nTrans);
}
