#pragma once

#include <vector>

#include "game/harmony.h"
#include "mid/mbt.h"
#include "mid/receiver.h"
#include "os/hxstr.h"

class Application;
class LevelBuilder;
class Riff;

/**
 * Kind of a MIDI track, as LevelConverter's track-name parser reports it.
 *
 * The values are the cases of the jump table at `0x007e7050`. Every name below is the track-name
 * text the parser matches. The two exceptions are the tempo track (track 0, whatever its name) and
 * the instrument track (a name with a colon as its second character).
 */
enum LevelConverterTrackType {
    kTrackTypeUnknown = 0,     /*!< A name the parser rejected. */
    kTrackTypeTempo = 1,       /*!< Track 0. */
    kTrackTypeAxe = 2,         /*!< `axe`. */
    kTrackTypePitch = 3,       /*!< `pitch`. */
    kTrackTypeScratch = 4,     /*!< `scratch`. */
    kTrackTypeVocal = 5,       /*!< `vocal`, outside play mode 1. */
    kTrackTypeData = 6,        /*!< `data`. */
    kTrackTypeHarmony = 7,     /*!< `harmony`. */
    kTrackTypeCatch = 8,       /*!< `catch`, and `vocal` in play mode 1. */
    kTrackTypeBackground = 9,  /*!< A name beginning `bg_`. */
    kTrackTypeIntro = 10,      /*!< `intro`. */
    kTrackTypeControl = 11,    /*!< `control`. */
    kTrackTypeInstrument = 12, /*!< A letter, a colon, and a display name. */
    kTrackTypeGhost = 13,      /*!< `ghost`. */
};

/**
 * Converter that reads a Standard MIDI File and fills a LevelBuilder from its events.
 *
 * `14LevelConverter` in the RTTI descriptor at `0x008ef230`, with Mid::Receiver as its one base at
 * offset 0. Its table is at `0x007e76b8` and has sixteen entries with a zero terminator at index
 * 16, the same length as the base table, so the class adds no virtual. It overrides slots 2
 * through 10 and inherits slots 11 through 15.
 *
 * Slot 11 is worth stating separately. The derived table records `0x001ea360` where the base
 * records `0x001ea2c0`, and both are the same two instructions. This toolchain re-emits an inline
 * empty body into every translation unit that needs one, so a unique address for an empty body is
 * no evidence of an override. The slot is recorded as inherited and nothing is declared for it.
 *
 * The object is at least 0xcc bytes, and several members are unrecovered. Every member whose
 * purpose is undetermined retains its offset.
 *
 * Convert() is the entry point. It stores the builder, extracts the file's base name into a global
 * buffer at `0x00891a38`, reads two display-configuration codes, builds three stack objects for
 * the file, the byte stream, and the Standard MIDI File reader, hands itself to the reader as the
 * event sink, and runs it. The three stack objects are an HxMemStream with HxStream::mSwapBytes set
 * to 1, an HxDataChunkReader over the whole stream, and a Mid::FileReader. The file-static
 * comparator at `0x001e6450` is stored in Mid::FileReader::mCompare before Mid::FileReader::Read()
 * runs. The comparator ranks each event by its status class (note off first, note on last).
 *
 * The five event handlers all follow one shape. They report through ReportError() on the error
 * paths, check the channel through CheckChannel(), and then forward to one of two sinks. With
 * mRiffTrack clear the event goes to LevelBuilder::AddEvent(). With it set the event goes to
 * Riff::AddMidiMsg() on mRiff instead, with the position rebased against mRiffStart and saturated
 * to Mid::MBT's bounds.
 *
 * Every method name below that is not a Mid::Receiver override is inferred from its body.
 */
