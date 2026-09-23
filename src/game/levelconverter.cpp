#include "game/levelconverter.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "app/application.h"
#include "game/levelbuilder.h"
#include "game/playmap.h"
#include "game/riff.h"
#include "game/trackdata.h"
#include "mid/mbt.h"
#include "mid/midifilereader.h"
#include "msg/sustainnotemsg.h"
#include "os/hostmode.h"
#include "script/configquery.h"
#include "stream/hxdatachunkreader.h"
#include "stream/hxmemstream.h"

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

constexpr unsigned char kMidiNoteOff = 0x80;
constexpr unsigned char kMidiNoteOn = 0x90;
constexpr unsigned char kMidiPolyPressure = 0xa0;
constexpr unsigned char kMidiChannelPressure = 0xd0;
constexpr unsigned char kMidiPitchBend = 0xe0;

// The status class of a channel status byte.
constexpr unsigned char kMidiStatusClassMask = 0xf0;

// The controllers the handlers act on or reject.
constexpr unsigned char kControllerVolume = 7;
constexpr unsigned char kControllerExpression = 11;
constexpr unsigned char kControllerQuantization = 0x66;
constexpr unsigned char kControllerScoreThreshold = 0x68;
constexpr unsigned char kControllerErrorThreshold = 0x69;
constexpr unsigned char kControllerActiveness = 0x6a;
constexpr unsigned char kControllerGem = 0x6b;

// The quantisation values the quantisation controller accepts.
constexpr unsigned char kQuantizationWhole = 1;
constexpr unsigned char kQuantizationHalf = 2;
constexpr unsigned char kQuantizationQuarter = 4;
constexpr unsigned char kQuantizationEighth = 8;
constexpr unsigned char kQuantizationSixteenth = 16;

// The value mChannel has until the track supplies one.
constexpr unsigned char kNoChannel = 0xff;

// The position mRiffStart, mHarmonyStart, and mLastBankBar take at the start of a track.
constexpr int kUnsetTick = -1;

// The track index Convert() starts with, before the first track.
constexpr int kNoTrack = -1;

// Configuration codes Convert() and ParseTrackTypeString() query.
constexpr int kBankSelectDisableQuery = 0x3a1;
constexpr int kBankSelectQuery = 0x3a4;

// The play mode in which a vocal track is read as a catch track.
constexpr int kCatchVocalPlayMode = 1;

// Track names are `t`, a number, and a type from this position.
constexpr char kScoreTrackPrefix = 't';
constexpr unsigned kTrackTypePosition = 3;

// An instrument track name is a letter, a colon, and a display name from this position.
constexpr char kInstrumentSeparator = ':';
constexpr unsigned kDisplayNamePosition = 4;

// The `bg_` prefix's length.
constexpr unsigned kBackgroundPrefixLength = 3;

// TrackData::mInstrument values, by the letter that selects them.
enum Instrument {
    kInstrumentDrums = 0,
    kInstrumentBass = 1,
    kInstrumentSynth = 2,
    kInstrumentGuitar = 3,
    kInstrumentVocal = 4,
    kInstrumentFx = 5,
};

// The name ReportError() writes for the summary lines.
constexpr char kOverallFileName[] = "Overall File";

// The order the file reader delivers one position's events in, by status class.
enum EventRank {
    kRankNoteOff = 1,
    kRankControlChange = 2,
    kRankProgramChange = 3,
    kRankChannelPressure = 4,
    kRankPitchBend = 5,
    kRankPolyPressure = 6,
    kRankNoteOn = 7,
    kRankOther = 8,
};

inline int StatusRank(unsigned char nStatus) {
    switch (nStatus & kMidiStatusClassMask) {
    case kMidiNoteOff:
        return kRankNoteOff;
    case kMidiControlChange:
        return kRankControlChange;
    case kMidiProgramChange:
        return kRankProgramChange;
    case kMidiChannelPressure:
        return kRankChannelPressure;
    case kMidiPitchBend:
        return kRankPitchBend;
    case kMidiPolyPressure:
        return kRankPolyPressure;
    case kMidiNoteOn:
        return kRankNoteOn;
    default:
        return kRankOther;
    }
}

// 0x001e6450
bool CompareEventStatus(const Mid::FileReader::Event &left, const Mid::FileReader::Event &right) {
    const int nLeft = StatusRank(left.mStatus);
    const int nRight = StatusRank(right.mStatus);
    if (nLeft == nRight) {
        return false;
    }
    return nLeft < nRight;
}

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
// the three span collections and the harmony first, then the pending-event collection, then the
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

