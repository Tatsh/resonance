#include "game/levelconverter.h"

#include <algorithm>
#include <cstdio>

#include "game/levelbuilder.h"
#include "game/playmap.h"
#include "game/riff.h"
#include "game/trackdata.h"
#include "mid/mbt.h"
#include "msg/sustainnotemsg.h"
#include "os/hostmode.h"
#include "script/configquery.h"

namespace {

// MIDI meta type 3 is the track name, which is the one text event this class acts on.
constexpr unsigned char kTrackNameMetaType = 3;

constexpr int kTicksPerBar = 1920;
constexpr int kTicksPerBeat = 480;

constexpr unsigned char kMidiControlChange = 0xb0;
constexpr unsigned char kMidiProgramChange = 0xc0;
constexpr unsigned char kMidiBankSelectMsb = 0;
constexpr unsigned char kMidiBankSelectLsb = 0x20;

// The value mProgram and mChannel have until the track supplies one.
constexpr unsigned char kNoProgram = 0xff;

constexpr int kGemDifficultyQuery = 0x38a;

// The gem track's octaves, counted from the lowest, are the three difficulties.
constexpr int kFirstGemOctave = 5;
constexpr int kNotesPerOctave = 12;

// The pitch classes a gem track may use, one per gem.
constexpr int kPitchClassC = 0;
constexpr int kPitchClassE = 4;
constexpr int kPitchClassG = 7;

enum Gem {
    kGemC = 0,
    kGemE = 1,
    kGemG = 2,
};

// The difficulty whose gems a ghost track also sends to the builder.
constexpr int kGhostGemDifficulty = 2;

// The riffs one riff set can take, one per difficulty.
constexpr int kRiffsPerSet = 3;

// The play mode in which a catch track's gems double as a ghost track's.
constexpr int kGhostPlayMode = 2;

// The results of CheckRiffPosition().
constexpr int kRiffPositionInside = 0;
constexpr int kRiffPositionBefore = 1;
constexpr int kRiffPositionAfter = -1;

// Set by Convert() so the first report of a conversion opens the log.
int g_bErrorLogPending;

// Convert() writes the log's path, the MIDI file's base name with the .err extension.
char g_szErrorLogPath[64];

FILE *g_pErrorLog;

Mid::MBT ClampMBT(int nTick) {
    return Mid::MBT(std::min(std::max(nTick, kMBTMinimum), kMBTMaximum));
}

} // namespace

// 0x001ea6e0
inline void LevelConverter::ReportError(int nTick, const char *pszMessage) {
    if (g_bErrorLogPending) {
        if (MidiErrorLogEnabled()) {
            g_pErrorLog = fopen(g_szErrorLogPath, "w");
        }
        g_bErrorLogPending = 0;
    }
    if (MidiErrorLogEnabled()) {
        fprintf(g_pErrorLog,
                "ErrT%d [%s] at Tick %d:%d.%d - %s\n",
                mTrack,
                mTrackName.mStr != nullptr ? mTrackName.mStr : g_szEmptyString,
                nTick / kTicksPerBar + 1,
                nTick % kTicksPerBar / kTicksPerBeat + 1,
                nTick % kTicksPerBar % kTicksPerBeat,
                pszMessage);
    }
}

// 0x001e6278
// The three HxStr members, the positions, and the six collections are default-constructed by the
// expansions the compiler places ahead of and around these stores.
LevelConverter::LevelConverter() : mUnknown90(0), mDifficulty(0) {
}

// 0x001e9ee0
// Every statement in the body is the compiler expanding the destructor of a member,
// the three span collections and the name map first, then the pending-event collection, then the
// three strings in reverse declaration order.
LevelConverter::~LevelConverter() {
}

// 0x001ea570
void LevelConverter::Tempo(int nTick, int nMicrosecondsPerQuarter) {
    mBuilder->SetTempo(nTick, nMicrosecondsPerQuarter);
    mHasTempo = 1;
}

// 0x001ea5a0
void LevelConverter::TextEvent(int nTick, const char *pText, unsigned char nType) {
    if (nTick == Mid::MBT(0).mTick && nType == kTrackNameMetaType) {
        ParseTrackTypeString(pText);
    }
}

