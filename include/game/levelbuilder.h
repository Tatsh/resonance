#pragma once

#include <iostream>
#include <vector>

#include "app/attachment.h"
#include "game/leveldata.h"

class HxStr;
class Riff;
struct Harmony;

/**
 * Collection LevelBuilder::SelectTrack() takes the current track from.
 *
 * The values are the cases of the jump table at `0x007e7940`. The three collection names are the
 * labels LevelBuilder::Print() writes ahead of each track.
 */
enum LevelTrackKind {
    kLevelTrackNone = 0,    /*!< Clear the current track. */
    kLevelTrackBacking = 1, /*!< A track of mBackingTracks, labelled `Backing Track#`. */
    kLevelTrackIntro = 2,   /*!< A track of mIntroTracks, labelled `Intro Track#`. */
    kLevelTrackScore = 3,   /*!< A track of mTracks, labelled `Score Track#`. */
    kLevelTrackOwn = 4,     /*!< The one track the builder manages outside the collections. */
};

/**
 * Accumulator a MIDI conversion fills, and the one implementation of LevelData.
 *
 * `12LevelBuilder` in the RTTI descriptor at `0x008efc40`, with LevelData as its one base at
 * offset 0. Its table is at `0x007e79f8` and has ten entries with a zero terminator at index 10,
 * the same length as the base table, so the class adds no virtual. It implements all eight of
 * LevelData's pure slots and supplies the destructor.
 *
 * LevelConverter drives it. LevelConverter::Convert() stores the builder at its own `+0x50`, each
 * MIDI event handler forwards to AddEvent() with the status byte for the event, and
 * LevelConverter::Tempo() forwards to SetTempo().
 *
 * The object is at least 0x38 bytes. Three vectors of pointers occupy `+0x04` through `+0x27`, and
 * the destructor clears each one with a std::for_each over the deleting function at `0x001ec328`,
 * so all three manage their elements. That function deletes a TrackData, which is what fixes the
 * element type of all three. The two scalars at `+0x30` and `+0x34` are managed as well,
 * the first through Attachment::Release() and the second through its own table slot 1.
 *
 * The forwarding members below all reach the object at `+0x2c`, which the destructor does not
 * release. That object is the one the conversion appends to, and its routines sit in the same
 * address range as TrackData's own destructor and appenders, so it is a TrackData.
 *
 * Every method name here is inferred from the body behind it.
 */
class LevelBuilder : public LevelData {
public:
    /**
     * Construct an empty builder.
     *
     * The body is not written. It creates the tempo map at mUnknown30 at 500000 microseconds per
     * quarter note and the PlayMapLinear at mUnknown34, and it sizes the track collections by
     * nTrackCount with unsigned comparisons. GrooveWorld::StartLoad() passes the configuration
     * value the query at `0x00509110` reports for the identifier 0x384.
     *
     * @param nTrackCount The number of tracks to prepare for.
     * @ghidraAddress 0x001ea838
     */
    explicit LevelBuilder(unsigned nTrackCount);

    /**
     * @ghidraAddress 0x001eafe8
     */
    virtual ~LevelBuilder();

    /**
     * @ghidraAddress 0x001ec430
     */
    virtual int TrackCount();

    /**
     * @ghidraAddress 0x001ec448
     */
    virtual int BackingTrackCount();

    /**
     * @ghidraAddress 0x001ec6d0
     */
    virtual TrackData *OwnTrack();

    /**
     * @ghidraAddress 0x001ec6f0
     */
    virtual TrackData *TrackAt(int nIndex);

    /**
     * @ghidraAddress 0x001ec708
     */
    virtual TrackData *BackingTrackAt(int nIndex);

    /**
     * @ghidraAddress 0x001ec478
     */
    virtual Sch::TempoMap *OnUnknownSlot7();

    /**
     * @ghidraAddress 0x001ec480
     */
    virtual PlayMap *OnUnknownSlot8();

    /**
     * Call the play map's slot 8 and discard the result.
     *
     * @ghidraAddress 0x001ec738
     */
    virtual void OnUnknownSlot9();

    /**
     * Make one track the current track, creating it on first use.
     *
     * A score track must already exist. A backing or intro collection grows to include nIndex.
     * An empty slot, like an absent own track, receives a new TrackData with the index -1 over the
     * play map at mUnknown34. The title is inferred.
     *
     * @param nKind The collection, a LevelTrackKind.
     * @param nIndex The position in the collection. The none and own kinds do not read it.
     * @ghidraAddress 0x001eb200
     */
    void SelectTrack(int nKind, int nIndex);

    /**
     * Write every track of the three collections to a diagnostic stream.
     *
     * Each track is introduced by its collection's label, its position, and a colon. The routine
     * has no caller in the image.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001eb4a8
     */
    void Print(std::ostream &stream);

    /**
     * Forward a bar count to the play map through its slot 3.
     *
     * GrooveWorld's draw pass at `0x0018cd44` passes zero. The title is inferred.
     *
     * @param nBarCount The value stored in the play map's bar count.
     * @ghidraAddress 0x001ec498
     */
    void SetBarCount(int nBarCount);