// 0x001e65e0
void LevelConverter::Convert(const char *pszPath,
                             void *pBuffer,
                             int nLength,
                             LevelBuilder *pBuilder) {
    g_bErrorLogPending = 1;

    const char *pszEnd = pszPath + strlen(pszPath);
    const char *pszDot = pszEnd;
    while (pszPath < pszDot && *pszDot != '.') {
        --pszDot;
    }
    const char *pszBase = pszDot;
    while (pszPath < pszBase && pszBase[-1] != '/' && pszBase[-1] != '\\') {
        --pszBase;
    }
    const int nBaseLength = pszDot - pszBase;
    strncpy(g_szErrorLogPath, pszBase, nBaseLength);
    g_szErrorLogPath[nBaseLength] = '\0';
    strcat(g_szErrorLogPath, ".err");

    mBuilder = pBuilder;
    mScoreTrack = 0;
    mBackingTrackCount = 0;
    mIntroTrackCount = 0;
    mUnknown38 = 0;
    mHasTempo = 0;
    if (QueryConfigFlag(kBankSelectDisableQuery)) {
        mBankSelect = 0;
    } else {
        mBankSelect = QueryConfigFlag(kBankSelectQuery);
    }
    mPlayMode = Application::shared()->GetPlayMode();
    mTrack = kNoTrack;
    mPath = pszPath;

    HxMemStream stream(pszPath, static_cast<char *>(pBuffer), nLength);
    stream.mSwapBytes = 1;
    HxDataChunkReader chunks(&stream, false);
    Mid::FileReader reader(&chunks, this);
    reader.mCompare = CompareEventStatus;
    reader.Read();
    mBuilder->PrepareTracks();
    FinishErrorLog();
}

// 0x001e6880
void LevelConverter::NewTrack(unsigned char nTrack) {
    mTrack = nTrack;
    mPairNotes = 0;
    mRiffTrack = 0;
    mHarmonyTrack = 0;
    mGemSpanTrack = 0;
    mPending.clear();
    mTrackType = kTrackTypeUnknown;
    mChannel = kNoChannel;
    mRiff = nullptr;
    mRiffIndex = 0;
    mRiffStart = Mid::MBT(kUnsetTick);
    mProgram = kNoProgram;
    mRiffOpened = 0;
    mHarmony = Harmony();
    mLastBankBar = kUnsetTick;
    mHarmonyStart = Mid::MBT(kUnsetTick);
}

// 0x001e6fc0
void LevelConverter::NoteOn(int nTick,
                            unsigned char nNote,
                            unsigned char nVelocity,
                            unsigned char nChannel) {
    CheckChannel(nChannel, nTick);
    if (mHarmonyTrack) {
        AddHarmonyNote(nTick, nNote);
        return;
    }
    if (mPairNotes) {
        PendingEvent event;
        memset(&event, 0, sizeof(event));
        event.mNote = nNote;
        event.mVelocity = nVelocity;
        event.mTick = nTick;
        mPending.push_back(event);
        return;
    }
    if (mRiffTrack) {
        SyncRiff(nTick);
        if (mRiff == nullptr) {
            ReportError(nTick, "No gem found for Note On");
            return;
        }
        mRiff->AddMidiMsg(
            ClampMBT(nTick - mRiffStart.mTick).mTick, kMidiNoteOn, nNote, nVelocity, nChannel);
        return;
    }
    mBuilder->AddEvent(nTick, kMidiNoteOn, nNote, nVelocity, nChannel);
}

// 0x001e7188
void LevelConverter::NoteOff(int nTick, unsigned char nNote, unsigned char nChannel) {
    CheckChannel(nChannel, nTick);
    if (mHarmonyTrack) {
        return;
    }
    if (mPairNotes) {
        int bFound = 0;
        for (auto it = mPending.begin(); it != mPending.end(); ++it) {
            if (it->mNote != nNote) {
                continue;
            }
            const Mid::MBT start(it->mTick);
            const unsigned char nVelocity = it->mVelocity;
            const Mid::MBT duration = ClampMBT(nTick - Mid::MBT(it->mTick).mTick);
            AddNote(start.mTick, nNote, nVelocity, duration.mTick, nChannel);
            mPending.erase(it);
            bFound = 1;
            break;
        }
        if (!bFound) {
            ReportError(nTick, "Found note-off, but no corresponding note-on.");
        }
        return;
    }
    if (mRiffTrack) {
        SyncRiff(nTick);
        if (mRiff == nullptr) {
            ReportError(nTick, "No gem found for Note Off");
            return;
        }
        mRiff->AddMidiMsg(
            ClampMBT(nTick - mRiffStart.mTick).mTick, kMidiNoteOff, nNote, 0, nChannel);
        return;
    }
    mBuilder->AddEvent(nTick, kMidiNoteOff, nNote, 0, nChannel);
}

