#pragma once

#include <iostream>
#include <vector>

#include "mid/mbt.h"
#include "mid/tickobj.h"
#include "os/hxstr.h"

class Gamer;
class MuseMsg;
class PhraseDatabase;
class PlayMap;
class Player;
class Riff;
struct Harmony;
struct RiffSet;

/** The play modes of a track, as TrackData::Print() writes them. */
enum TrackMode {
    kTrackModeAxe = 1,   /*!< Printed as `axe`. */
    kTrackModeRiff = 2,  /*!< Printed as `riff`, the default the constructor sets. */
    kTrackModeCatch = 5, /*!< Printed as `catch`. */
};

/**
 * Converted events of one track of a level.
 *
 * The class emits no RTTI descriptor and is not polymorphic. Its name is attested rather than
 * inferred. The RTTI records Catcher's constructor signature as
 * `__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick`, whose fourth
 * parameter demangles to `const TrackData *`, and the LevelBuilder constructor allocates it under
 * the tag `TrackData`.
 *
 * The LevelBuilder allocation measures the object at 0x54 bytes. The constructor sizes mBars to the
 * last step of the play map, one Bar per bar of the level, and every position the class accepts
 * is split into a bar and an offset within the bar by Locate() or LocateMapped(), the second
 * mapping the bar through PlayMap::Slot5() first.
 *
 * The destructor deletes every riff set and every harmony the track created. A bar refers to them
 * only through its sorted lists.
 *
 * The members mUnknown04, mChannel, and mKind are public because every stage class reads them
 * directly and no accessor exists. Overlay reads mInstrument the same way. The rest are private.
 *
 * The queries are const. The mangled Catcher constructor attests a `const TrackData *`, every stage
 * class and Quantizer store the track in that form, and Quantizer::GetQuantum() calls GetQuant()
 * through the const pointer.
 */
class TrackData {
public:
    /**
     * One bar of a track, with its scoring values and the sorted lists of what starts in it.
     *
     * The record is 0x3c bytes, the element stride of TrackData::mBars. Print() labels every member
     * except mUnknown08. The type name is inferred. The record has no descriptor, allocation tag,
     * or literal, and its allocations are billed to the vector.
     */
    struct Bar {
        /**
         * Construct an empty bar quantised to 120 ticks.
         *
         * @ghidraAddress 0x001d2ef0
         */
        Bar();

        /**
         * Delete every MIDI message the bar holds and free the four lists.
         *
         * @ghidraAddress 0x001d2f50
         */
        ~Bar();

        /**
         * Write every member Print() labels to a diagnostic stream.
         *
         * @param stream The stream to write to.
         * @ghidraAddress 0x001d31d8
         */
        void Print(std::ostream &stream);

        int mQuant;                                  /*!< Labelled `quant = `. +0x00 */
        int mPoints;                                 /*!< Labelled `points = `. +0x04 */
        int mUnknown08;                              /*!< Set by ScoreBars(). +0x08 */
        std::vector<TickObj<Harmony *> > mHarmonies; /*!< Labelled `harms: `. +0x0c */
        std::vector<TickObj<RiffSet *> > mRiffSets;  /*!< Labelled `riffs: `. +0x18 */
        std::vector<TickObj<MuseMsg *> > mMidi;      /*!< Labelled `midi: `, owned. +0x24 */
        std::vector<TickObj<int> > mGems;            /*!< Labelled `gems: `. +0x30 */
    };

    /**
     * Construct an empty track over a play map.
     *
     * @param nIndex The track's index, stored in mUnknown04.
     * @param pMap The play map whose last step sizes mBars.
     * @ghidraAddress 0x001d35a8
     */
    TrackData(int nIndex, PlayMap *pMap);

    /**
     * Delete every riff set and harmony the track created, then the bars.
     *
     * LevelBuilder's deleter at `0x001ec328` passes an in-charge value of 3.
     *
     * @ghidraAddress 0x001d3900
     */
    ~TrackData();

