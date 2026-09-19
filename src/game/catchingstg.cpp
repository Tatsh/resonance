#include "game/catchingstg.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/singlecatcher.h"

// 0x001a0450
CatchingSTG::~CatchingSTG() {
    CatchingSTG::Slot3(); // The binary calls this class's own body rather than dispatching.
    delete mCatcher;
    delete mNeutralizer;
}

// 0x001a04d8
void CatchingSTG::Slot2() {
    ScoreTrackGraph::Slot2();
    mCatcher->Slot4();
}

// 0x001a0518
void CatchingSTG::Slot3() {
    mCatcher->Slot5();
    ScoreTrackGraph::Slot3();
}

// 0x0019fd88
void CatchingSTG::Slot4(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary) {
    pPrimary->AddSink(mMixer);
    pPrimary->AddSink(mCatcher);
    pPrimary->AddSink(mNeutralizer);
    pPrimary->AddSink(mPhrasePlayer);
    pPrimary->AddSink(mPhraseMgr);

    mCatcher->AddSink(mPhrasePlayer);
    mCatcher->AddSink(mMuseSynth);

    pSecondary->AddSink(mPhraseMgr);
    pSecondary->AddSink(mCatcher);

    if (pOptional != nullptr) {
        pOptional->AddSink(mPhraseMgr);
        pOptional->AddSink(mCatcher);
    }

    mPhrasePlayer->AddSink(mMuseSynth);
}

// 0x001a0558
void CatchingSTG::Slot5(MsgSource *pSource) {
    pSource->AddSink(mMixer);
}

// 0x001a0588
void CatchingSTG::Slot6(int nValue) {
    mMuseSynth->AddMuseSink(mMixer);
    mMixer->mUnknown04 = nValue;
}

// 0x001a05c8
void CatchingSTG::Slot7(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mCatcher->AddSink(pSink);
    mNeutralizer->AddSink(pSink);
}

// 0x001a0650
void CatchingSTG::Slot8(int nValue) {
    if (nValue != 0) {
        mPhraseMgr->mUnknown1c = nValue;
    }
}

// 0x001a06f0
int CatchingSTG::Slot9() {
    return mCatcher->Slot6();
}

// 0x001a0668
int CatchingSTG::Slot10(int nFirst, int nSecond) {
    // Yes, the binary discards this call's result. It is what remains of a compiled-away assertion.
    Application::shared()->GetGameManager()->GetGameMode();

    // Unguarded on purpose: a MultiCatcher yields a null receiver here.
    return dynamic_cast<SingleCatcher *>(mCatcher)->Score(nFirst, nSecond);
}

// 0x001a0428
int CatchingSTG::Slot11() {
    return 1;
}
