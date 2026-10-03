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
int g_nMusePlayerSerial;

// NTSC-U/C: 0x001aa440, PAL: 0x001b01a8
MusePlayer::MusePlayer() : mId(++g_nMusePlayerSerial) {
}

// NTSC-U/C: 0x001aa4d8, PAL: 0x001b0240
MuseSynth::MuseSynth(Sch::TickClock *pClock)
    : mClock(pClock), mSustainer(nullptr), mOutput(&mSplitter) {
}

// NTSC-U/C: 0x001aa5d8, PAL: 0x001b0340
MuseSynth::~MuseSynth() {
    ReleaseAllPlayers();
    delete mSustainer;
}

// NTSC-U/C: 0x001aad98, PAL: 0x001b0b00
MsgSplitter::~MsgSplitter() {
}

// NTSC-U/C: 0x001aae68, PAL: 0x001b0bd0
MsgSplitter::MsgSplitter() {
}

// NTSC-U/C: 0x001aafb0, PAL: 0x001b0d18
void MuseSynth::CreateSustainer() {
    mSustainer = new SynthSustainer();
    mOutput = mSustainer;
    mSustainer->mSink = &mSplitter;
}

// NTSC-U/C: 0x001aaf68, PAL: 0x001b0cd0
int MuseSynth::HasPlayers() {
    return mPlayers.size() != 0;
}

// NTSC-U/C: 0x001ab038, PAL: 0x001b0da0
void MuseSynth::AddSink(MsgSink *pSink) {
    mSplitter.MsgSource::AddSink(pSink);
}

// NTSC-U/C: 0x001ab058, PAL: 0x001b0dc0
void MuseSynth::OnStdMidi(StdMidiMsg *pMsg) {
    mOutput->Handle(pMsg);
}

// NTSC-U/C: 0x001ab088, PAL: 0x001b0df0
void MuseSynth::OnSustainNote(SustainNoteMsg *pMsg) {
    mOutput->Handle(pMsg);
}

// NTSC-U/C: 0x001ab0b8, PAL: 0x001b0e20
void MuseSynth::OnAllNotesOff() {
    ReleaseAllPlayers();
}

// NTSC-U/C: 0x001ab0d8, PAL: 0x001b0e40
void MuseSynth::ReleaseAllPlayers() {
    for (std::list<MusePlayer *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        (*it)->Stop();
        delete *it;
    }
    mPlayers.clear();
}

// NTSC-U/C: 0x001aa690, PAL: 0x001b03f8
void MuseSynth::StartNotePlayer(Message *pMsg) {
    NoteMsg *pNote = static_cast<NoteMsg *>(pMsg);
    NotePlayer *pPlayer = new NotePlayer(
        pNote->mNote, pNote->mVelocity, pNote->mLength.mTick, pNote->mChannel, this, mClock);
    mPlayers.push_back(pPlayer);
    pPlayer->Start(mOutput);
}

// NTSC-U/C: 0x001aa7c0, PAL: 0x001b0528
void MuseSynth::StartMultiMusePlayer(Message *pMsg) {
    MultiMusePlayer *pMultiPlayer =
        new MultiMusePlayer(static_cast<MultiMuseMsg *>(pMsg)->mMuse, this, mClock);
    mPlayers.push_back(pMultiPlayer);
    pMultiPlayer->Start(mOutput);
}

// NTSC-U/C: 0x001aa908, PAL: 0x001b0670
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

// NTSC-U/C: 0x001aa9f0, PAL: 0x001b0758
void MuseSynth::PlayerFinished(MusePlayer *pPlayer) {
    mPlayers.remove(pPlayer);
    delete pPlayer;
}

// NTSC-U/C: 0x001ab170, PAL: 0x001b0ed8
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

// NTSC-U/C: 0x001ab4a8, PAL: 0x001b1210
void MsgSplitter::HandleMessage(Message *pMsg) {
    Send(pMsg);
}
