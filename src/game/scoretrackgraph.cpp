#include "game/scoretrackgraph.h"

#include <vector>

#include "app/msgsource.h"
#include "game/midichase.h"
#include "game/playmap.h"
#include "mid/mbt.h"
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

// 0x001cee50
ScoreTrackGraph::ScoreTrackGraph(TrackData *pTrackData)
    : mTrack(pTrackData->mIndex), mTrackData(pTrackData), mPhraseMgr(nullptr),
      mPhrasePlayer(nullptr), mQuantizer(nullptr), mMuseSynth(nullptr), mUnused(0), mMixer(nullptr),
      mApplication(Application::shared()), mSequencer(nullptr) {
    mQuantizer = new Quantizer(mTrackData);
    mPhraseMgr = new PhraseMgr(mApplication->GetSongClock(),
                               Mid::MBT(kBarTicks).mTick,
                               mApplication->GetPlayMap(),
                               QueryConfigValue(kPhraseMgrConfigCode),
                               mTrackData);
    mPhrasePlayer = new PhrasePlayer(mPhraseMgr, mQuantizer, mTrackData);
    mMixer = new Mixer(mTrack, mTrackData->mChannel);
    mMuseSynth = new MuseSynth(mApplication->GetSongClock());
    mPhraseMgr->mPhrasePlayer = mPhrasePlayer;
}

// 0x001cf840
ScoreTrackGraph::~ScoreTrackGraph() {
    delete mMuseSynth;
    delete mMixer;
    delete mPhrasePlayer;
    delete mPhraseMgr;
    delete mQuantizer;
}

// 0x001cf088
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
    mSequencer->Start(Mid::MBT(0).mTick);
}

// 0x001cf918
void ScoreTrackGraph::Stop() {
    mPhraseMgr->WithdrawCommands();
    delete mSequencer;
    mSequencer = nullptr;
}

// 0x001cf750
void ScoreTrackGraph::AddMixerToSource(MsgSource *) {
}

// 0x001cf758
int ScoreTrackGraph::HasNothingPending() {
    return 1;
}

// 0x001cf760
void ScoreTrackGraph::GivePhrases(int, Player *) {
}

// 0x001cf768
int ScoreTrackGraph::CanGivePhrases() {
    return 0;
}

// 0x001cf778
void ScoreTrackGraph::CreatePowerbarMgr() {
}

// 0x001cf978
PhraseDatabase *ScoreTrackGraph::GetPhraseDatabase() {
    return mPhraseMgr->mDatabase;
}
