#include "gs/museutil.h"

#include "app/msgsink.h"
#include "gs/multimuse.h"
#include "mid/tick.h"
#include "msg/musemsg.h"
#include "msg/notemsg.h"
#include "msg/stdmidimsg.h"

namespace {

constexpr unsigned char kStatusKindMask = 0xf0;
constexpr unsigned char kStatusNoteOff = 0x80;
constexpr unsigned char kStatusNoteOn = 0x90;

// MultiMuse::Add() tries to append before it searches, because the entries arrive in order.
constexpr int kAppendFirst = 1;

// `_GLOBAL_$N$GsMuseUtil.cpp::Shifter` in the RTTI, with MsgSink as its one base.
// CloneAndTranspose() builds one on its stack and visits every entry of the sequence through it.
class Shifter : public MsgSink {
public:
    // Builds a new sequence holding a copy of every entry, each transposed as DispatchPriv()
    // decides, and returns it with one reference.
    // NTSC-U/C: 0x001ab4c8, PAL: 0x001b1230
    MultiMuse *GetShiftedMuse(const MultiMuse &muse, int nTrans) {
        mResult = new MultiMuse;
        mTrans = nTrans;
        for (const auto &entry : muse.mEntries) {
            mPosition = entry.mPosition;
            Dispatch(entry.mValue);
        }
        return mResult;
    }

    // NTSC-U/C: 0x001ab970, PAL: 0x001b16d8
    virtual void DispatchPriv(Message *pMsg) {
        const int nType = pMsg->Type();
        if (nType == static_cast<int>(NoteMsg::sID)) {
            OnMsg(*static_cast<NoteMsg *>(pMsg));
        } else if (nType == static_cast<int>(StdMidiMsg::sID)) {
            OnMsg(*static_cast<StdMidiMsg *>(pMsg));
        } else if (nType >= g_nFirstMuseMsgType && nType < g_nEndMuseMsgType) {
            OnMsg(*static_cast<MuseMsg *>(pMsg));
        }
    }

private:
    // Adds a copy of the note with its number raised by mTrans, wrapping at 256.
    // NTSC-U/C: 0x001ab588, PAL: 0x001b12f0
    void OnMsg(const NoteMsg &msg) {
        NoteMsg shifted(msg);
        shifted.mNote = static_cast<unsigned char>(msg.mNote + mTrans);
        mResult->Add(&shifted, mPosition.mTick, kAppendFirst);
    }

    // Adds a copy of the message, with the note number of a note-on or note-off raised by mTrans.
    // NTSC-U/C: 0x001ab620, PAL: 0x001b1388
    void OnMsg(const StdMidiMsg &msg) {
        StdMidiMsg shifted(msg);
        const unsigned char nKind = msg.mStatus & kStatusKindMask;
        if (nKind == kStatusNoteOn || nKind == kStatusNoteOff) {
            shifted.mData1 = static_cast<unsigned char>(shifted.mData1 + mTrans);
        }
        mResult->Add(&shifted, mPosition.mTick, kAppendFirst);
    }

    // The out-of-line copy of the branch DispatchPriv() expands inline for any other message in
    // the MuseMsg identity range. The image has no caller of this copy.
    // NTSC-U/C: 0x001ab948, PAL: 0x001b16b0
    void OnMsg(MuseMsg &msg) {
        mResult->Add(&msg, mPosition.mTick, kAppendFirst);
    }

    int mTrans;          // +0x04, read back as its low byte
    MultiMuse *mResult;  // +0x08
    Sch::Tick mPosition; // +0x0c, the position of the entry being visited
};

} // namespace

MultiMuse *CloneAndTranspose(const MultiMuse &muse, int nTrans) {
    Shifter shifter;
    return shifter.GetShiftedMuse(muse, nTrans);
}