class LevelConverter : public Mid::Receiver {
    // GrooveWorld::FinishLoad() at 0x00194d00 sets mUnknown90 on its local converter directly.
    friend class GrooveWorld;

public:
    /**
     * One note on waiting for its note off.
     *
     * `PendingEvent` is a placeholder for the name. NoteOn() zeroes the eight bytes and fills the
     * three fields, and NoteOff() pairs the first entry with the same note and erases it.
     */
    struct PendingEvent {
        unsigned char mNote;     /*!< The note number. */
        unsigned char mVelocity; /*!< The note-on velocity. */
        int mTick;               /*!< The note-on position, in MIDI ticks. */
    };

    /**
     * One gem of a gem track, as the three span collections store it.
     *
     * `Span` is a placeholder for the name. The size comes from the stride the destructor steps
     * each collection by. AddGemSpan() appends one per note of a gem track, and NextRiff() opens a
     * riff for each in turn.
     */
    struct Span {
        Mid::MBT mStart;  /*!< The gem's song position. */
        int mGem;         /*!< The gem, 0 through 2, from a C, an E, or a G. */
        Mid::MBT mLength; /*!< The note's duration, which becomes the riff's length. */
    };

    /**
     * @ghidraAddress 0x001e6278
     */
    LevelConverter();

    /**
     * @ghidraAddress 0x001e9ee0
     */
    virtual ~LevelConverter();

    /**
     * Read one Standard MIDI File and fill a builder from it.
     *
     * The error log path is the file's base name with `.err` appended. The file is read through
     * an HxMemStream with byte swapping on, an HxDataChunkReader, and a Mid::FileReader whose
     * events of one position are ordered by status class (note off first, note on last).
     * LevelBuilder::PrepareTracks() and FinishErrorLog() run after the file is read.
     *
     * @param pszPath The file to read.
     * @param pBuffer The file's contents, forwarded to the HxMemStream constructor.
     *                GrooveWorld::FinishLoad() passes the buffer its asynchronous read filled.
     * @param nLength The length of pBuffer in bytes, forwarded to the same object.
     * @param pBuilder The builder the events are appended to.
     * @ghidraAddress 0x001e65e0
     */
    void Convert(const char *pszPath, void *pBuffer, int nLength, LevelBuilder *pBuilder);

    /**
     * Reset the per-track state and begin one track.
     *
     * The track type, the routing flags, the pending notes, the riff, the channel, the program,
     * and the harmony all return to their unset values.
     *
     * @param nTrack The track index.
     * @ghidraAddress 0x001e6880
     */
    virtual void NewTrack(unsigned char nTrack);

    /**
     * Receive a note on and forward it under MIDI status 0x90.
     *
     * A harmony track adds the note to the harmony through AddHarmonyNote(). A track that pairs
     * notes stores the note on in mPending instead of forwarding it.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nNote The note number.
     * @param nVelocity The velocity.
     * @param nChannel The channel.
     * @ghidraAddress 0x001e6fc0
     */
    virtual void
    NoteOn(int nTick, unsigned char nNote, unsigned char nVelocity, unsigned char nChannel);

    /**
     * Receive a note off and forward it under MIDI status 0x80.
     *
     * A harmony track ignores it. A track that pairs notes hands the first pending note on with
     * the same note to AddNote() with the duration between the two, and reports a note off with no
     * partner. Otherwise the forwarded call passes zero where the note-on path passes a velocity.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nNote The note number.
     * @param nChannel The channel.
     * @ghidraAddress 0x001e7188
     */
    virtual void NoteOff(int nTick, unsigned char nNote, unsigned char nChannel);

    /**
     * Receive a controller change and forward it under MIDI status 0xb0.
     *
     * Harmony and gem-span tracks reject it. On a riff track, controller 0x66 sets the bar
     * quantisation (1, 2, 4, 8, or 16) unless mUnknown90 is set, controller 7 (volume) is
     * rejected, and controller 0x6a forwards whether its value is non-zero through
     * LevelBuilder::OnUnknownForwarder001ec580(). Controller 0x6a is rejected on every other
     * track, and controllers 0x68, 0x69, 0x6b, and 11 (expression) are rejected everywhere.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nController The controller number.
     * @param nValue The value.
     * @param nChannel The channel.
     * @ghidraAddress 0x001e7788
     */
    virtual void
    Controller(int nTick, unsigned char nController, unsigned char nValue, unsigned char nChannel);

