#include "game/axenewgemmaker.h"

#include <algorithm>

#include "game/axeoldgemmaker.h"
#include "game/nullplayer.h"
#include "mid/mbt.h"
#include "msg/axisfxmsg.h"
#include "msg/axisregistermsg.h"
#include "msg/durgemmsg.h"
#include "msg/stdmidimsg.h"
#include "msg/susgemmsg.h"
#include "msg/trackselectmsg.h"

namespace {

// The axis value the constructor assumes, the middle of the range.
constexpr float kAxisCenter = 0.5f;

constexpr unsigned char kStatusKindMask = 0xf0;
constexpr unsigned char kStatusNoteOn = 0x90;
constexpr unsigned char kStatusControlChange = 0xb0;
constexpr unsigned char kSustainController = 46;

// A note-on draws a gem this many ticks long.
constexpr int kNoteGemTicks = 80;

// The word DurGemMsg's +0x18 carries for a live note's gem.
constexpr int kDurGemUnknown18 = 1;

// A sustain strip is drawn at the middle blend.
constexpr float kSustainBlend = 0.5f;

// SusGemMsg::mStop for an opening strip and for a closing one.
constexpr int kStripOpen = 0;
constexpr int kStripClose = 2;

// Saturates a tick to the finite range, as the inline Mid::MBT arithmetic does.
inline int ClampTick(int nTick) {
    return std::min(std::max(nTick, kMBTMinimum), kMBTMaximum);
}

} // namespace

// 0x001a2da0
AxeNewGemMaker::AxeNewGemMaker(const TrackData *pTrackData)
    : mTrack(pTrackData->mUnknown04), mTrackData(pTrackData), mStripId(0), mValue(kAxisCenter),
      mPlayer(&g_nullPlayer) {
}

// 0x001a2f18
void AxeNewGemMaker::PostGemMessages(StdMidiMsg *pMsg) {
    const int nTick = pMsg->mTick;
    const unsigned char nKind = pMsg->mUnknown08 & kStatusKindMask;

    if (nKind == kStatusNoteOn) {
        const float flBlend = AxeOldGemMaker::BlendForAxis(mValue);
        DurGemMsg gem;
        gem.mLane = mTrack;
        gem.mStartFrame = nTick;
        gem.mStartBlend = flBlend;
        gem.mEndFrame = Mid::MBT(ClampTick(nTick + Mid::MBT(kNoteGemTicks).mTick)).mTick;
        gem.mEndBlend = flBlend;
        gem.mUnknown18 = kDurGemUnknown18;
        gem.mPlayer = mPlayer;
        Send(&gem);
        return;
    }
    if (nKind != kStatusControlChange || pMsg->mUnknown09 != kSustainController) {
        return;
    }

    if (pMsg->mUnknown0a == 0 && mStripId == 0) {
        SusGemMsg open;
        open.mStripId = AxeOldGemMaker::NextStripId();
        open.mStop = kStripOpen;
        open.mLane = mTrack;
        open.mFrame = nTick;
        open.mBlend = kSustainBlend;
        open.mPlayer = mPlayer;
        mStripId = open.mStripId;
        Send(&open);
    }
    if (pMsg->mUnknown0a == 0 || mStripId == 0) {
        return;
    }

    SusGemMsg close;
    close.mStripId = mStripId;
    close.mStop = kStripClose;
    close.mLane = mTrack;
    close.mFrame = nTick;
    close.mBlend = kSustainBlend;
    close.mPlayer = mPlayer;
    Send(&close);
    mStripId = 0;
}

// 0x001a46e0
void AxeNewGemMaker::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        TrackSelectMsg *pSelect = static_cast<TrackSelectMsg *>(pMsg);
        if (pSelect->mUnknown04 == mTrack && pSelect->mUnknown08 == 0) {
            mPlayer = pSelect->mUnknown10;
        }
    } else if (nType == g_nAxisRegisterMsgType) {
        AxisRegisterMsg *pAxis = static_cast<AxisRegisterMsg *>(pMsg);
        if (pAxis->mPlayer == mPlayer) {
            mValue = pAxis->mValue;
        }
    } else if (nType == g_nAxisFXMsgType) {
        return;
    } else if (nType == static_cast<int>(g_dwStdMidiMsgType)) {
        PostGemMessages(static_cast<StdMidiMsg *>(pMsg));
    }
}
