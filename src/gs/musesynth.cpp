#include "gs/musesynth.h"

#include "gs/multimuseplayer.h"
#include "gs/noteplayer.h"
#include "msg/allnotesoffmsg.h"
#include "msg/multimusemsg.h"
#include "msg/notemsg.h"
#include "msg/stdmidimsg.h"
#include "msg/sustainnotemsg.h"
#include "synth/synthsustainer.h"

// NTSC-U/C: 0x00686290, PAL: 0x006c74f8
int sMuseID;

MusePlayer::MusePlayer() : mId(++sMuseID) {
}

MuseSynth::MuseSynth(Sch::TickClock *pClock)
    : mClock(pClock), mSustainer(nullptr), mOutput(&mSplitter) {
}

MuseSynth::~MuseSynth() {
    ReleaseAllPlayers();
    delete mSustainer;
}

MsgSplitter::~MsgSplitter() {
}

MsgSplitter::MsgSplitter() {
}

void MuseSynth::CreateSustainer() {
    mSustainer = new SynthSustainer();
    mOutput = mSustainer;
    mSustainer->mSink = &mSplitter;
}

int MuseSynth::IsActive() const {
    return mPlayers.size() != 0;
}

void MuseSynth::AddSink(MsgSink *pSink) {
    mSplitter.MsgSource::AddSink(pSink);
}

void MuseSynth::OnStdMidi(StdMidiMsg *pMsg) {
    mOutput->Dispatch(pMsg);
}

void MuseSynth::OnSustainNote(SustainNoteMsg *pMsg) {
    mOutput->Dispatch(pMsg);
}

void MuseSynth::OnAllNotesOff() {
    ReleaseAllPlayers();
}

void MuseSynth::ReleaseAllPlayers() {
    for (std::list<MusePlayer *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        (*it)->Stop();
        delete *it;
    }
    mPlayers.clear();
}

void MuseSynth::StartNotePlayer(Message *pMsg) {
    NoteMsg *pNote = static_cast<NoteMsg *>(pMsg);
    NotePlayer *pPlayer = new NotePlayer(
        pNote->mNote, pNote->mVelocity, pNote->mLength.mTick, pNote->mChannel, this, mClock);
    mPlayers.push_back(pPlayer);
    pPlayer->Start(mOutput);
}

void MuseSynth::StartMultiMusePlayer(Message *pMsg) {
    MultiMusePlayer *pMultiPlayer =
        new MultiMusePlayer(static_cast<MultiMuseMsg *>(pMsg)->mMuse, this, mClock);
    mPlayers.push_back(pMultiPlayer);
    pMultiPlayer->Start(mOutput);
}

void MuseSynth::RetainOnly(MusePlayer *pPlayer) {
    if (pPlayer->DisplacesSiblings() == 0) {
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

void MuseSynth::PlayerFinished(MusePlayer *pPlayer) {
    mPlayers.remove(pPlayer);
    delete pPlayer;
}

void MuseSynth::DispatchPriv(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(StdMidiMsg::sID) ||
        nType == static_cast<int>(SustainNoteMsg::sID)) {
        mOutput->Dispatch(pMsg);
        return;
    }
    if (nType == static_cast<int>(NoteMsg::sID)) {
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

void MsgSplitter::DispatchPriv(Message *pMsg) {
    Send(pMsg);
}
