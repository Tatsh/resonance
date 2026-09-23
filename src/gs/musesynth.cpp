#include "gs/musesynth.h"

#include "gs/multimuseplayer.h"
#include "gs/noteplayer.h"
#include "msg/allnotesoffmsg.h"
#include "msg/multimusemsg.h"
#include "msg/notemsg.h"
#include "msg/stdmidimsg.h"
#include "msg/sustainnotemsg.h"
#include "synth/synthsustainer.h"

// 0x00686290
int g_nMusePlayerSerial;

// 0x001aa440
MusePlayer::MusePlayer() : mId(++g_nMusePlayerSerial) {
}

// 0x001aa4d8
MuseSynth::MuseSynth(Sch::TickClock *pClock)
    : mClock(pClock), mSustainer(nullptr), mOutput(&mSplitter) {
}

// 0x001aa5d8
MuseSynth::~MuseSynth() {
    ReleaseAllPlayers();
}

// 0x001aad98
MsgSplitter::~MsgSplitter() {
}

// 0x001aae68
MsgSplitter::MsgSplitter() {
}

// 0x001aafb0
void MuseSynth::CreateSustainer() {
    mSustainer = new SynthSustainer();
    mOutput = mSustainer;
    mSustainer->mSink = &mSplitter;
}

// 0x001aaf68
int MuseSynth::HasPlayers() {
    return mPlayers.size() != 0;
}

// 0x001ab038
void MuseSynth::AddSink(MsgSink *pSink) {
    mSplitter.MsgSource::AddSink(pSink);
}

// 0x001ab058
void MuseSynth::OnStdMidi(StdMidiMsg *pMsg) {
    mOutput->Handle(pMsg);
}

// 0x001ab088
void MuseSynth::OnSustainNote(SustainNoteMsg *pMsg) {
    mOutput->Handle(pMsg);
}

// 0x001ab0b8
void MuseSynth::OnAllNotesOff() {
    ReleaseAllPlayers();
}

// 0x001ab0d8
void MuseSynth::ReleaseAllPlayers() {
    for (std::list<MusePlayer *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        (*it)->Stop();
        delete *it;
    }
    mPlayers.clear();
}

// 0x001aa690
void MuseSynth::StartNotePlayer(Message *pMsg) {
    NoteMsg *pNote = static_cast<NoteMsg *>(pMsg);
    NotePlayer *pPlayer = new NotePlayer(
        pNote->mNote, pNote->mVelocity, pNote->mLength.mTick, pNote->mChannel, this, mClock);
    mPlayers.push_back(pPlayer);
    pPlayer->Start(mOutput);
}

// 0x001aa7c0
void MuseSynth::StartMultiMusePlayer(Message *pMsg) {
    MultiMusePlayer *pMultiPlayer =
        new MultiMusePlayer(static_cast<MultiMuseMsg *>(pMsg)->mMuse, this, mClock);
    mPlayers.push_back(pMultiPlayer);
    pMultiPlayer->Start(mOutput);
}

// 0x001aa908
void MuseSynth::RetainOnly(MusePlayer *pPlayer) {
    if (pPlayer->Slot4() == 0) {
        return;
    }

    std::list<MusePlayer *>::iterator it = mPlayers.begin();
    while (it != mPlayers.end()) {
        if (*it == pPlayer) {
            ++it;
            continue;
        }
        delete *it;
        it = mPlayers.erase(it);
    }
}

// 0x001aa9f0
void MuseSynth::PlayerFinished(MusePlayer *pPlayer) {
    mPlayers.remove(pPlayer);
    delete pPlayer;
}

// 0x001ab170
void MuseSynth::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_dwStdMidiMsgType) ||
        nType == static_cast<int>(g_dwSustainNoteMsgType)) {
        mOutput->Handle(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_dwNoteMsgType)) {
        StartNotePlayer(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_dwMultiMuseMsgType)) {
        StartMultiMusePlayer(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_dwAllNotesOffMsgType)) {
        ReleaseAllPlayers();
    }
}

// 0x001ab4a8
void MsgSplitter::HandleMessage(Message *pMsg) {
    Send(pMsg);
}