    /**
     * Give a riff a place in the riff set at a song position.
     *
     * A position other than the one the current riff set was created for starts a new set. The new
     * set is stored at that position in its bar and at offset zero in every later bar. A position
     * at or past the end of the track is ignored.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pRiff The riff, stored at the index given by its mId.
     * @ghidraAddress 0x001d3b18
     */
    void AddRiff(int nTick, Riff *pRiff);

    /**
     * Start a harmony at a song position.
     *
     * The harmony is stored at that position in its bar and at offset zero in every later bar. A
     * position at or past the end of the track is ignored.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param notes The notes of the harmony, copied into it.
     * @ghidraAddress 0x001d3d10
     */
    void AddHarmony(int nTick, const std::vector<char> &notes);

    /**
     * Add a gem at a song position, with a riff set of its own when a riff is given.
     *
     * A position at or past the end of the track is ignored.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nGem The gem.
     * @param pRiff The riff, or null.
     * @ghidraAddress 0x001d3ee0
     */
    void AddGem(int nTick, int nGem, Riff *pRiff);

    /**
     * Add a StdMidiMsg built from three bytes to the bar at a song position.
     *
     * The body is not written, because StdMidiMsg declares its payload private and no constructor
     * that takes it. The message is allocated under the message tag and inserted into Bar::mMidi.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nStatus The status byte.
     * @param nData1 The first data byte.
     * @param nData2 The second data byte.
     * @ghidraAddress 0x001d4088
     */
    void AddMidiMsg(int nTick, unsigned char nStatus, unsigned char nData1, unsigned char nData2);

    /**
     * Add a NoteMsg built from four values to the bar at a song position.
     *
     * The body is not written, for the reason recorded on AddMidiMsg().
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nUnknown09 The byte stored at the message's `+0x09`.
     * @param nUnknown0a The byte stored at the message's `+0x0a`.
     * @param nUnknown0c The word stored at the message's `+0x0c`.
     * @param nUnknown08 The byte stored at the message's `+0x08`.
     * @ghidraAddress 0x001d41c0
     */
    void AddNoteMsg(int nTick,
                    unsigned char nUnknown09,
                    unsigned char nUnknown0a,
                    int nUnknown0c,
                    unsigned char nUnknown08);

    /**
     * Give every bar its point value and its configured word.
     *
     * A track in one of the modes 1 through 3 takes a flat configured value for every bar, and any
     * other track scores each bar from its gems. The game mode is read and discarded.
     *
     * @ghidraAddress 0x001d4308
     */
    void ScoreBars();

    /**
     * Find the last gem at or before a song position, looking back into the previous bar when the
     * position's own bar has none.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pTick Receives the gem's song position.
     * @param pGem Receives the gem.
     * @return Non-zero when a gem was found.
     * @ghidraAddress 0x001d4428
     */
    int FindGemAtOrBefore(int nTick, int *pTick, int *pGem) const;

    /**
     * Find the first gem at or after a song position within mUnknown30 bars.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pTick Receives the gem's song position.
     * @param pGem Receives the gem.
     * @return Non-zero when a gem was found.
     * @ghidraAddress 0x001d46f0
     */
    int FindGemAtOrAfter(int nTick, int *pTick, int *pGem) const;

    /**
     * Write the track and every bar to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001d4978
     */
    void Print(std::ostream &stream);

    /**
     * Split a song position into its bar and the offset within the bar.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pBar Receives the bar.
     * @param offset Receives the offset within the bar.
     * @ghidraAddress 0x001d4b40
     */
    void Locate(int nTick, Bar *&pBar, Mid::MBT &offset);

    /**
     * Split a song position into its bar, mapped through PlayMap::Slot5(), and the offset.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pBar Receives the bar.
     * @param offset Receives the offset within the bar.
     * @ghidraAddress 0x001d4c58
     */
    void LocateMapped(int nTick, const Bar *&pBar, Mid::MBT &offset) const;

