#include "game/scoretrackgraph.h"

#include <vector>

#include "app/msgsource.h"
#include "game/midichase.h"
#include "game/playmap.h"
#include "mid/tick.h"
#include "sch/barsequencer.h"
#include "script/configquery.h"

namespace {

// The bar length the phrase manager is built with, 1920 ticks.
constexpr int kBarTicks = 1920;

// The configuration code the phrase manager's configuration word comes from.
constexpr int kPhraseMgrConfigCode = 702;

// The BarSequencer unmapped argument Start() passes, where BGTrackGraph passes its flag.
constexpr int kMapped = 0;

} // namespace

ScoreTrackGraph::ScoreTrackGraph(TrackData *pTrackData)
    : mTrack(pTrackData->mIndex), mTrackData(pTrackData), mPhraseMgr(nullptr),
      mPhrasePlayer(nullptr), mQuantizer(nullptr), mMuseSynth(nullptr), mUnused(0), mMixer(nullptr),
      mApplication(Application::shared()), mSequencer(nullptr) {
    mQuantizer = new Quantizer(mTrackData);
    mPhraseMgr = new PhraseMgr(mApplication->GetSongClock(),
                               Sch::Tick(kBarTicks).mTick,
                               mApplication->GetPlayMap(),
                               QueryConfigValue(kPhraseMgrConfigCode),
                               mTrackData);
    mPhrasePlayer = new PhrasePlayer(mPhraseMgr, mQuantizer, mTrackData);
    mMixer = new Mixer(mTrack, mTrackData->mChannel);
    mMuseSynth = new MuseSynth(mApplication->GetSongClock());
    mPhraseMgr->mPhrasePlayer = mPhrasePlayer;
}

ScoreTrackGraph::~ScoreTrackGraph() {
    delete mMuseSynth;
    delete mMixer;
    delete mPhrasePlayer;
    delete mPhraseMgr;
    delete mQuantizer;
}

void ScoreTrackGraph::Start() {
    mPhraseMgr->StartCommands();
    MidiChase chase;
    const int nBarCount = mApplication->GetPlayMap()->mBarCount;
    for (int i = 0; i < nBarCount; ++i) {
        const std::vector<TickObj<MuseMsg *> > *pMidi = mTrackData->GetMidiInBar(i);
        chase.HandleRange(pMidi->data(), pMidi->data() + pMidi->size());
    }
    chase.Replay(mMuseSynth);

    mSequencer = new BarSequencer(mApplication->GetSongClock(), mTrackData, mMuseSynth, kMapped);
    mSequencer->Start(Sch::Tick(0).mTick);
}

void ScoreTrackGraph::Stop() {
    mPhraseMgr->WithdrawCommands();
    delete mSequencer;
    mSequencer = nullptr;
}

void ScoreTrackGraph::ConnectGamer(MsgSource *) {
}

int ScoreTrackGraph::HasNothingPending() {
    return 1;
}

void ScoreTrackGraph::GivePhrases(int, Player *) {
}

int ScoreTrackGraph::CanGivePhrases() {
    return 0;
}

void ScoreTrackGraph::CreatePowerbarMgr() {
}

PhraseDatabase *ScoreTrackGraph::GetPhraseDatabase() {
    return mPhraseMgr->mDatabase;
}