// 0x001e6bd0
void LevelConverter::ApplyTrackType() {
    mHarmonyTrack = 0;
    mGemSpanTrack = 0;
    mGhostGems = 0;
    mDifficulty = 0;
    switch (mTrackType) {
    case kTrackTypeAxe:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mBuilder->SetKind(kTrackModeAxe);
        mBuilder->SetInstrument(mInstrument, mDisplayName);
        mRiffTrack = 1;
        mUnknown3c = 1;
        break;
    case kTrackTypePitch:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mBuilder->SetKind(kTrackModeRiff);
        mBuilder->SetInstrument(mInstrument, mDisplayName);
        mRiffTrack = 1;
        mUnknown3c = 1;
        break;
    case kTrackTypeScratch:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mBuilder->SetKind(kTrackModeScratch);
        mBuilder->SetInstrument(mInstrument, mDisplayName);
        mRiffTrack = 1;
        mUnknown3c = 1;
        break;
    case kTrackTypeVocal:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mBuilder->SetKind(kTrackModeVocal);
        mBuilder->SetInstrument(mInstrument, mDisplayName);
        mRiffTrack = 0;
        mUnknown3c = 1;
        break;
    case kTrackTypeData:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mUnknown3c = 0;
        mRiffTrack = 0;
        break;
    case kTrackTypeHarmony:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mUnknown3c = 0;
        mHarmonyTrack = 1;
        mRiffTrack = 0;
        break;
    case kTrackTypeCatch:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mBuilder->SetKind(kTrackModeCatch);
        mBuilder->SetInstrument(mInstrument, mDisplayName);
        mRiffTrack = 1;
        mUnknown3c = 1;
        mDifficulty = QueryConfigValue(kGemDifficultyQuery);
        if (static_cast<unsigned>(mDifficulty) >= static_cast<unsigned>(kDifficultyCount)) {
            // The report skips the span cursor below.
            ReportError(Mid::MBT(0).mTick, "Gem difficulty must be in range [0,2]");
            return;
        }
        break;
    case kTrackTypeBackground:
        mBuilder->SelectTrack(kLevelTrackBacking, mBackingTrackCount);
        mUnknown3c = 1;
        mRiffTrack = 0;
        ++mBackingTrackCount;
        break;
    case kTrackTypeIntro:
        mBuilder->SelectTrack(kLevelTrackIntro, mIntroTrackCount);
        mUnknown3c = 1;
        mRiffTrack = 0;
        ++mIntroTrackCount;
        break;
    case kTrackTypeControl:
        mBuilder->SelectTrack(kLevelTrackOwn, 0);
        mUnknown3c = 0;
        mRiffTrack = 0;
        break;
    case kTrackTypeInstrument:
        for (int i = 0; i < kDifficultyCount; ++i) {
            mSpans[i].clear();
        }
        mBuilder->SelectTrack(kLevelTrackNone, 0);
        mRiffTrack = 0;
        mGemSpanTrack = 1;
        mUnknown3c = 1;
        break;
    case kTrackTypeGhost:
        for (int i = 0; i < kDifficultyCount; ++i) {
            mSpans[i].clear();
        }
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mGemSpanTrack = 1;
        mUnknown3c = 1;
        mRiffTrack = 0;
        mGhostGems = mPlayMode == kGhostPlayMode;
        break;
    default:
        mUnknown3c = 0;
        mRiffTrack = 0;
        mBuilder->SelectTrack(kLevelTrackNone, 0);
        break;
    }
    mNextSpan = mSpans[mDifficulty].begin();
}

// 0x001e7420
void LevelConverter::AddNote(int nTick,
                             unsigned char nNote,
                             unsigned char nVelocity,
                             int nDuration,
                             unsigned char nChannel) {
    if (mGemSpanTrack) {
        AddGemSpan(nTick, nNote, nDuration);
        return;
    }
    if (mRiffTrack) {
        SyncRiff(nTick);
        if (mRiff == nullptr) {
            ReportError(nTick, "No gem found for Midi Note");
            return;
        }
        if (ClampMBT(nTick - mRiffStart.mTick).mTick < 0) {
            ReportError(nTick, "MidiNote duration extends into subsequent Riff");
            return;
        }
        EmitRiffProgram(nTick);
        if (mTrackType == kTrackTypeAxe && nDuration >= Mid::MBT(kTicksPerBar).mTick) {
            SustainNoteMsg sustain(Mid::MBT(0).mTick, nNote);
            mRiff->Add(&sustain, Mid::MBT(0).mTick, 0);
            const int nStart = Mid::MBT(0).mTick;
            const int nLength = ClampMBT(nDuration + Mid::MBT(1).mTick).mTick;
            mRiff->AddNoteMsg(nStart, nNote, nVelocity, nLength, nChannel);
            return;
        }
        mRiff->AddNoteMsg(
            ClampMBT(nTick - mRiffStart.mTick).mTick, nNote, nVelocity, nDuration, nChannel);
        return;
    }
    if (mBankSelect != 0) {
        PlayMap *pMap = mBuilder->OnUnknownSlot8();
        const int nBar = pMap->FindStepIndex(nTick / Mid::MBT(kTicksPerBar).mTick);
        if (nBar != mLastBankBar) {
            mLastBankBar = nBar;
            mBuilder->AddEvent(nTick, kMidiControlChange, kMidiBankSelectMsb, 0, mChannel);
            mBuilder->AddEvent(nTick, kMidiControlChange, kMidiBankSelectLsb, nBar, mChannel);
        }
    }
    mBuilder->AddNoteMsg(nTick, nNote, nVelocity, nDuration, nChannel);
}