// 0x001e7788
void LevelConverter::Controller(int nTick,
                                unsigned char nController,
                                unsigned char nValue,
                                unsigned char nChannel) {
    if (mHarmonyTrack || mGemSpanTrack) {
        ReportError(nTick, "CC message -- ignored in this track");
        return;
    }
    CheckChannel(nChannel, nTick);

    if (mRiffTrack) {
        if (nController == kControllerQuantization) {
            if (nValue == kQuantizationWhole || nValue == kQuantizationHalf ||
                nValue == kQuantizationQuarter || nValue == kQuantizationEighth ||
                nValue == kQuantizationSixteenth) {
                if (mUnknown90 == 0) {
                    mBuilder->SetQuant(nTick, nValue);
                }
            } else {
                ReportError(nTick, "Illegal value for Quantization CC");
            }
            return;
        }
        if (nController == kControllerVolume) {
            ReportError(nTick, "Volume CC -- ignored. Should be in data track");
            return;
        }
        if (nController == kControllerActiveness) {
            mBuilder->OnUnknownForwarder001ec580(nTick, nValue != 0);
            return;
        }
    } else if (nController == kControllerActiveness) {
        ReportError(nTick, "Activeness CC -- not supported here");
        return;
    }

    switch (nController) {
    case kControllerErrorThreshold:
        ReportError(nTick, "ErrorThreshold CC -- obsolete");
        return;
    case kControllerScoreThreshold:
        ReportError(nTick, "ScoreThreshold CC -- obsolete");
        return;
    case kControllerGem:
        ReportError(nTick, "Gem controller -- obsolete");
        return;
    case kControllerExpression:
        ReportError(nTick, "Expression CC -- not allowed. Use volume");
        return;
    default:
        break;
    }

    if (mRiffTrack) {
        SyncRiff(nTick);
        if (mRiff == nullptr || ClampMBT(nTick - mRiffStart.mTick).mTick < 0) {
            ReportError(nTick, "No gem found for Control Change");
            return;
        }
        mRiff->AddMidiMsg(ClampMBT(nTick - mRiffStart.mTick).mTick,
                          kMidiControlChange,
                          nController,
                          nValue,
                          nChannel);
        return;
    }
    mBuilder->AddEvent(nTick, kMidiControlChange, nController, nValue, nChannel);
}

// 0x001ea370
void LevelConverter::ProgramChange(int nTick, unsigned char nProgram, unsigned char nChannel) {
    if (mHarmonyTrack || mGemSpanTrack) {
        ReportError(nTick, "Program Change message -- ignored in this track");
        return;
    }
    CheckChannel(nChannel, nTick);
    if (mRiffTrack) {
        mProgram = nProgram;
        return;
    }
    mBuilder->AddEvent(nTick, kMidiProgramChange, nProgram, 0, nChannel);
}

// 0x001ea420
void LevelConverter::PitchBend(int nTick,
                               unsigned char nLow,
                               unsigned char nHigh,
                               unsigned char nChannel) {
    if (mHarmonyTrack || mGemSpanTrack) {
        ReportError(nTick, "Pitchbend message -- ignored in this track");
        return;
    }
    CheckChannel(nChannel, nTick);
    if (mRiffTrack) {
        SyncRiff(nTick);
        if (mRiff == nullptr) {
            ReportError(nTick, "No gem found for PitchBend");
            return;
        }
        mRiff->AddMidiMsg(
            ClampMBT(nTick - mRiffStart.mTick).mTick, kMidiPitchBend, nLow, nHigh, nChannel);
        return;
    }
    mBuilder->AddEvent(nTick, kMidiPitchBend, nLow, nHigh, nChannel);
}

