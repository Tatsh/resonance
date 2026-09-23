#pragma once

#include <vector>

#include "mid/mbt.h"

class HxDataChunkReader;
class HxStream;

namespace Mid {

class Receiver;

/**
 * Standard MIDI File reader that delivers each event to a Mid::Receiver.
 *
 * The class is not polymorphic and has no RTTI. Its name is inferred from the Mid namespace of its
 * receiver and the `MThd` and `MTrk` chunks it reads. LevelConverter::Convert() is the one user.
 * It builds the reader on the stack over an HxDataChunkReader, sets mCompare, and calls Read().
 *
 * Event positions are rescaled from the file's division to kTargetDivision ticks per quarter note.
 * With mCompare unset, every channel event goes straight to the receiver. With mCompare set, the
 * reader collects the channel events of one position in mPending, sorts them with mCompare when the
 * position changes or the track ends, and then delivers them in sorted order.
 *
 * Only format 1 and format 2 files have their tracks read. A format 0 file delivers no event.
 *
 * The destructor at `0x003d6450` is compiler-generated and destroys mPending.
 *
 * Every member is public, because LevelConverter stores mCompare directly and the image exposes no
 * accessor.
 */
class FileReader {
public:
    /** Ticks per quarter note of every position delivered to the receiver. */
    static constexpr short kTargetDivision = 480;

    /**
     * One channel event waiting in mPending. Three bytes, with no padding.
     */
    struct Event {
        unsigned char mStatus; /*!< The status byte, with the channel in its low four bits. */
        unsigned char mData1;  /*!< The first data byte. */
        unsigned char mData2;  /*!< The second data byte, or 0 for a one-byte event. */
    };

    /** Ordering mPending is sorted by. The result is non-zero when the first event sorts first. */
    typedef bool (*EventCompare)(const Event &left, const Event &right);

    /**
     * Build a reader over the chunks of a file.
     *
     * @param pReader The chunk reader positioned at the start of the file.
     * @param pReceiver The sink for every event.
     * @ghidraAddress 0x003d4a68
     */
    FileReader(HxDataChunkReader *pReader, Receiver *pReceiver);

    /**
     * Read every chunk of the file.
     *
     * @ghidraAddress 0x003d6528
     */
    void Read();

    /**
     * Read the next chunk.
     *
     * An `MThd` chunk fills the header fields and an `MTrk` chunk is read as a track. A chunk with
     * any other name is skipped. At the end of the file EndOfFile() is called.
     *
     * @return Whether a chunk was read.
     * @ghidraAddress 0x003d4ac8
     */
    bool ReadChunk();

    /**
     * Report the end of the file to the receiver's AllDone().
     *
     * @ghidraAddress 0x003d65a8
     */
    void EndOfFile();

    /**
     * Read the format, track count, and division of an `MThd` chunk.
     *
     * @param stream The chunk payload.
     * @ghidraAddress 0x003d6558
     */
    void ReadHeader(HxStream &stream);

    /**
     * Read one `MTrk` chunk when the file format has one track per chunk.
     *
     * @param stream The chunk payload.
     * @ghidraAddress 0x003d65d8
     */
    void ReadTrackChunk(HxStream &stream);

    /**
     * Begin a track at the receiver and read its events until the end-of-track meta event.
     *
     * @param stream The chunk payload.
     * @ghidraAddress 0x003d6610
     */
    void ReadTrack(HxStream &stream);

    /**
     * Read one delta time and event.
     *
     * A data byte in the status position repeats the running status. A note on with a velocity of
     * 0 becomes a note off.
     *
     * @param stream The chunk payload.
     * @ghidraAddress 0x003d4c10
     */
    void ReadEvent(HxStream &stream);

    /**
     * Skip a system-exclusive event or read a meta event, depending on mRunningStatus.
     *
     * @param stream The chunk payload, positioned after the status byte.
     * @ghidraAddress 0x003d4df0
     */
    void ReadSystemEvent(HxStream &stream);

    /**
     * Read one meta event and deliver end of track, tempo, and the seven text types.
     *
     * The stream is left after the event's data whatever the type.
     *
     * @param nType The meta type.
     * @param stream The chunk payload, positioned at the length.
     * @ghidraAddress 0x003d4f00
     */
    void ReadMeta(unsigned char nType, HxStream &stream);

    /**
     * Deliver one channel event to the receiver slot its status selects.
     *
     * Polyphonic and channel pressure are dropped.
     *
     * @param tick The position.
     * @param nStatus The status byte.
     * @param nData1 The first data byte.
     * @param nData2 The second data byte.
     * @ghidraAddress 0x003d5260
     */
    void Dispatch(MBT tick, unsigned char nStatus, unsigned char nData1, unsigned char nData2);

    /**
     * Deliver a channel event, or collect it in mPending when mCompare is set.
     *
     * A position different from mPendingTick flushes mPending first.
     *
     * @param tick The position.
     * @param nStatus The status byte.
     * @param nData1 The first data byte.
     * @param nData2 The second data byte.
     * @ghidraAddress 0x003d66a8
     */
    void QueueEvent(MBT tick, unsigned char nStatus, unsigned char nData1, unsigned char nData2);

    /**
     * Sort mPending with mCompare, deliver every event at mPendingTick, and empty it.
     *
     * @ghidraAddress 0x003d5140
     */
    void Flush();

    short mFormat;                /*!< The file format, or -1 before the header. */
    short mTrackCount;            /*!< The track count from the header. */
    short mDivision;              /*!< The file's ticks per quarter note. */
    short mTargetDivision;        /*!< Always kTargetDivision. */
    int mTrack;                   /*!< Index of the track being read. */
    Receiver *mReceiver;          /*!< The event sink. */
    int mTick;                    /*!< Position in file ticks within the track. */
    unsigned char mRunningStatus; /*!< The last status byte, or 0 after a system event. */
    int mTrackDone;               /*!< Non-zero once the end-of-track event is read. */
    HxDataChunkReader *mReader;   /*!< The chunk reader over the file. */
    std::vector<Event> mPending;  /*!< Events collected at mPendingTick. */
    MBT mPendingTick;             /*!< Position of every event in mPending. */
    EventCompare mCompare;        /*!< Ordering for mPending, or null to deliver directly. */
};

} // namespace Mid
