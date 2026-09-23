#include "game/pitchpicker.h"

#include <cstring>
#include <vector>

#include "game/harmony.h"
#include "game/linearmap.h"
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

// 0x001c29c8
PitchPicker::PitchPicker(const TrackData *pTrackData)
    : mTrackData(pTrackData), mAxis(kAxisCenter), mSustainTick(kNoSustainTick), mRiffLow(kMiddleC),
      mRiffHigh(kMiddleC), mTrack(pTrackData->mUnknown04), mPlayer(&g_nullPlayer) {
}

// 0x001c2c60
void PitchPicker::FindRiffRange(MultiMuseMsg *pMsg) {
    RiffRangeFinder finder(pMsg->mMuse, &mRiffLow, &mRiffHigh);
}

// 0x001c2d40
void PitchPicker::PostSustainNoteMsg(SustainNoteMsg *pMsg) {
    mSustainTick.mTick = pMsg->mTick; // The tick is stored without the finiteness check.
    SustainNoteMsg sustain(pMsg->mTick, GetSustainPitch(pMsg->mTick, pMsg->mUnknown08));
    Send(&sustain);
}

// 0x001c2dd0
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

// 0x001c2f08
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

// 0x001c3060
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

// 0x001c3130
void PitchPicker::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_dwMultiMuseMsgType)) {
        FindRiffRange(static_cast<MultiMuseMsg *>(pMsg));
    } else if (nType == static_cast<int>(g_dwStdMidiMsgType)) {
        OnStdMidi(static_cast<StdMidiMsg *>(pMsg));
    } else if (nType == static_cast<int>(g_dwSustainNoteMsgType)) {
        PostSustainNoteMsg(static_cast<SustainNoteMsg *>(pMsg));
    } else if (nType == g_nAxisRegisterMsgType) {
        AxisRegisterMsg *pAxis = static_cast<AxisRegisterMsg *>(pMsg);
        if (pAxis->mPlayer == mPlayer) {
            mAxis = static_cast<int>(pAxis->mValue * kAxisScale);
        }
    } else if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        TrackSelectMsg *pSelect = static_cast<TrackSelectMsg *>(pMsg);
        if (pSelect->mUnknown04 == mTrack && pSelect->mUnknown08 == 0) {
            mPlayer = pSelect->mUnknown10;
        }
    }
}

// 0x001c43b0
void PitchPicker::OnStdMidi(StdMidiMsg *pMsg) {
    const unsigned char nStatus = pMsg->mUnknown08;
    switch (nStatus & kStatusKindMask) {
    case kStatusNoteOff:
        PostNoteOff(pMsg->mTick, nStatus, pMsg->mUnknown09);
        break;
    case kStatusNoteOn:
        PostNoteOn(pMsg->mTick, nStatus, pMsg->mUnknown09, pMsg->mUnknown0a);
        break;
    default:
        Send(pMsg);
        break;
    }
}

// 0x001c4488
unsigned char PitchPicker::PickPitch(int nTick, unsigned char nNote) {
    Harmony *pHarmony = mTrackData->GetHarmony(nTick);
    if (pHarmony == nullptr) {
        return nNote;
    }

    int nLow;
    int nHigh;
    pHarmony->GetRange(&nLow, &nHigh);
    LinearMap map(kAxisMinimum, kAxisMaximum, nLow - mRiffLow, nHigh - mRiffHigh);
    return pHarmony->Snap(static_cast<unsigned char>(nNote + map.Map(mAxis)));
}