    /**
     * Receive a program change and forward it under MIDI status 0xc0.
     *
     * Harmony and gem-span tracks reject it. A riff track stores the program in mProgram and sends
     * nothing, and EmitRiffProgram() sends it with each riff instead.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nProgram The program number.
     * @param nChannel The channel.
     * @ghidraAddress 0x001ea370
     */
    virtual void ProgramChange(int nTick, unsigned char nProgram, unsigned char nChannel);

    /**
     * Receive a pitch bend and forward it under MIDI status 0xe0.
     *
     * Harmony and gem-span tracks reject it. Otherwise it takes the shared shape.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nLow The low seven bits of the bend.
     * @param nHigh The high seven bits of the bend.
     * @param nChannel The channel.
     * @ghidraAddress 0x001ea420
     */
    virtual void
    PitchBend(int nTick, unsigned char nLow, unsigned char nHigh, unsigned char nChannel);

    /**
     * Hand a tempo meta event to the builder and record that the file has a tempo.
     *
     * Both parameters pass through to LevelBuilder::SetTempo() in the registers they arrive in.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nMicrosecondsPerQuarter The tempo.
     * @ghidraAddress 0x001ea570
     */
    virtual void Tempo(int nTick, int nMicrosecondsPerQuarter);

    /**
     * Parse a track name, and ignore every other text event.
     *
     * A type other than 3 and a position other than 0 are both ignored, and 3 is the MIDI
     * track-name meta type. The discarded IsFiniteMBT(0) ahead of the test is the shape of an
     * assertion compiled without its report.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param pText The text.
     * @param nType The meta type.
     * @ghidraAddress 0x001ea5a0
     */
    virtual void TextEvent(int nTick, const char *pText, unsigned char nType);

    /**
     * Finish the track NewTrack() began.
     *
     * A harmony that was started goes to the builder. A riff track that opened no riff forwards
     * two zeroes through LevelBuilder::OnUnknownForwarder001ec580(). An axe riff with no length and
     * a note on with no note off are both reported, and otherwise a riff track empties the three
     * span collections.
     *
     * @ghidraAddress 0x001e6a30
     */
    virtual void EndTrack();

private:
    /**
     * Read a track name and set the track's kind from it.
     *
     * Track 0 is the tempo track whatever its name. Otherwise the lowered name is matched against
     * `control`, `intro`, and the `bg_` prefix, and then read as `t`, a score track number from 1,
     * and a type (`pitch`, `scratch`, `axe`, `catch`, `vocal`, `data`, `harmony`, or `ghost`), or
     * as an instrument letter (`d`, `b`, `s`, `g`, `v`, or `f`), a colon, and a display name. A
     * second name, an unknown letter, and a name that does not begin with `t` are reported. Every
     * path but a second name and a bad track number ends in ApplyTrackType().
     *
     * @param pText The track name.
     * @ghidraAddress 0x001e8318
     */
    void ParseTrackTypeString(const char *pText);

    /**
     * Select the builder's current track and the event routing for mTrackType.
     *
     * The three score kinds that play riffs (axe, pitch, and scratch) route events into riffs.
     * A catch track does too, after reading the gem difficulty from configuration code 0x38a. A
     * vocal track takes the kind and the instrument without the riff routing. A background or an
     * intro track takes the next slot of its collection, and a control track the builder's own
     * track. The data and harmony tracks write to the score track without a kind, and the two
     * gem-span kinds empty the three span collections. Every path but an out-of-range difficulty
     * finishes by pointing mNextSpan at the start of the difficulty's collection. The title is
     * inferred.
     *
     * @ghidraAddress 0x001e6bd0
     */
    void ApplyTrackType();

