#include "game/phraseneutralizer.h"

// 0x001c0918
PhraseNeutralizer::PhraseNeutralizer(const TrackData *pTrackData, PhraseMgr *pPhraseMgr)
    : mTrack(pTrackData->mUnknown04), mPhraseMgr(pPhraseMgr), mTrackData(pTrackData) {
}

// 0x001c1700
void PhraseNeutralizer::HandleMessage(Message *pMsg) {
    if (pMsg->Type() == g_nNeutralizeMsgType) {
        PostTrackNeutralizedMsg(static_cast<NeutralizeMsg *>(pMsg));
    }
}
