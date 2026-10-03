#include "mid/reader.h"

#include <algorithm>
#include <cstring>

#include "mid/receiver.h"
#include "stream/hxchunkheader.h"
#include "stream/hxchunkname.h"
#include "stream/hxidatachunk.h"
#include "stream/hxilistchunk.h"
#include "stream/hxstream.h"

namespace {

constexpr unsigned char kStatusBit = 0x80;
constexpr unsigned char kStatusTypeMask = 0xf0;
constexpr unsigned char kChannelMask = 0x0f;

constexpr unsigned char kStatusNoteOff = 0x80;
constexpr unsigned char kStatusNoteOn = 0x90;
constexpr unsigned char kStatusPolyPressure = 0xa0;
constexpr unsigned char kStatusController = 0xb0;
constexpr unsigned char kStatusProgramChange = 0xc0;
constexpr unsigned char kStatusChannelPressure = 0xd0;
constexpr unsigned char kStatusPitchBend = 0xe0;
constexpr unsigned char kStatusSystem = 0xf0;
constexpr unsigned char kStatusSysEx = 0xf0;
constexpr unsigned char kStatusSysExContinue = 0xf7;
constexpr unsigned char kStatusMeta = 0xff;

constexpr unsigned char kMetaSequenceNumber = 0x00;
constexpr unsigned char kMetaFirstText = 0x01;
constexpr unsigned char kMetaLastText = 0x07;
constexpr unsigned char kMetaEndOfTrack = 0x2f;
constexpr unsigned char kMetaTempo = 0x51;
constexpr unsigned char kMetaSmpteOffset = 0x54;

constexpr short kFormatSingleTrack = 0;
constexpr short kFormatSimultaneous = 1;
constexpr short kFormatSequential = 2;

constexpr int kTempoHighShift = 16;
constexpr int kTempoMiddleShift = 8;

} // namespace

// NTSC-U/C: 0x003d4a68, PAL: 0x0040c958
Mid::Reader::Reader(HxIListChunk *pReader, Receiver *pReceiver)
    : mFormat(-1), mTrackCount(0), mDivision(0), mTargetDivision(kTargetDivision), mTrack(0),
      mReceiver(pReceiver), mTick(0), mRunningStatus(0), mTrackDone(0), mReader(pReader),
      mCompare(nullptr) {
}

// NTSC-U/C: 0x003d6528, PAL: 0x0040e418
void Mid::Reader::ReadAllTracks() {
    while (ReadChunk()) {
    }
}

// NTSC-U/C: 0x003d4ac8, PAL: 0x0040c9b8
bool Mid::Reader::ReadChunk() {
    HxChunkHeader *pId = mReader->Next();
    if (pId == nullptr) {
        AllTracksRead();
        return false;
    }

    if (pId->Name() == kMidiHeaderChunkID) {
        HxIDataChunk chunk(mReader);
        ReadHeader(chunk);
    } else if (pId->Name() == kMidiTrackChunkID) {
        HxIDataChunk chunk(mReader);
        ReadTrackChunk(chunk);
    }
    return true;
}

// NTSC-U/C: 0x003d65a8, PAL: 0x0040e498
void Mid::Reader::AllTracksRead() {
    mReceiver->AllDone();
}

// NTSC-U/C: 0x003d6558, PAL: 0x0040e448
void Mid::Reader::ReadHeader(HxStream &stream) {
    stream.ReadNum(&mFormat, sizeof(mFormat))
        .ReadNum(&mTrackCount, sizeof(mTrackCount))
        .ReadNum(&mDivision, sizeof(mDivision));
}

// NTSC-U/C: 0x003d65d8, PAL: 0x0040e4c8
void Mid::Reader::ReadTrackChunk(HxStream &stream) {
    switch (mFormat) {
    case kFormatSingleTrack:
        break; // Yes, the binary reads no track of a format 0 file.
    case kFormatSimultaneous:
    case kFormatSequential:
        ReadTrack(stream);
        break;
    }
}

// NTSC-U/C: 0x003d6610, PAL: 0x0040e500
void Mid::Reader::ReadTrack(HxStream &stream) {
    mReceiver->NewTrack(static_cast<unsigned char>(mTrack));
    mTrackDone = 0;
    mRunningStatus = 0;
    mTick = 0;
    mPendingTick = MBT(-1);
    while (mTrackDone == 0) {
        ReadEvent(stream);
    }
    ++mTrack;
}