// 0x001e6a30
void LevelConverter::EndTrack() {
    if (mHarmonyStart.mTick != Mid::MBT(kUnsetTick).mTick) {
        mBuilder->AddHarmony(mHarmonyStart.mTick, mHarmony);
    }
    if (mRiffTrack && !mRiffOpened) {
        mBuilder->OnUnknownForwarder001ec580(Mid::MBT(0).mTick, 0);
    }
    if (mRiff != nullptr && mTrackType == kTrackTypeAxe &&
        mRiff->mLength.mTick == Mid::MBT(0).mTick) {
        ReportError(Mid::MBT(0).mTick, "Length must be set for axe riffs.");
        return;
    }
    if (mPending.size() != 0) {
        ReportError(Mid::MBT(0).mTick, "Some Note-Ons were not matched by Note-Offs");
        return;
    }
    if (mRiffTrack) {
        for (int i = 0; i < kDifficultyCount; ++i) {
            mSpans[i].clear();
        }
    }
}

// 0x001e8318
void LevelConverter::ParseTrackTypeString(const char *pText) {
    if (mTrackType != kTrackTypeUnknown) {
        ReportError(Mid::MBT(0).mTick, "Track Type can only be set once");
        return;
    }
    const unsigned char nTrackCount = mBuilder->TrackCount();
    mTrackName = pText;
    HxStr name(mTrackName);
    std::transform(name.mStr, name.mStr + name.mLen, name.mStr, tolower);

    if (mTrack == 0) {
        mTrackType = kTrackTypeTempo;
    } else if (name == "control") {
        mTrackType = kTrackTypeControl;
    } else if (name == "intro") {
        mTrackType = kTrackTypeIntro;
    } else if (name.Mid(0, kBackgroundPrefixLength) == "bg_") {
        mTrackType = kTrackTypeBackground;
    } else if (name[0] == kScoreTrackPrefix) {
        const int nScoreTrack = atoi(pText + 1);
        if (nScoreTrack <= 0 || nTrackCount < nScoreTrack) {
            ReportError(Mid::MBT(0).mTick, "Track number invalid or out or range.");
            return;
        }
        mScoreTrack = nScoreTrack - 1;
        const HxStr type = name.Mid(kTrackTypePosition);
        if (type == "pitch") {
            mTrackType = kTrackTypePitch;
        } else if (type == "scratch") {
            mTrackType = kTrackTypeScratch;
        } else if (type == "axe") {
            mTrackType = kTrackTypeAxe;
        } else if (type == "catch") {
            mTrackType = kTrackTypeCatch;
        } else if (type == "vocal" && mPlayMode == kCatchVocalPlayMode) {
            mTrackType = kTrackTypeCatch;
        } else if (type == "vocal") {
            mTrackType = kTrackTypeVocal;
        } else if (type == "data") {
            mTrackType = kTrackTypeData;
        } else if (type == "harmony") {
            mTrackType = kTrackTypeHarmony;
        } else if (type == "ghost") {
            mTrackType = kTrackTypeGhost;
        } else if (type[1] == kInstrumentSeparator) {
            // Anything else leaves the type unrecognised without a report.
            mInstrument = kInstrumentDrums;
            mTrackType = kTrackTypeInstrument;
            mDisplayName = mTrackName.Mid(kDisplayNamePosition);
            switch (type[0]) {
            case 'b':
                mInstrument = kInstrumentBass;
                break;
            case 'd':
                mInstrument = kInstrumentDrums;
                break;
            case 'f':
                mInstrument = kInstrumentFx;
                break;
            case 'g':
                mInstrument = kInstrumentGuitar;
                break;
            case 's':
                mInstrument = kInstrumentSynth;
                break;
            case 'v':
                mInstrument = kInstrumentVocal;
                break;
            default:
                mTrackType = kTrackTypeUnknown;
                ReportError(Mid::MBT(0).mTick, "Unrecognized instrument letter-code.");
                break;
            }
        }
    } else {
        mTrackType = kTrackTypeUnknown;
        ReportError(Mid::MBT(0).mTick, "Unrecognized Track Type");
    }
    ApplyTrackType();
}

// 0x001e7ac0
void LevelConverter::AddHarmonyNote(int nTick, unsigned char nNote) {
    if (mHarmonyStart.mTick != nTick) {
        if (mHarmonyStart.mTick != Mid::MBT(kUnsetTick).mTick) {
            mBuilder->AddHarmony(mHarmonyStart.mTick, mHarmony);
        }
        mHarmony = Harmony();
        mHarmonyStart.mTick = nTick;
    }
    mHarmony.AddNote(nNote);
}