// 0x001e7c20
void LevelConverter::AddGemSpan(int nTick, unsigned char nNote, int nDuration) {
    const int nDifficulty = nNote / kNotesPerOctave - kFirstGemOctave;
    if (static_cast<unsigned>(nDifficulty) >= static_cast<unsigned>(kDifficultyCount)) {
        ReportError(nTick, "Gem found in incorrect octave");
        return;
    }
    int nGem;
    switch (nNote % kNotesPerOctave) {
    case kPitchClassC:
        nGem = kGemC;
        break;
    case kPitchClassE:
        nGem = kGemE;
        break;
    case kPitchClassG:
        nGem = kGemG;
        break;
    default:
        ReportError(nTick, "Gem track notes must be C, E, or G");
        return;
    }
    std::vector<Span> &spans = mSpans[nDifficulty];
    if (spans.size() != 0 && !(spans.back().mStart.mTick < nTick)) {
        ReportError(nTick, "Gems cannot overlap.");
        return;
    }
    Span span;
    span.mStart.mTick = nTick;
    span.mGem = nGem;
    span.mLength.mTick = nDuration;
    spans.push_back(span);
    if (mGhostGems && nDifficulty == kGhostGemDifficulty) {
        mBuilder->AddGem(nTick, nGem, nullptr);
    }
}

// 0x001e7e00
int LevelConverter::CheckRiffPosition(int nTick) {
    if (mRiff == nullptr) {
        return kRiffPositionBefore;
    }
    Mid::MBT next(-1); // Yes, the binary checks this placeholder and then overwrites it.
    if (mNextSpan != mSpans[mDifficulty].end()) {
        next = mNextSpan->mStart;
    } else {
        next = Mid::MBT(kMBTMaximum);
    }
    const int bBeforeNext = nTick < next.mTick;
    if (!bBeforeNext) {
        return kRiffPositionAfter;
    }
    if (nTick < mRiffStart.mTick) {
        return kRiffPositionBefore;
    }
    return kRiffPositionInside;
}

// 0x001e7eb0
void LevelConverter::NextRiff() {
    if (mNextSpan == mSpans[mDifficulty].end()) {
        return;
    }
    mRiffStart = mNextSpan->mStart;
    const int nGem = mNextSpan->mGem;
    const Mid::MBT length = mNextSpan->mLength;
    if (mTrackType == kTrackTypeAxe || mTrackType == kTrackTypePitch ||
        mTrackType == kTrackTypeScratch) {
        if (nGem != 0) {
            ++mRiffIndex;
        } else {
            mRiffSetStart = mRiffStart;
            mRiffIndex = 0;
        }
        if (static_cast<unsigned>(mRiffIndex) >= static_cast<unsigned>(kRiffsPerSet)) {
            ReportError(mRiffStart.mTick, "Too many riffs in riffset\n");
            return;
        }
        mRiff = new Riff(mRiffIndex);
        mRiff->mLength = length;
        mBuilder->AddRiff(mRiffSetStart.mTick, mRiff);
    } else {
        mRiff = new Riff(nGem);
        mRiff->mLength = length;
        mBuilder->AddGem(mRiffStart.mTick, nGem, mRiff);
    }
    mUnknown6c = 1;
    mProgramSent = 0;
    ++mNextSpan;
}

// 0x001e8cb8
void LevelConverter::EmitRiffProgram(int nTick) {
    if (mProgram == kNoProgram) {
        ReportError(nTick, "Program Change not specified for Riffs");
        return;
    }
    if (mProgramSent) {
        return;
    }
    mProgramSent = 1;
    if (mBankSelect != 0 && mTrackType != kTrackTypeAxe && mTrackType != kTrackTypeScratch) {
        PlayMap *pMap = mBuilder->OnUnknownSlot8();
        const int nBar = pMap->FindStepIndex(nTick / Mid::MBT(kTicksPerBar).mTick);
        mRiff->AddMidiMsg(ClampMBT(nTick - mRiffStart.mTick).mTick,
                          kMidiControlChange,
                          kMidiBankSelectMsb,
                          0,
                          mChannel);
        mRiff->AddMidiMsg(ClampMBT(nTick - mRiffStart.mTick).mTick,
                          kMidiControlChange,
                          kMidiBankSelectLsb,
                          nBar,
                          mChannel);
    }
    mRiff->AddMidiMsg(
        ClampMBT(nTick - mRiffStart.mTick).mTick, kMidiProgramChange, mProgram, 0, mChannel);
}

// 0x001ea618
void LevelConverter::SyncRiff(int nTick) {
    int bOpenedHere = 0;
    if (mRiff == nullptr) {
        NextRiff();
        bOpenedHere = 1;
    }
    for (;;) {
        const int nPosition = CheckRiffPosition(nTick);
        if (nPosition == kRiffPositionInside) {
            return;
        }
        if (nPosition == kRiffPositionBefore) {
            ReportError(nTick, "No gem found for this midi event.");
            return;
        }
        if (bOpenedHere) {
            ReportError(mRiffStart.mTick, "Found a gem with no midi events");
        }
        NextRiff();
        bOpenedHere = 1;
    }
}
