#include "game/axiscontrol.h"

#include <cstdlib>

#include "game/nullplayer.h"
#include "game/player.h"
#include "msg/allnotesoffmsg.h"
#include "msg/axisfxmsg.h"
#include "msg/nowbarmsg.h"
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

// A NowBarMsg lane runs from 1 at coarse position 0 down to 0 at 128.
constexpr int kLaneCount = 128;
constexpr float kLaneScale = 1.0f / kLaneCount;

// An AxisRegisterMsg's value, between 0 and 1, scales onto the 0..1024 stick range, and the
// coarse lane is the stick position divided by this.
constexpr float kAxisScale = 1024.0f;
constexpr int kLaneDivisor = 8;

} // namespace

// NTSC-U/C: 0x0019e940, PAL: 0x001a46a8
AxisControl::AxisControl(const TrackData *pTrackData)
    : mTrack(pTrackData->mIndex), mChannel(pTrackData->mChannel), mLane(kLaneCenter),
      mAxis(kNoAxis), mBending(0), mBendOrigin(0), mSustainTick(0), mPlayer(&g_nullPlayer) {
}

// NTSC-U/C: 0x0019ea80, PAL: 0x001a47e8
void AxisControl::OnAxisRegister(AxisRegisterMsg *pMsg) {
    const int bOwnTrack = pMsg->mTrack == mTrack;
    mAxis = static_cast<int>(pMsg->mValue * kAxisScale);
    const int nLane = mAxis / kLaneDivisor;
    if (nLane != mLane) {
        mLane = nLane;
        if (bOwnTrack == 0) {
            return;
        }
        NowBarMsg nowBar;
        nowBar.mTrack = pMsg->mTrack;
        nowBar.mPlayer = pMsg->mPlayer;
        nowBar.mLane = static_cast<float>(kLaneCount - nLane) * kLaneScale;
        Send(&nowBar);
    }
    if (bOwnTrack == 0) {
        return;
    }

    if (mBending != 0 && mBendOrigin == kAxisCenter) {
        if (pMsg->mPlayer == mPlayer) {
            SendPitchBend(pMsg->mPosition.mTick, mAxis - kAxisCenter);
        }
        return;
    }
    // A bend that started off centre waits for the stick to cross the centre before it drives.
    if (mBendOrigin > kAxisCenter) {
        if (mAxis <= kAxisCenter) {
            mBendOrigin = kAxisCenter;
        }
    } else if (mBendOrigin < kAxisCenter) {
        if (mAxis >= kAxisCenter) {
            mBendOrigin = kAxisCenter;
        }
    }
}

// NTSC-U/C: 0x0019ec10, PAL: 0x001a4978
void AxisControl::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }
    mPlayer = pMsg->mPlayer;
    if (mPlayer->IsNull() != 0) {
        return;
    }
    NowBarMsg nowBar;
    nowBar.mTrack = mTrack;
    nowBar.mPlayer = pMsg->mPlayer;
    nowBar.mLane = static_cast<float>(kLaneCount - mLane) * kLaneScale;
    Send(&nowBar);
}

// NTSC-U/C: 0x0019ecf0, PAL: 0x001a4a58
void AxisControl::SendPitchBend(int nTick, int nValue) {
    const unsigned char nCoarse =
        static_cast<unsigned char>(((nValue + kAxisCenter) / kBendScale) & kDataByteMask);
    StdMidiMsg msg(nTick, static_cast<unsigned char>(kStatusPitchBend | mChannel), 0, nCoarse);
    Send(&msg);
}

// NTSC-U/C: 0x0019ed80, PAL: 0x001a4ae8
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

// NTSC-U/C: 0x0019fab0, PAL: 0x001a5818
void AxisControl::OnStdMidi(StdMidiMsg *pMsg) {
    if ((pMsg->mStatus & kStatusKindMask) != kStatusNoteOn) {
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

// NTSC-U/C: 0x0019fb40, PAL: 0x001a58a8
void AxisControl::OnAllNotesOff(AllNotesOffMsg *pMsg) {
    if (mBending == 0) {
        return;
    }
    SendPitchBend(pMsg->mTick, kNoBend);
    mBending = 0;
}