// NTSC-U/C: 0x003d4c10, PAL: 0x0040cb00
void Mid::Reader::ReadEvent(HxStream &stream) {
    int nDelta;
    ReadVarLen(nDelta, stream);
    mTick += nDelta;
    MBT tick(mTick * mTargetDivision / mDivision);

    unsigned char nData1;
    stream.ReadNum(&nData1, sizeof(nData1));
    bool bRunning = true;
    if ((nData1 & kStatusBit) != 0) {
        mRunningStatus = nData1;
        bRunning = false;
    }

    if ((mRunningStatus & kStatusTypeMask) == kStatusSystem) {
        ReadSystemEvent(stream);
        mRunningStatus = 0;
        return;
    }

    if (!bRunning) {
        stream.ReadNum(&nData1, sizeof(nData1));
    }

    unsigned char nStatus = mRunningStatus;
    unsigned char nData2; // Yes, the binary delivers it uninitialised for an unknown status.
    switch (nStatus & kStatusTypeMask) {
    case kStatusNoteOff:
    case kStatusPolyPressure:
    case kStatusController:
    case kStatusPitchBend:
        stream.ReadNum(&nData2, sizeof(nData2));
        break;
    case kStatusNoteOn:
        stream.ReadNum(&nData2, sizeof(nData2));
        if (nData2 == 0) {
            nStatus = (nStatus & kChannelMask) | kStatusNoteOff;
        }
        break;
    case kStatusProgramChange:
    case kStatusChannelPressure:
        nData2 = 0;
        break;
    }
    QueueEvent(tick, nStatus, nData1, nData2);
}

// NTSC-U/C: 0x003d4df0, PAL: 0x0040cce0
void Mid::Reader::ReadSystemEvent(HxStream &stream) {
    switch (mRunningStatus) {
    case kStatusSysEx:
    case kStatusSysExContinue: {
        int nLength;
        ReadVarLen(nLength, stream);
        stream.SetMarker(nLength, kHxSeekCur);
        break;
    }
    case kStatusMeta: {
        unsigned char nType;
        stream.ReadNum(&nType, sizeof(nType));
        ReadMeta(nType, stream);
        break;
    }
    }
}

// NTSC-U/C: 0x003d4f00, PAL: 0x0040cdf0
void Mid::Reader::ReadMeta(unsigned char nType, HxStream &stream) {
    int nLength;
    ReadVarLen(nLength, stream);
    int nStart = stream.GetMarker();
    MBT tick(mTick * mTargetDivision / mDivision);

    switch (nType) {
    case kMetaSequenceNumber:
    case kMetaSmpteOffset:
        break;
    case kMetaEndOfTrack:
        if (mCompare != nullptr) {
            Flush();
            mPendingTick = MBT(-1);
        }
        mTrackDone = 1;
        mReceiver->EndTrack();
        break;
    case kMetaTempo: {
        unsigned char nHigh;
        unsigned char nMiddle;
        unsigned char nLow;
        stream.ReadNum(&nHigh, sizeof(nHigh))
            .ReadNum(&nMiddle, sizeof(nMiddle))
            .ReadNum(&nLow, sizeof(nLow));
        mReceiver->Tempo(tick.mTick,
                         ((nHigh << kTempoHighShift) + (nMiddle << kTempoMiddleShift)) | nLow);
        break;
    }
    default:
        if (nType >= kMetaFirstText && nType <= kMetaLastText) {
            char *pszText = new char[nLength + 1];
            stream.ReadData(pszText, nLength);
            pszText[nLength] = '\0';
            mReceiver->TextEvent(tick.mTick, pszText, nType);
            delete[] pszText;
        }
        break;
    }
    stream.SetMarker(nStart + nLength, kHxSeekSet);
}

// NTSC-U/C: 0x003d5260, PAL: 0x0040d150
void Mid::Reader::SendChannelMsg(MBT tick,
                                 unsigned char nStatus,
                                 unsigned char nData1,
                                 unsigned char nData2) {
    unsigned char nChannel = nStatus & kChannelMask;
    switch (nStatus & kStatusTypeMask) {
    case kStatusNoteOn:
        mReceiver->NoteOn(tick.mTick, nData1, nData2, nChannel);
        break;
    case kStatusNoteOff:
        mReceiver->NoteOff(tick.mTick, nData1, nChannel);
        break;
    case kStatusController:
        mReceiver->Controller(tick.mTick, nData1, nData2, nChannel);
        break;
    case kStatusPitchBend:
        mReceiver->PitchBend(tick.mTick, nData1, nData2, nChannel);
        break;
    case kStatusProgramChange:
        mReceiver->ProgramChange(tick.mTick, nData1, nChannel);
        break;
    }
}

// NTSC-U/C: 0x003d66a8, PAL: 0x0040e598
void Mid::Reader::QueueEvent(MBT tick,
                             unsigned char nStatus,
                             unsigned char nData1,
                             unsigned char nData2) {
    if (mCompare == nullptr) {
        SendChannelMsg(tick, nStatus, nData1, nData2);
        return;
    }

    if (tick.mTick != mPendingTick.mTick) {
        Flush();
        mPendingTick = tick;
    }
    Event event;
    memset(&event, 0, sizeof(event));
    event.mStatus = nStatus;
    event.mData1 = nData1;
    event.mData2 = nData2;
    mPending.push_back(event);
}

// NTSC-U/C: 0x003d5140, PAL: 0x0040d030
void Mid::Reader::Flush() {
    if (mPending.size() == 0) {
        return;
    }

    std::sort(mPending.begin(), mPending.end(), mCompare);
    for (const auto &event : mPending) {
        SendChannelMsg(mPendingTick, event.mStatus, event.mData1, event.mData2);
    }
    mPending.erase(mPending.begin(), mPending.end());
}
