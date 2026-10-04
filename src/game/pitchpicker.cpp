#include "game/pitchpicker.h"

#include <cstring>
#include <vector>

#include "game/harmony.h"
#include "game/lineartransform.h"
#include "game/nullplayer.h"
#include "game/riffrangefinder.h"
#include "msg/axisregistermsg.h"
#include "msg/trackselectmsg.h"

namespace {

// The axis position the constructor assumes, the middle of the 0..1024 scale.
constexpr int kAxisCenter = 512;

// The input range PickPitch() maps the axis position from.
constexpr int kAxisMinimum = 0;
constexpr int kAxisMaximum = 1023;

// An AxisRegisterMsg's value, between 0 and 1, is scaled onto 0..1024.
constexpr float kAxisScale = 1024.0f;

// The riff range the constructor assumes until a MultiMuseMsg arrives, middle C at both ends.
constexpr int kMiddleC = 60;

// The sustain tick the constructor assumes, before any SustainNoteMsg.
constexpr int kNoSustainTick = -1;

// The high nibble of a channel message's status, and the two note kinds OnStdMidi() separates.
constexpr unsigned char kStatusKindMask = 0xf0;
constexpr unsigned char kStatusNoteOff = 0x80;
constexpr unsigned char kStatusNoteOn = 0x90;

// The velocity PostNoteOff() sends.
constexpr unsigned char kReleaseVelocity = 0;

// A pairing with every byte cleared, then filled.
inline PitchPicker::NoteMapping MakeMapping(unsigned char nNote, unsigned char nPitch) {
    PitchPicker::NoteMapping mapping;
    std::memset(&mapping, 0, sizeof(mapping));
    mapping.mNote = nNote;
    mapping.mPitch = nPitch;
    return mapping;
}

} // namespace

PitchPicker::PitchPicker(const TrackData *pTrackData)
    : mTrackData(pTrackData), mAxis(kAxisCenter), mSustainTick(kNoSustainTick), mRiffLow(kMiddleC),
      mRiffHigh(kMiddleC), mTrack(pTrackData->mIndex), mPlayer(&NullPlayer::sInstance) {
}

void PitchPicker::OnMsg(const MultiMuseMsg &msg) {
    RiffRangeFinder finder(msg.mMuse, &mRiffLow, &mRiffHigh);
}

void PitchPicker::PostSustainNoteMsg(SustainNoteMsg *pMsg) {
    mSustainTick.mTick = pMsg->mTick; // The tick is stored without the finiteness check.
    SustainNoteMsg sustain(pMsg->mTick, GetSustainPitch(pMsg->mTick, pMsg->mNote));
    Send(&sustain);
}

void PitchPicker::PostNoteOn(int nTick,
                             unsigned char nStatus,
                             unsigned char nNote,
                             unsigned char nVelocity) {
    const unsigned char nPitch =
        (nTick == mSustainTick.mTick) ? GetSustainPitch(nTick, nNote) : PickPitch(nTick, nNote);
    mHeldNotes.push_back(MakeMapping(nNote, nPitch));

    StdMidiMsg msg(nTick, nStatus, nPitch, nVelocity);
    Send(&msg);
}

void PitchPicker::PostNoteOff(int nTick, unsigned char nStatus, unsigned char nNote) {
    if (nTick != mSustainTick.mTick) {
        mSustainNotes.clear();
    }

    for (std::vector<NoteMapping>::iterator it = mHeldNotes.begin(); it != mHeldNotes.end(); ++it) {
        if (it->mNote == nNote) {
            StdMidiMsg msg(nTick, nStatus, it->mPitch, kReleaseVelocity);
            Send(&msg);
            mHeldNotes.erase(it);
            return;
        }
    }
}

unsigned char PitchPicker::GetSustainPitch(int nTick, unsigned char nNote) {
    for (std::vector<NoteMapping>::iterator it = mSustainNotes.begin(); it != mSustainNotes.end();
         ++it) {
        if (it->mNote == nNote) {
            return it->mPitch;
        }
    }

    const unsigned char nPitch = PickPitch(nTick, nNote);
    mSustainNotes.push_back(MakeMapping(nNote, nPitch));
    return nPitch;
}

void PitchPicker::DispatchPriv(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_dwMultiMuseMsgType)) {
        OnMsg(*static_cast<MultiMuseMsg *>(pMsg));
    } else if (nType == static_cast<int>(StdMidiMsg::sID)) {
        OnStdMidi(static_cast<StdMidiMsg *>(pMsg));
    } else if (nType == static_cast<int>(SustainNoteMsg::sID)) {
        PostSustainNoteMsg(static_cast<SustainNoteMsg *>(pMsg));
    } else if (nType == g_nAxisRegisterMsgType) {
        AxisRegisterMsg *pAxis = static_cast<AxisRegisterMsg *>(pMsg);
        if (pAxis->mPlayer == mPlayer) {
            mAxis = static_cast<int>(pAxis->mValue * kAxisScale);
        }
    } else if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        TrackSelectMsg *pSelect = static_cast<TrackSelectMsg *>(pMsg);
        if (pSelect->mTrack == mTrack && pSelect->mPlace == 0) {
            mPlayer = pSelect->mPlayer;
        }
    }
}

void PitchPicker::OnStdMidi(StdMidiMsg *pMsg) {
    const unsigned char nStatus = pMsg->mStatus;
    switch (nStatus & kStatusKindMask) {
    case kStatusNoteOff:
        PostNoteOff(pMsg->mTick, nStatus, pMsg->mData1);
        break;
    case kStatusNoteOn:
        PostNoteOn(pMsg->mTick, nStatus, pMsg->mData1, pMsg->mData2);
        break;
    default:
        Send(pMsg);
        break;
    }
}

unsigned char PitchPicker::PickPitch(int nTick, unsigned char nNote) {
    Harmony *pHarmony = mTrackData->GetHarmony(nTick);
    if (pHarmony == nullptr) {
        return nNote;
    }

    int nLow;
    int nHigh;
    pHarmony->GetRange(&nLow, &nHigh);
    LinearTransform map(kAxisMinimum, kAxisMaximum, nLow - mRiffLow, nHigh - mRiffHigh);
    return pHarmony->SnapToHarmony(static_cast<unsigned char>(nNote + map.Apply(mAxis)));
}
