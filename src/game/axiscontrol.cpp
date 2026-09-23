#include "game/axiscontrol.h"

#include <cstdlib>

#include "game/nullplayer.h"
#include "msg/allnotesoffmsg.h"
#include "msg/axisfxmsg.h"
#include "msg/stdmidimsg.h"
#include "msg/sustainnotemsg.h"

namespace {

// The coarse stick position the constructor assumes, the middle of 0..127.
constexpr int kLaneCenter = 64;

// The stick position the constructor assumes before the first reading.
constexpr int kNoAxis = -1;

// The centre of the 0..1023 stick range, and how close to it a bend snaps to it.
constexpr int kAxisCenter = 512;
constexpr int kAxisSnapRange = 50;

// A pitch-bend status before the channel, and the divisor from stick units to its coarse byte.
constexpr unsigned char kStatusPitchBend = 0xe0;
constexpr int kBendScale = 8;
constexpr int kDataByteMask = 0x7f;

// The high nibble of a channel message's status, and the note-on kind.
constexpr unsigned char kStatusKindMask = 0xf0;
constexpr unsigned char kStatusNoteOn = 0x90;

// The bend value that returns the pitch to the centre.
constexpr int kNoBend = 0;

} // namespace

// 0x0019e940
AxisControl::AxisControl(const TrackData *pTrackData)
    : mTrack(pTrackData->mUnknown04), mChannel(pTrackData->mChannel), mLane(kLaneCenter),
      mAxis(kNoAxis), mBending(0), mBendOrigin(0), mSustainTick(0), mPlayer(&g_nullPlayer) {
}

// 0x0019ecf0
void AxisControl::SendPitchBend(int nTick, int nValue) {
    const unsigned char nCoarse =
        static_cast<unsigned char>(((nValue + kAxisCenter) / kBendScale) & kDataByteMask);
    StdMidiMsg msg(nTick, static_cast<unsigned char>(kStatusPitchBend | mChannel), 0, nCoarse);
    Send(&msg);
}

// 0x0019ed80
void AxisControl::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nAxisRegisterMsgType) {
        OnAxisRegister(static_cast<AxisRegisterMsg *>(pMsg));
        return;
    }
    if (nType == g_nAxisFXMsgType) {
        return;
    }
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(static_cast<TrackSelectMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(g_dwSustainNoteMsgType)) {
        // The tick is stored without the finiteness check.
        mSustainTick.mTick = static_cast<SustainNoteMsg *>(pMsg)->mTick;
        return;
    }

    if (nType == static_cast<int>(g_dwStdMidiMsgType)) {
        OnStdMidi(static_cast<StdMidiMsg *>(pMsg));
    } else if (nType == static_cast<int>(g_dwAllNotesOffMsgType)) {
        OnAllNotesOff(static_cast<AllNotesOffMsg *>(pMsg));
    }
}

// 0x0019fab0
void AxisControl::OnStdMidi(StdMidiMsg *pMsg) {
    if ((pMsg->mUnknown08 & kStatusKindMask) != kStatusNoteOn) {
        return;
    }
    if (mSustainTick.mTick == pMsg->mTick) {
        if (mBending != 0) {
            return;
        }
        mBendOrigin = mAxis;
        if (std::abs(mAxis - kAxisCenter) < kAxisSnapRange) {
            mBendOrigin = kAxisCenter;
        }
        mBending = 1;
        return;
    }
    if (mBending == 0) {
        return;
    }
    SendPitchBend(pMsg->mTick, kNoBend);
    mBending = 0;
}

// 0x0019fb40
void AxisControl::OnAllNotesOff(AllNotesOffMsg *pMsg) {
    if (mBending == 0) {
        return;
    }
    SendPitchBend(pMsg->mTick, kNoBend);
    mBending = 0;
}
