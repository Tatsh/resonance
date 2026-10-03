#include "game/jameffectsmgr.h"

#include <algorithm>
#include <bitset>
#include <vector>

#include "app/application.h"
#include "game/grooveworld.h"
#include "gs/phrasemgr.h"
#include "msg/invalidatetrackmsg.h"
#include "msg/remixfxmsg.h"
#include "script/configquery.h"

namespace {

// The configuration code that lists the effect types a jam track builds.
constexpr int kEffectorTypesConfigCode = 905;

// The width of a step mask, one machine word.
constexpr int kStepMaskBits = 64;

// The bar range an effect toggle invalidates, the whole song.
constexpr int kSongFirstBar = 0;
constexpr int kSongEndBar = 100000;

// Effect types from kFirstStatsEffect up to kEndStatsEffect mark the world's statistics.
constexpr int kFirstStatsEffect = 5;
constexpr int kEndStatsEffect = 11;

// Deleter the destructor runs over mEffectors.
// NTSC-U/C: 0x001a6050, PAL: 0x001abdb8
void DeleteEffector(Effector *pEffector) {
    delete pEffector;
}

} // namespace

// NTSC-U/C: 0x001a5020, PAL: 0x001aad88
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

// NTSC-U/C: 0x001a5378, PAL: 0x001ab0e0
JamEffectsMgr::~JamEffectsMgr() {
    std::for_each(mEffectors.begin(), mEffectors.end(), DeleteEffector);
}

// NTSC-U/C: 0x001a56d8, PAL: 0x001ab440
void JamEffectsMgr::ApplyStepMask(long long nMask) {
    std::bitset<kStepMaskBits> mask(nMask);
    for (std::vector<Effector *>::iterator it = mEffectors.begin(); it != mEffectors.end(); ++it) {
        Effector *pEffector = *it;
        pEffector->Enable(mask[pEffector->Type()]);
    }
}

// NTSC-U/C: 0x001a54d8, PAL: 0x001ab240
void JamEffectsMgr::PostRemixFxMsg(JamEffectMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }

    const int nEffect = pMsg->mEffect;
    const int nBar = pMsg->mBar;
    // The binary does not test the effector for null.
    Effector *pEffector = FindEffector(nEffect);
    long long *pStep = mPhraseMgr->GetStepValue(nBar);
    // The image flips and tests the bit through a std::bitset reference on the step word.
    const long long nBit = 1LL << (nEffect & (kStepMaskBits - 1));
    *pStep ^= nBit;
    pEffector->Enable((*pStep & nBit) != 0);

    InvalidateTrackMsg invalidate(kSongFirstBar, kSongEndBar, mTrack);
    mPhraseMgr->Handle(&invalidate);

    RemixFXMsg remix(mTrack, nBar, nEffect, (*pStep & nBit) != 0, pMsg->mPlayer);
    Send(&remix);

    if (nEffect < kEndStatsEffect && nEffect >= kFirstStatsEffect) {
        Application::shared()->GetWorld()->MarkStatsFlag();
    }
}

// NTSC-U/C: 0x001a62d8, PAL: 0x001ac040
Effector *JamEffectsMgr::FindEffector(int nType) {
    for (std::vector<Effector *>::iterator it = mEffectors.begin(); it != mEffectors.end(); ++it) {
        if ((*it)->Type() == nType) {
            return *it;
        }
    }
    return nullptr;
}

// NTSC-U/C: 0x001a6350, PAL: 0x001ac0b8
void JamEffectsMgr::EnableAll(int bEnabled) {
    for (std::vector<Effector *>::iterator it = mEffectors.begin(); it != mEffectors.end(); ++it) {
        (*it)->Enable(bEnabled);
    }
}

// NTSC-U/C: 0x001a63d0, PAL: 0x001ac138
void JamEffectsMgr::HandleMessage(Message *pMsg) {
    if (pMsg->Type() == g_nJamEffectMsgType) {
        PostRemixFxMsg(static_cast<JamEffectMsg *>(pMsg));
    }
}
