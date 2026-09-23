#include "game/jameffectsmgr.h"

#include <algorithm>
#include <bitset>
#include <vector>

#include "script/configquery.h"

namespace {

// The configuration code that lists the effect types a jam track builds.
constexpr int kEffectorTypesConfigCode = 0x389;

// The width of a step mask, one machine word.
constexpr int kStepMaskBits = 64;

// Deleter the destructor runs over mEffectors.
// 0x001a6050
void DeleteEffector(Effector *pEffector) {
    delete pEffector;
}

} // namespace

// 0x001a5020
JamEffectsMgr::JamEffectsMgr(
    int nTrack, unsigned char nChannel, PlayMap *pPlayMap, PhraseMgr *pPhraseMgr, MsgSink *pSink)
    : mPlayMap(pPlayMap), mPhraseMgr(pPhraseMgr), mTrack(nTrack), mChannel(nChannel) {
    std::vector<int> types;
    QueryConfigVector(&types, kEffectorTypesConfigCode);
    for (std::vector<int>::iterator it = types.begin(); it != types.end(); ++it) {
        Effector *pEffector = Effector::CreateForType(*it, mChannel, mTrack);
        pEffector->AddSink(pSink);
        mEffectors.push_back(pEffector);
    }
}

// 0x001a5378
JamEffectsMgr::~JamEffectsMgr() {
    std::for_each(mEffectors.begin(), mEffectors.end(), DeleteEffector);
}

// 0x001a56d8
void JamEffectsMgr::ApplyStepMask(long nMask) {
    std::bitset<kStepMaskBits> mask(nMask);
    for (std::vector<Effector *>::iterator it = mEffectors.begin(); it != mEffectors.end(); ++it) {
        Effector *pEffector = *it;
        pEffector->Enable(mask[pEffector->Type()]);
    }
}

// 0x001a62d8
Effector *JamEffectsMgr::FindEffector(int nType) {
    for (std::vector<Effector *>::iterator it = mEffectors.begin(); it != mEffectors.end(); ++it) {
        if ((*it)->Type() == nType) {
            return *it;
        }
    }
    return nullptr;
}

// 0x001a6350
void JamEffectsMgr::EnableAll(int bEnabled) {
    for (std::vector<Effector *>::iterator it = mEffectors.begin(); it != mEffectors.end(); ++it) {
        (*it)->Enable(bEnabled);
    }
}

// 0x001a63d0
void JamEffectsMgr::HandleMessage(Message *pMsg) {
    if (pMsg->Type() == g_nJamEffectMsgType) {
        PostRemixFxMsg(static_cast<JamEffectMsg *>(pMsg));
    }
}
