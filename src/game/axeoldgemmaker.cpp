#include "game/axeoldgemmaker.h"

#include "msg/notemsg.h"
#include "msg/phrasemsg.h"
#include "msg/stdmidimsg.h"

namespace {

// The seven blends BlendForStep() reports, for steps -3 through 3.
constexpr int kLowestStep = -3;
constexpr float kStepBlends[] = {0.7f, 0.8f, 0.9f, 0.5f, 0.3f, 0.2f, 0.1f};
constexpr int kStepCount = sizeof(kStepBlends) / sizeof(kStepBlends[0]);

} // namespace

int g_nNextStripId;

// 0x001a31c8
AxeOldGemMaker::AxeOldGemMaker(const TrackData *pTrackData)
    : mTrack(pTrackData->mUnknown04), mPhrase(nullptr), mSustainStart(0) {
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