// 0x001e8120
void LevelConverter::CheckChannel(unsigned char nChannel, int nTick) {
    if (mTrackType == kTrackTypeUnknown) {
        ReportError(nTick, "Track Type not recognized for this track.");
        return;
    }
    if (mGemSpanTrack) {
        return;
    }
    if (mChannel == kNoChannel) {
        mChannel = nChannel;
        mBuilder->SetChannel(nChannel);
        return;
    }
    if (nChannel != mChannel) {
        ReportError(nTick, "Channel cannot change mid-track.");
    }
}

// 0x001e8af0
void LevelConverter::FinishErrorLog() {
    mTrackName = kOverallFileName;
    mTrack = 0;
    if (!mHasTempo) {
        ReportError(Mid::MBT(0).mTick, "Tempo Marker not found.");
        return;
    }
    const char *pszPath = mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString;
    if (g_bErrorLogPending) {
        if (MidiErrorLogEnabled()) {
            g_pErrorLog = fopen(g_szErrorLogPath, "w");
            fprintf(g_pErrorLog, "%s is free of Errors, You Rock.\n", pszPath);
            fclose(g_pErrorLog);
        }
    } else if (MidiErrorLogEnabled()) {
        fprintf(g_pErrorLog, "%s has Errors, you fail to rock.\n", pszPath);
        fclose(g_pErrorLog);
    }
    g_bErrorLogPending = 1;
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
        mPairNotes = 1;
        break;
    case kTrackTypePitch:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mBuilder->SetKind(kTrackModeRiff);
        mBuilder->SetInstrument(mInstrument, mDisplayName);
        mRiffTrack = 1;
        mPairNotes = 1;
        break;
    case kTrackTypeScratch:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mBuilder->SetKind(kTrackModeScratch);
        mBuilder->SetInstrument(mInstrument, mDisplayName);
        mRiffTrack = 1;
        mPairNotes = 1;
        break;
    case kTrackTypeVocal:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mBuilder->SetKind(kTrackModeVocal);
        mBuilder->SetInstrument(mInstrument, mDisplayName);
        mRiffTrack = 0;
        mPairNotes = 1;
        break;
    case kTrackTypeData:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mPairNotes = 0;
        mRiffTrack = 0;
        break;
    case kTrackTypeHarmony:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mPairNotes = 0;
        mHarmonyTrack = 1;
        mRiffTrack = 0;
        break;
    case kTrackTypeCatch:
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mBuilder->SetKind(kTrackModeCatch);
        mBuilder->SetInstrument(mInstrument, mDisplayName);
        mRiffTrack = 1;
        mPairNotes = 1;
        mDifficulty = QueryConfigValue(kGemDifficultyQuery);
        if (static_cast<unsigned>(mDifficulty) >= static_cast<unsigned>(kDifficultyCount)) {
            // The report skips the span cursor below.
            ReportError(Mid::MBT(0).mTick, "Gem difficulty must be in range [0,2]");
            return;
        }
        break;
    case kTrackTypeBackground:
        mBuilder->SelectTrack(kLevelTrackBacking, mBackingTrackCount);
        mPairNotes = 1;
        mRiffTrack = 0;
        ++mBackingTrackCount;
        break;
    case kTrackTypeIntro:
        mBuilder->SelectTrack(kLevelTrackIntro, mIntroTrackCount);
        mPairNotes = 1;
        mRiffTrack = 0;
        ++mIntroTrackCount;
        break;
    case kTrackTypeControl:
        mBuilder->SelectTrack(kLevelTrackOwn, 0);
        mPairNotes = 0;
        mRiffTrack = 0;
        break;
    case kTrackTypeInstrument:
        for (int i = 0; i < kDifficultyCount; ++i) {
            mSpans[i].clear();
        }
        mBuilder->SelectTrack(kLevelTrackNone, 0);
        mRiffTrack = 0;
        mGemSpanTrack = 1;
        mPairNotes = 1;
        break;
    case kTrackTypeGhost:
        for (int i = 0; i < kDifficultyCount; ++i) {
            mSpans[i].clear();
        }
        mBuilder->SelectTrack(kLevelTrackScore, mScoreTrack);
        mGemSpanTrack = 1;
        mPairNotes = 1;
        mRiffTrack = 0;
        mGhostGems = mPlayMode == kGhostPlayMode;
        break;
    default:
        mPairNotes = 0;
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
    mRiffOpened = 1;
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
