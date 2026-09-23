#include "game/axeoldgemmaker.h"

#include <algorithm>

#include "game/phrase.h"
#include "gs/multimuse.h"
#include "msg/durgemmsg.h"
#include "msg/notemsg.h"
#include "msg/phrasemsg.h"
#include "msg/stdmidimsg.h"

namespace {

// The seven blends BlendForStep() reports, for steps -3 through 3.
constexpr int kLowestStep = -3;
constexpr float kStepBlends[] = {0.7f, 0.8f, 0.9f, 0.5f, 0.3f, 0.2f, 0.1f};
constexpr int kStepCount = sizeof(kStepBlends) / sizeof(kStepBlends[0]);

constexpr int kBarTicks = 1920;

// A note's gem is this many ticks shorter than the note, and never shorter than this.
constexpr int kGemTrimTicks = 60;

constexpr unsigned char kStatusKindMask = 0xf0;
constexpr unsigned char kStatusControlChange = 0xb0;
constexpr unsigned char kSustainController = 46;

// A sustain gem is drawn at the middle blend from end to end.
constexpr float kSustainBlend = 0.5f;

// The word DurGemMsg's +0x18 carries for every gem this maker sends.
constexpr int kDurGemUnknown18 = 0;

// Saturates a tick to the finite range, as the inline Mid::MBT arithmetic does.
inline int ClampTick(int nTick) {
    return std::min(std::max(nTick, kMBTMinimum), kMBTMaximum);
}

} // namespace

int g_nNextStripId;

// 0x001a31c8
AxeOldGemMaker::AxeOldGemMaker(const TrackData *pTrackData)
    : mTrack(pTrackData->mUnknown04), mPhrase(nullptr), mSustainStart(0) {
}

// 0x001a32f0
void AxeOldGemMaker::PostDurGemMsg(NoteMsg *pMsg) {
    const Mid::MBT offset(mPosition.mTick % Mid::MBT(kBarTicks).mTick);
    const float flBlend = BlendForAxis(mPhrase->GetValue(offset.mTick));

    Mid::MBT length(ClampTick(pMsg->mLength.mTick - Mid::MBT(kGemTrimTicks).mTick));
    if (length.mTick < Mid::MBT(kGemTrimTicks).mTick) {
        length = Mid::MBT(kGemTrimTicks);
    }

    DurGemMsg gem;
    gem.mLane = mTrack;
    gem.mStartFrame = mPosition.mTick;
    gem.mStartBlend = flBlend;
    gem.mEndFrame = Mid::MBT(ClampTick(mPosition.mTick + length.mTick)).mTick;
    gem.mEndBlend = flBlend;
    gem.mUnknown18 = kDurGemUnknown18;
    gem.mPlayer = mPhrase->mPlayer;
    Send(&gem);
}

// 0x001a34e8
void AxeOldGemMaker::OnStdMidi(StdMidiMsg *pMsg) {
    if ((pMsg->mUnknown08 & kStatusKindMask) != kStatusControlChange ||
        pMsg->mUnknown09 != kSustainController) {
        return;
    }

    if (pMsg->mUnknown0a == 0 && mSustainStart.mTick == Mid::MBT(0).mTick) {
        mSustainStart = mPosition;
    }
    if (pMsg->mUnknown0a == 0 || mSustainStart.mTick == Mid::MBT(0).mTick) {
        return;
    }

    DurGemMsg gem;
    gem.mLane = mTrack;
    gem.mStartFrame = mSustainStart.mTick;
    gem.mStartBlend = kSustainBlend;
    gem.mEndFrame = mPosition.mTick;
    gem.mEndBlend = kSustainBlend;
    gem.mUnknown18 = kDurGemUnknown18;
    gem.mPlayer = mPhrase->mPlayer;
    Send(&gem);
    mSustainStart = Mid::MBT(0);
}

// 0x001a3600
void AxeOldGemMaker::OnPhrase(PhraseMsg *pMsg) {
    mPhrase = pMsg->mPhrase;
    mPosition = Mid::MBT(ClampTick(pMsg->mBar * Mid::MBT(kBarTicks).mTick));

    MultiMuse *pMuse = mPhrase->mMuse;
    if (pMuse != nullptr) {
        for (const auto &entry : pMuse->mEntries) {
            // The replay steps the position without the finiteness check.
            mPosition.mTick = ClampTick(mPosition.mTick + entry.mPosition.mTick);
            Handle(entry.mValue);
            mPosition.mTick = ClampTick(mPosition.mTick - entry.mPosition.mTick);
        }
    }
    mPhrase = nullptr;
}

// 0x001a4578
float AxeOldGemMaker::BlendForStep(int nStep) {
    const int nIndex = nStep - kLowestStep;
    if (nIndex < 0 || nIndex >= kStepCount) {
        return 0.0f;
    }
    return kStepBlends[nIndex];
}

// 0x001a47a8
void AxeOldGemMaker::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nPhraseMsgType) {
        OnPhrase(static_cast<PhraseMsg *>(pMsg));
    } else if (nType == static_cast<int>(g_dwNoteMsgType)) {
        PostDurGemMsg(static_cast<NoteMsg *>(pMsg));
    } else if (nType == static_cast<int>(g_dwStdMidiMsgType)) {
        OnStdMidi(static_cast<StdMidiMsg *>(pMsg));
    }
}
