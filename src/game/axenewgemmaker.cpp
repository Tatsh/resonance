#include "game/axenewgemmaker.h"

#include <algorithm>

#include "game/axeoldgemmaker.h"
#include "game/nullplayer.h"
#include "mid/tick.h"
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

// The value DurGemMsg::mLive receives for a live note's gem. AxeOldGemMaker's gems receive zero.
constexpr int kLiveNoteGem = 1;

// A sustain strip is drawn at the middle blend.
constexpr float kSustainBlend = 0.5f;

// SusGemMsg::mStop for an opening strip and for a closing one.
constexpr int kStripOpen = 0;
constexpr int kStripClose = 2;

// Saturates a tick to the finite range, as the inline Sch::Tick arithmetic does.
inline int ClampTick(int nTick) {
    return std::min(std::max(nTick, kTickMinimum), kTickMaximum);
}

} // namespace

AxeNewGemMaker::AxeNewGemMaker(const TrackData *pTrackData)
    : mTrack(pTrackData->mIndex), mTrackData(pTrackData), mStripId(0), mValue(kAxisCenter),
      mPlayer(&NullPlayer::sInstance) {
}

void AxeNewGemMaker::PostGemMessages(StdMidiMsg *pMsg) {
    const int nTick = pMsg->mTick;
    const unsigned char nKind = pMsg->mStatus & kStatusKindMask;

    if (nKind == kStatusNoteOn) {
        const float flBlend = AxeOldGemMaker::BlendForAxis(mValue);
        DurGemMsg gem;
        gem.mLane = mTrack;
        gem.mStartFrame = nTick;
        gem.mStartBlend = flBlend;
        gem.mEndFrame = Sch::Tick(ClampTick(nTick + Sch::Tick(kNoteGemTicks).mTick)).mTick;
        gem.mEndBlend = flBlend;
        gem.mLive = kLiveNoteGem;
        gem.mPlayer = mPlayer;
        Send(&gem);
        return;
    }
    if (nKind != kStatusControlChange || pMsg->mData1 != kSustainController) {
        return;
    }

    if (pMsg->mData2 == 0 && mStripId == 0) {
        SusGemMsg open;
        open.mStripId = GetNewGemID();
        open.mStop = kStripOpen;
        open.mLane = mTrack;
        open.mFrame = nTick;
        open.mBlend = kSustainBlend;
        open.mPlayer = mPlayer;
        mStripId = open.mStripId;
        Send(&open);
    }
    if (pMsg->mData2 == 0 || mStripId == 0) {
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

void AxeNewGemMaker::DispatchPriv(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        TrackSelectMsg *pSelect = static_cast<TrackSelectMsg *>(pMsg);
        if (pSelect->mTrack == mTrack && pSelect->mPlace == 0) {
            mPlayer = pSelect->mPlayer;
        }
    } else if (nType == g_nAxisRegisterMsgType) {
        AxisRegisterMsg *pAxis = static_cast<AxisRegisterMsg *>(pMsg);
        if (pAxis->mPlayer == mPlayer) {
            mValue = pAxis->mValue;
        }
    } else if (nType == g_nAxisFXMsgType) {
        return;
    } else if (nType == static_cast<int>(StdMidiMsg::sID)) {
        PostGemMessages(static_cast<StdMidiMsg *>(pMsg));
    }
}