    /**
     * Return one intro track.
     *
     * The index is not tested against the collection. GrooveWorld's draw pass at `0x0018d22c` is
     * the caller. The title is inferred.
     *
     * @param nIndex The position in mIntroTracks.
     * @return The track, or null for a slot SelectTrack() passed over.
     * @ghidraAddress 0x001ec720
     */
    TrackData *IntroTrackAt(int nIndex);

    /**
     * Set the MIDI channel of the current track.
     *
     * @param nChannel The channel.
     * @ghidraAddress 0x001ec488
     */
    void SetChannel(unsigned char nChannel);

    /**
     * Set the TrackMode of the current track.
     *
     * @param nKind The TrackMode.
     * @ghidraAddress 0x001ec5c0
     */
    void SetKind(int nKind);

    /**
     * Set the instrument index and the name of the current track.
     *
     * @param nInstrument The instrument index.
     * @param name The track name, copied.
     * @ghidraAddress 0x001ec5d0
     */
    void SetInstrument(int nInstrument, const HxStr &name);

    /**
     * Append one MIDI event to the current track.
     *
     * The status and the channel arrive separately and the body combines them with a bitwise or
     * before forwarding to TrackData::AddMidiMsg().
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nStatus The MIDI status byte without its channel, 0x80 through 0xe0.
     * @param nData1 The first data byte.
     * @param nData2 The second data byte, or zero for a one-byte event.
     * @param nChannel The channel, which the body ors into the status.
     * @ghidraAddress 0x001ec4c8
     */
    void AddEvent(int nTick,
                  unsigned char nStatus,
                  unsigned char nData1,
                  unsigned char nData2,
                  unsigned char nChannel);

    /**
     * Forward to TrackData::AddNoteMsg() on the current track.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nUnknown09 The byte stored at the message's `+0x09`.
     * @param nUnknown0a The byte stored at the message's `+0x0a`.
     * @param nUnknown0c The word stored at the message's `+0x0c`.
     * @param nUnknown08 The byte stored at the message's `+0x08`.
     * @ghidraAddress 0x001ec4f8
     */
    void AddNoteMsg(int nTick,
                    unsigned char nUnknown09,
                    unsigned char nUnknown0a,
                    int nUnknown0c,
                    unsigned char nUnknown08);

    /**
     * Forward to TrackData::SetQuant() on the current track.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nQuant The quantisation, in MIDI ticks.
     * @ghidraAddress 0x001ec520
     */
    void SetQuant(int nTick, int nQuant);

    /**
     * Forward to TrackData::AddRiff() on the current track.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pRiff The riff.
     * @ghidraAddress 0x001ec540
     */
    void AddRiff(int nTick, Riff *pRiff);

    /**
     * Forward to TrackData::AddGem() on the current track.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nGem The gem.
     * @param pRiff The riff, or null.
     * @ghidraAddress 0x001ec5a0
     */
    void AddGem(int nTick, int nGem, Riff *pRiff);

    /**
     * Start a harmony on the current track.
     *
     * Forwards to TrackData::AddHarmony() on mCurrentTrack. LevelConverter calls it at
     * `0x001e6a60` and `0x001e7b04` with the harmony at its `+0x7c`.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param harmony The harmony, copied by the track.
     * @ghidraAddress 0x001ec560
     */
    void AddHarmony(int nTick, const Harmony &harmony);

    /**
     * Forward to TrackData::OnUnknown001d7758() on the current track.
     *
     * LevelConverter::EndTrack() calls it with two zeroes.
     *
     * @param nFirst The first argument, forwarded unchanged.
     * @param nSecond The second argument, forwarded unchanged.
     * @ghidraAddress 0x001ec580
     */
    void OnUnknownForwarder001ec580(int nFirst, int nSecond);

    /**
     * Replace the tempo map at `+0x30` with one built from a tempo meta event.
     *
     * The body releases the map already there, allocates 0x28 bytes, and constructs a
     * Sch::TempoMap from the tempo. The one caller, LevelConverter::Tempo() at `0x001ea570`, passes
     * its own two arguments through in the registers they arrive in.
     *
     * @param nTick The event position. No instruction in the body reads it.
     * @param nMicrosecondsPerQuarter The tempo the map is constructed from.
     * @ghidraAddress 0x001ec5f8
     */
    void SetTempo(int nTick, int nMicrosecondsPerQuarter);

    /**
     * Run TrackData::ScoreBars() on every score track.
     *
     * LevelConverter::Convert() calls it once the reader has been built and before the file is
     * read.
     *
     * @ghidraAddress 0x001ec680
     */
    void PrepareTracks();

private:
    // All three vectors manage their elements. The destructor clears each with a std::for_each
    // over the deleting function at 0x001ec328.
    std::vector<TrackData *> mTracks;        // +0x04
    std::vector<TrackData *> mBackingTracks; // +0x10
    // No slot of LevelData reads this collection.
    std::vector<TrackData *> mIntroTracks; // +0x1c
    // Deleted by the destructor through TrackData's own destructor at 0x001d3900.
    TrackData *mOwnTrack; // +0x28
    // The track the forwarding members append to. The destructor does not release it.
    TrackData *mCurrentTrack; // +0x2c
    // Released by the destructor through Attachment::Release(). SetTempo() replaces it.
    Sch::TempoMap *mUnknown30; // +0x30
    // Deleted by the destructor through its own table slot 1.
    PlayMap *mUnknown34; // +0x34
};