    /**
     * Add every gem of every phrase of a database, one phrase per step, and score each bar.
     *
     * GrooveWorld's setup path is the recovered caller.
     *
     * @param pDatabase The database.
     * @ghidraAddress 0x001d4d90
     */
    void AddPhrases(PhraseDatabase *pDatabase);

    /**
     * Set the quantisation of the bar at a song position.
     *
     * A position at or past the end of the track is ignored.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nQuant The quantisation, in MIDI ticks.
     * @ghidraAddress 0x001d7688
     */
    void SetQuant(int nTick, int nQuant);

    /**
     * Do nothing. LevelBuilder's forwarder at `0x001ec580` calls it with two zeroes.
     *
     * @param nFirst The first argument, not read.
     * @param nSecond The second argument, not read.
     * @ghidraAddress 0x001d7758
     */
    void OnUnknown001d7758(int nFirst, int nSecond);

    /**
     * Report a player for a bar to the gamer, with this track's index.
     *
     * @param pPlayer The player.
     * @param nBar The bar.
     * @ghidraAddress 0x001d7760
     */
    void SetOwner(Player *pPlayer, int nBar);

    /**
     * @param nTick The song position, in MIDI ticks.
     * @return The harmony in force at the position, or null.
     * @ghidraAddress 0x001d7788
     */
    Harmony *GetHarmony(int nTick) const;

    /**
     * @param nTick The song position, in MIDI ticks.
     * @param nLevel The difficulty level.
     * @return The riff of that level in the riff set in force at the position, or null.
     * @ghidraAddress 0x001d77e0
     */
    Riff *GetRiff(int nTick, int nLevel) const;

    /**
     * @param nBar The bar, mapped through PlayMap::Slot5().
     * @param nOffset The offset within the bar.
     * @param nLevel The difficulty level.
     * @return The riff of that level in the riff set in force at the offset, or null.
     * @ghidraAddress 0x001d7858
     */
    Riff *GetRiffInMappedBar(int nBar, int nOffset, int nLevel) const;

    /**
     * @param nBar The index into mBars, not mapped.
     * @param nOffset The offset within the bar.
     * @param nLevel The difficulty level.
     * @return The riff of that level in the riff set in force at the offset, or null.
     * @ghidraAddress 0x001d78d0
     */
    Riff *GetRiffInBar(int nBar, int nOffset, int nLevel) const;

    /**
     * @param nTick The song position, in MIDI ticks.
     * @return The gem at exactly the position, or -1.
     * @ghidraAddress 0x001d7948
     */
    int GetGemAt(int nTick) const;

    /**
     * @param nBar The bar, mapped through PlayMap::Slot5().
     * @return The bar's quantisation.
     * @ghidraAddress 0x001d79a8
     */
    int GetQuant(int nBar) const;

    /**
     * @param nBar The bar.
     * @return PlayMap::IsStepStart() for the bar.
     * @ghidraAddress 0x001d79d0
     */
    int IsStepStart(int nBar) const;

    /**
     * @param nBar The bar.
     * @return PlayMap::StepStartBar() for the bar.
     * @ghidraAddress 0x001d79f0
     */
    int StepStartBar(int nBar) const;

    /**
     * @param nBar The bar.
     * @return PlayMap::NextStepBar() for the bar.
     * @ghidraAddress 0x001d7a10
     */
    int NextStepBar(int nBar) const;

    /**
     * @param nBar The bar.
     * @return PlayMap::FollowingStepBar() for the bar.
     * @ghidraAddress 0x001d7a30
     */
    int FollowingStepBar(int nBar) const;

    /**
     * Ask the gamer about a bar, with this track's index.
     *
     * @param nBar The bar.
     * @return The gamer's answer.
     * @ghidraAddress 0x001d7a50
     */
    int QueryBar(int nBar) const;

    /**
     * @param nBar The bar, mapped through PlayMap::Slot5().
     * @return The bar's points.
     * @ghidraAddress 0x001d7a78
     */
    int GetPoints(int nBar) const;