    /**
     * Receive one complete note, from its note on to its note off.
     *
     * A gem-span track records the note through AddGemSpan(). A riff track adds the note to the
     * current riff, rebased against the riff's start, after SyncRiff() and EmitRiffProgram(). An
     * axe note of a bar or longer is instead added at the riff's start as a SustainNoteMsg and a
     * note one tick longer than its duration. Any other track sends the note to the builder, first
     * sending a bank select for the bar when configuration code 0x3a4 enabled one and the bar has
     * changed. The title is inferred.
     *
     * @param nTick The note's song position, in MIDI ticks.
     * @param nNote The note number.
     * @param nVelocity The velocity.
     * @param nDuration The note's duration, in MIDI ticks.
     * @param nChannel The channel.
     * @ghidraAddress 0x001e7420
     */
    void AddNote(int nTick,
                 unsigned char nNote,
                 unsigned char nVelocity,
                 int nDuration,
                 unsigned char nChannel);

    /**
     * Record one gem of a gem-span track.
     *
     * The note's octave, counted from 5, selects the difficulty, and its pitch class selects the
     * gem (C, E, or G). A gem that does not start after the difficulty's last gem is rejected. A
     * ghost track at the third difficulty also sends the gem straight to the builder without a
     * riff. The title is inferred.
     *
     * @param nTick The gem's song position, in MIDI ticks.
     * @param nNote The note number.
     * @param nDuration The note's duration, in MIDI ticks.
     * @ghidraAddress 0x001e7c20
     */
    void AddGemSpan(int nTick, unsigned char nNote, int nDuration);

    /**
     * Place a song position against the current riff.
     *
     * The title is inferred.
     *
     * @param nTick The song position, in MIDI ticks.
     * @return 1 with no riff open or before the riff's start, -1 at or after the next gem, and 0
     *         inside the riff.
     * @ghidraAddress 0x001e7e00
     */
    int CheckRiffPosition(int nTick);

    /**
     * Open a riff for the next gem of the selected difficulty and hand it to the builder.
     *
     * A riff of an axe, pitch, or scratch track joins the riff set the last gem 0 began, at the
     * next index of that set, and more than three riffs in one set are reported. Any other track
     * opens a riff per gem through LevelBuilder::AddGem(). The title is inferred.
     *
     * @ghidraAddress 0x001e7eb0
     */
    void NextRiff();

    /**
     * Send the track's program change to the current riff once per riff.
     *
     * A bank select for the bar precedes it when configuration code 0x3a4 enabled one, except on
     * an axe or a scratch track. A track with no program change is reported instead. The title is
     * inferred.
     *
     * @param nTick The song position, in MIDI ticks.
     * @ghidraAddress 0x001e8cb8
     */
    void EmitRiffProgram(int nTick);

    /**
     * Advance to the riff a song position falls in.
     *
     * A riff that was opened and passed without receiving an event is reported, and so is a
     * position that falls before the riff it arrives at. The title is inferred.
     *
     * @param nTick The song position, in MIDI ticks.
     * @ghidraAddress 0x001ea618
     */
    void SyncRiff(int nTick);

    /**
     * Check an event's channel against the track's.
     *
     * A track with no type is reported. A gem-span track does not check. The first event of a
     * track sets mChannel and the builder's channel, and a later event on another channel is
     * reported. The title is inferred.
     *
     * @param nChannel The event's channel.
     * @param nTick The event position, in MIDI ticks.
     * @ghidraAddress 0x001e8120
     */
    void CheckChannel(unsigned char nChannel, int nTick);

    /**
     * Add one note to the harmony at a song position.
     *
     * A position other than mHarmonyStart first hands the harmony built so far to the builder,
     * unless none was started, and begins an empty one there. The title is inferred.
     *
     * @param nTick The note's song position, in MIDI ticks.
     * @param nNote The note number.
     * @ghidraAddress 0x001e7ac0
     */
    void AddHarmonyNote(int nTick, unsigned char nNote);

