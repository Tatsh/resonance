#include "game/axefx.h"

#include "app/globals.h"
#include "game/trackdata.h"
#include "msg/allnotesoffmsg.h"
#include "msg/axisfxmsg.h"
#include "msg/multimusemsg.h"
#include "msg/stdmidimsg.h"
#include "sch/tickclock.h"
#include "script/configquery.h"

namespace {

// The configuration code the controller number is read from.
constexpr int kControllerConfigCode = 0x393;

// The controller value the constructor assumes, the middle of the range.
constexpr int kValueCenter = 64;

// The filter's step, 100 milliseconds in nanoseconds.
constexpr long long kFilterInterval = 100000000;

// A control-change status before the channel, and the top of a controller value.
constexpr unsigned char kStatusControlChange = 0xb0;
constexpr int kControllerMaximum = 127;

// The scale from the filter's 0..1 output to controller units.
constexpr float kFilterScale = 128.0f;

} // namespace

// 0x0019aa20
AxeFX::AxeFX(Globals *pGlobals, const TrackData *pTrackData)
    : mChannel(pTrackData->mChannel), mFilter(pGlobals->GetSongClock(), this), mPlaying(0),
      mValue(kValueCenter), mClock(pGlobals->GetSongClock()) {
    mController = static_cast<unsigned char>(QueryConfigValue(kControllerConfigCode));
    mFilter.mInterval.mValue = kFilterInterval;
}

// 0x0019abd8
void AxeFX::SendController() {
    StdMidiMsg msg(mClock->SongTick(),
                   static_cast<unsigned char>(kStatusControlChange | mChannel),
                   mController,
                   static_cast<unsigned char>(mValue));
    Send(&msg);
}

// 0x0019b498
void AxeFX::OnAxisFX(AxisFXMsg *pMsg) {
    mFilter.SetTarget(pMsg->mValue);
}

// 0x0019b4b8
void AxeFX::OnMultiMuse() {
    mPlaying = 1;
    SendController();
}

// 0x0019b4e0
void AxeFX::OnFilterValue(float flValue) {
    const int nValue = kControllerMaximum - static_cast<int>(flValue * kFilterScale);
    if (nValue == mValue) {
        return;
    }
    mValue = nValue;
    if (mPlaying != 0) {
        SendController();
    }
}

// 0x0019b538
void AxeFX::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nAxisFXMsgType) {
        OnAxisFX(static_cast<AxisFXMsg *>(pMsg));
    } else if (nType == static_cast<int>(g_dwMultiMuseMsgType)) {
        OnMultiMuse();
    } else if (nType == static_cast<int>(g_dwAllNotesOffMsgType)) {
        mPlaying = 0;
    }
}