    /**
     * @param nBar The bar, mapped through PlayMap::Slot5().
     * @return The bar's mUnknown08.
     * @ghidraAddress 0x001d7aa0
     */
    int GetUnknown08(int nBar) const;

    /**
     * @param nBar The index into mBars, not mapped.
     * @return The bar's MIDI messages.
     * @ghidraAddress 0x001d7ac8
     */
    const std::vector<TickObj<MuseMsg *> > *GetMidiInBar(int nBar) const;

    /**
     * @param nBar The index into mBars, not mapped.
     * @return The bar's gems.
     * @ghidraAddress 0x001d7ae0
     */
    const std::vector<TickObj<int> > *GetGemsInBar(int nBar) const;

    /**
     * @param nBar The bar, mapped through PlayMap::Slot5().
     * @return The bar's MIDI messages.
     * @ghidraAddress 0x001d7af8
     */
    const std::vector<TickObj<MuseMsg *> > *GetMidi(int nBar) const;

    /**
     * @param nBar The bar, mapped through PlayMap::Slot5().
     * @return The bar's gems.
     * @ghidraAddress 0x001d7b20
     */
    const std::vector<TickObj<int> > *GetGems(int nBar) const;

    /**
     * Find the bar of a song position, discarding the offset.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pBar Receives the bar.
     * @ghidraAddress 0x001d7b48
     */
    void Locate(int nTick, Bar *&pBar);

    /**
     * Find the mapped bar of a song position, discarding the offset.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pBar Receives the bar.
     * @ghidraAddress 0x001d7b70
     */
    void LocateMapped(int nTick, const Bar *&pBar) const;

    /**
     * Find a bar by its bar number, mapped through PlayMap::Slot5().
     *
     * @param nBar The bar.
     * @param pBar Receives the bar.
     * @ghidraAddress 0x001d7b98
     */
    void GetBar(int nBar, const Bar *&pBar) const;

    /**
     * @param nBar The bar.
     * @return The index of the step at or before the bar, through PlayMap::FindStepIndex().
     * @ghidraAddress 0x001d7bf0
     */
    int FindStepIndex(int nBar) const;

    Gamer *mGamer;  /*!< The gamer the track reports to. Not written by the constructor. +0x00 */
    int mUnknown04; /*!< The track's index. Copied into ScoreTrackGraph's first member. +0x04 */
    unsigned char mChannel;    /*!< MIDI channel the stage sends controller changes on. +0x08 */
    unsigned char mUnknown09;  /*!< +0x09 */
    unsigned short mUnknown0a; /*!< +0x0a */
    int mKind;                 /*!< A TrackMode. 2 selects a NotePitcher and 3 a Scratcher. +0x0c */

    /**
     * Instrument index from 0 to 5. +0x10
     *
     * Overlay's constructor reads it at `0x0041d0dc` through LevelData::TrackAt() and selects
     * `DRUMS`, `BASS`, `SYNTH`, `GUITAR`, `VOCAL`, or `FX` through the jump table at `0x00819080`.
     */
    int mInstrument;

private:
    // Scores a bar from its gems. Each gem adds the weight of the first divisor its position is a
    // multiple of, and the total is placed among the configured thresholds.
    // 0x001d2e08
    static int ScoreGems(const std::vector<TickObj<int> > &gems);

    // Fills the weights and thresholds ScoreGems() uses from the configuration, once.
    // 0x001d2ca8
    static void InitScoreTables();

    HxStr mName;                            // +0x14
    PlayMap *mMap;                          // +0x1c
    std::vector<Bar> mBars;                 // +0x20
    int mBarLength;                         // +0x2c, 1920 ticks
    int mUnknown30;                         // +0x30, the bars FindGemAtOrAfter() searches, 4
    RiffSet *mCurrentRiffSet;               // +0x34
    int mCurrentRiffTick;                   // +0x38, starts at -1
    std::vector<RiffSet *> mRiffSetsOwned;  // +0x3c
    std::vector<Harmony *> mHarmoniesOwned; // +0x48
};