    /**
     * Finish the conversion's error log.
     *
     * A file with no tempo event is reported. Otherwise a conversion that reported nothing writes
     * a line that the file is free of errors to a freshly opened log, and one that reported writes
     * a line that it has errors, and the log is closed. The title is inferred.
     *
     * @ghidraAddress 0x001e8af0
     */
    void FinishErrorLog();

    /**
     * Append one line to the conversion's error log.
     *
     * Inline. The first report of a conversion opens the log, the file whose path Convert() builds
     * from the MIDI file's base name. Each report writes the track index, the track name, the
     * position as bar, beat, and tick, and the message. Both steps are skipped unless
     * MidiErrorLogEnabled() reports the boot option set. The retail configuration clears it. The
     * out-of-line copy is the address below.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pszMessage The message.
     * @ghidraAddress 0x001ea6e0
     */
    void ReportError(int nTick, const char *pszMessage);

    /** The number of gem difficulties, one span collection each. */
    static constexpr int kDifficultyCount = 3;

    HxStr mPath; // +0x04, assigned from Convert's path argument
    // A LevelConverterTrackType, set by ParseTrackTypeString().
    int mTrackType;         // +0x0c
    int mTrack;             // +0x10, the track index, set to -1 by Convert
    unsigned char mChannel; // +0x14, the MIDI channel events are sent on, 0xff at track start
    HxStr mTrackName;       // +0x18, the name ReportError() writes
    int mInstrument;        // +0x20
    HxStr mDisplayName;     // +0x24, the name LevelBuilder::SetInstrument() receives
    int mScoreTrack;        // +0x2c, the score track a named track writes to
    int mBackingTrackCount; // +0x30, cleared by Convert
    int mIntroTrackCount;   // +0x34, cleared by Convert
    int mUnknown38;         // +0x38, cleared by Convert
    // Set, a note on waits in mPending for its note off and the pair goes to AddNote().
    int mPairNotes; // +0x3c
    // Set, a track's events go into the current riff. Clear, they go to the builder.
    int mRiffTrack;         // +0x40
    int mHarmonyTrack;      // +0x44
    int mGemSpanTrack;      // +0x48
    int mGhostGems;         // +0x4c, a ghost track whose third-difficulty gems go to the builder
    LevelBuilder *mBuilder; // +0x50
    Riff *mRiff;            // +0x54
    int mRiffIndex;         // +0x58, the current riff's index in its riff set
    // The position riff events are rebased against. -1 at the start of each track.
    Mid::MBT mRiffStart;                // +0x5c
    Mid::MBT mRiffSetStart;             // +0x60
    unsigned char mProgram;             // +0x64, the withheld program number, 0xff at track start
    int mProgramSent;                   // +0x68
    int mRiffOpened;                    // +0x6c, set when a riff opens
    std::vector<PendingEvent> mPending; // +0x70
    // The harmony the converter builds note by note through Harmony::AddNote() (0x001e7b90) and
    // hands to LevelBuilder::AddHarmony() (0x001e6a60, 0x001e7b04). The constructor, NewTrack(),
    // and 0x001e7ac0 call its implicit default constructor, emitted at 0x001ea1e8.
    Harmony mHarmony;       // +0x7c
    Mid::MBT mHarmonyStart; // +0x88, the position mHarmony starts at, -1 before the first note
    int mHasTempo;          // +0x8c, cleared by Convert and set to 1 by Tempo
    int mUnknown90;         // +0x90
    // The gem difficulty, from configuration code 0x38a on a catch track.
    int mDifficulty;                            // +0x94
    std::vector<Span> mSpans[kDifficultyCount]; // +0x98
    std::vector<Span>::iterator mNextSpan;      // +0xbc
    // Set by Convert from configuration code 0x3a4, and to zero when code 0x3a1 reports non-zero.
    int mBankSelect;  // +0xc0
    int mLastBankBar; // +0xc4, set to -1 by NewTrack
    // Set by Convert from Globals::GetPlayMode() (0x00118dd8).
    int mPlayMode; // +0xc8
};
