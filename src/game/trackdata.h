#pragma once

#include <cstddef>
#include <iostream>
#include <vector>

#include "mid/mbt.h"
#include "mid/tickobj.h"
#include "os/hxstr.h"
#include "os/mem.h"

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
    kTrackModeAxe = 1,     /*!< Printed as `axe`. */
    kTrackModeRiff = 2,    /*!< Printed as `riff`, the default the constructor sets. */
    kTrackModeScratch = 3, /*!< Printed as `none`. Overlay's constructor labels it `SCRATCH`. */
    kTrackModeVocal = 4,   /*!< Printed as `none`. Overlay's constructor labels it `VOCAL`. */
    kTrackModeCatch = 5,   /*!< Printed as `catch`. */
};

/**
 * Converted events of one track of a level.
 *
 * The class emits no RTTI descriptor and is not polymorphic. Its name is attested rather than
 * inferred. The image records Catcher's constructor signature, whose third parameter is
 * `const TrackData *`, and the LevelBuilder constructor allocates it under the tag `TrackData`.
 *
 * The LevelBuilder allocation measures the object at 0x54 bytes. The constructor sizes mBars to the
 * last step of the play map, one Bar per bar of the level, and every position the class accepts
 * is split into a bar and an offset within the bar by Locate() or LocateMapped(), the second
 * mapping the bar through PlayMap::MapBar() first.
 *
 * The destructor deletes every riff set and every harmony the track created. A bar refers to them
 * only through its sorted lists.
 *
 * The members mIndex, mChannel, and mKind are public because every stage class reads them
 * directly and no accessor exists. Overlay reads mInstrument the same way. The rest are private.
 *
 * The queries are const. The mangled Catcher constructor attests a `const TrackData *`, every stage
 * class and Quantizer store the track in that form, and Quantizer::GetQuantum() calls GetQuant()
 * through the const pointer.
 */
class TrackData {
    // Catcher::FindNextGemTick() at 0x001aca48 reads mGemSearchBars directly.
    friend class Catcher;
    // LevelBuilder::SetInstrument() at 0x001ec5d0 assigns mName directly.
    friend class LevelBuilder;

public:
    /**
     * Allocate a track from the tagged heap under the tag `TrackData`.
     *
     * No out-of-line body exists. The LevelBuilder constructor and LevelBuilder::SelectTrack()
     * expand the call inline.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     */
    void *operator new(size_t nSize) {
        return AllocateTaggedMemory(nSize, "TrackData");
    }

    /**
     * Release a track to the tagged heap.
     *
     * No out-of-line body exists. The deleting destructor expands the call inline.
     *
     * @param pBlock The block.
     */
    void operator delete(void *pBlock) {
        FreeTaggedMemory(pBlock, "TrackData");
    }

    /**
     * One bar of a track, with its scoring values and the sorted lists of what starts in it.
     *
     * The record is 0x3c bytes, the element stride of TrackData::mBars. Print() labels every member
     * except mCatchPoints. The type name is inferred. The record has no descriptor, allocation tag,
     * or literal, and its allocations are billed to the vector.
     */
    struct Bar {
        /**
         * Construct an empty bar quantised to 120 ticks.
         *
         * @ghidraAddress NTSC-U/C: 0x001d2ef0
         * @ghidraAddress PAL: 0x001d8da8
         */
        Bar();

        /**
         * Delete every MIDI message the bar holds and free the four lists.
         *
         * @ghidraAddress NTSC-U/C: 0x001d2f50
         * @ghidraAddress PAL: 0x001d8e08
         */
        ~Bar();

        /**
         * Write every member Print() labels to a diagnostic stream.
         *
         * @param stream The stream to write to.
         * @ghidraAddress NTSC-U/C: 0x001d31d8
         * @ghidraAddress PAL: 0x001d9090
         */
        void Print(std::ostream &stream);

        int mQuant;       /*!< Labelled `quant = `. +0x00 */
        int mPoints;      /*!< Labelled `points = `. +0x04 */
        int mCatchPoints; /*!< The level's catch points, set by ScoreBars(). +0x08 */
        std::vector<TickObj<Harmony *> > mHarmonies; /*!< Labelled `harms: `. +0x0c */
        std::vector<TickObj<RiffSet *> > mRiffSets;  /*!< Labelled `riffs: `. +0x18 */
        std::vector<TickObj<MuseMsg *> > mMidi;      /*!< Labelled `midi: `, owned. +0x24 */
        std::vector<TickObj<int> > mGems;            /*!< Labelled `gems: `. +0x30 */
    };

    /**
     * Construct an empty track over a play map.
     *
     * @param nIndex The track's index, stored in mIndex.
     * @param pMap The play map whose last step sizes mBars.
     * @ghidraAddress NTSC-U/C: 0x001d35a8
     * @ghidraAddress PAL: 0x001d9460
     */
    TrackData(int nIndex, PlayMap *pMap);

    /**
     * Delete every riff set and harmony the track created, then the bars.
     *
     * LevelBuilder's deleter at `0x001ec328` passes an in-charge value of 3.
     *
     * @ghidraAddress NTSC-U/C: 0x001d3900
     * @ghidraAddress PAL: 0x001d97d0
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
     * @ghidraAddress NTSC-U/C: 0x001d3b18
     * @ghidraAddress PAL: 0x001d99f8
     */
    void AddRiff(int nTick, Riff *pRiff);

    /**
     * Start a harmony at a song position.
     *
     * The harmony is stored at that position in its bar and at offset zero in every later bar. A
     * position at or past the end of the track is ignored.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param harmony The harmony, copied onto the heap.
     * @ghidraAddress NTSC-U/C: 0x001d3d10
     * @ghidraAddress PAL: 0x001d9bf0
     */
    void AddHarmony(int nTick, const Harmony &harmony);

    /**
     * Add a gem at a song position, with a riff set of its own when a riff is given.
     *
     * A position at or past the end of the track is ignored.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nGem The gem.
     * @param pRiff The riff, or null.
     * @ghidraAddress NTSC-U/C: 0x001d3ee0
     * @ghidraAddress PAL: 0x001d9dc0
     */
    void AddGem(int nTick, int nGem, Riff *pRiff);

    /**
     * Add a StdMidiMsg built from three bytes to the bar at a song position.
     *
     * The message is allocated under the message tag and inserted into Bar::mMidi at its offset
     * in the bar. A position at or past the end of the track is ignored.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nStatus The status byte.
     * @param nData1 The first data byte.
     * @param nData2 The second data byte.
     * @ghidraAddress NTSC-U/C: 0x001d4088
     * @ghidraAddress PAL: 0x001d9f68
     */
    void AddMidiMsg(int nTick, unsigned char nStatus, unsigned char nData1, unsigned char nData2);

    /**
     * Add a NoteMsg built from four values to the bar at a song position.
     *
     * Placed the same way as AddMidiMsg().
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nNote The note number.
     * @param nVelocity The note-on velocity.
     * @param nLength The length of the note, in MIDI ticks.
     * @param nChannel The MIDI channel.
     * @ghidraAddress NTSC-U/C: 0x001d41c0
     * @ghidraAddress PAL: 0x001da0a0
     */
    void AddNoteMsg(int nTick,
                    unsigned char nNote,
                    unsigned char nVelocity,
                    int nLength,
                    unsigned char nChannel);

    /**
     * Give every bar its point value and its configured word.
     *
     * A track in one of the modes 1 through 3 takes a flat configured value for every bar, and any
     * other track scores each bar from its gems. The game mode is read and discarded.
     *
     * @ghidraAddress NTSC-U/C: 0x001d4308
     * @ghidraAddress PAL: 0x001da1e8
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
     * @ghidraAddress NTSC-U/C: 0x001d4428
     * @ghidraAddress PAL: 0x001da308
     */
    int FindGemAtOrBefore(int nTick, int *pTick, int *pGem) const;

    /**
     * Find the first gem at or after a song position within mGemSearchBars bars.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pTick Receives the gem's song position.
     * @param pGem Receives the gem.
     * @return Non-zero when a gem was found.
     * @ghidraAddress NTSC-U/C: 0x001d46f0
     * @ghidraAddress PAL: 0x001da5d0
     */
    int FindGemAtOrAfter(int nTick, int *pTick, int *pGem) const;

    /**
     * Write the track and every bar to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x001d4978
     * @ghidraAddress PAL: 0x001da858
     */
    void Print(std::ostream &stream);

    /**
     * Split a song position into its bar and the offset within the bar.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pBar Receives the bar.
     * @param offset Receives the offset within the bar.
     * @ghidraAddress NTSC-U/C: 0x001d4b40
     * @ghidraAddress PAL: 0x001daa20
     */
    void Locate(int nTick, Bar *&pBar, Mid::MBT &offset);

    /**
     * Split a song position into its bar, mapped through PlayMap::MapBar(), and the offset.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pBar Receives the bar.
     * @param offset Receives the offset within the bar.
     * @ghidraAddress NTSC-U/C: 0x001d4c58
     * @ghidraAddress PAL: 0x001dab38
     */
    void LocateMapped(int nTick, const Bar *&pBar, Mid::MBT &offset) const;

    /**
     * Add every gem of every phrase of a database, one phrase per step, and score each bar.
     *
     * GrooveWorld's setup path is the recovered caller.
     *
     * @param pDatabase The database.
     * @ghidraAddress NTSC-U/C: 0x001d4d90
     * @ghidraAddress PAL: 0x001dac70
     */
    void AddPhrases(PhraseDatabase *pDatabase);

    /**
     * Set the quantisation of the bar at a song position.
     *
     * A position at or past the end of the track is ignored.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nQuant The quantisation, in MIDI ticks.
     * @ghidraAddress NTSC-U/C: 0x001d7688
     * @ghidraAddress PAL: 0x001dd568
     */
    void SetQuant(int nTick, int nQuant);

    /**
     * Do nothing. LevelBuilder::SetActive() forwards an activeness controller change here.
     *
     * The title is inferred from the forwarder's callers.
     *
     * @param nTick The event position, not read.
     * @param bActive Whether the controller value was non-zero, not read.
     * @ghidraAddress NTSC-U/C: 0x001d7758
     * @ghidraAddress PAL: 0x001dd638
     */
    void SetActive(int nTick, int bActive);

    /**
     * Report a player for a bar to the gamer, with this track's index.
     *
     * @param pPlayer The player.
     * @param nBar The bar.
     * @ghidraAddress NTSC-U/C: 0x001d7760
     * @ghidraAddress PAL: 0x001dd640
     */
    void SetOwner(Player *pPlayer, int nBar) const;

    /**
     * @param nTick The song position, in MIDI ticks.
     * @return The harmony in force at the position, or null.
     * @ghidraAddress NTSC-U/C: 0x001d7788
     * @ghidraAddress PAL: 0x001dd668
     */
    Harmony *GetHarmony(int nTick) const;

    /**
     * @param nTick The song position, in MIDI ticks.
     * @param nLevel The difficulty level.
     * @return The riff of that level in the riff set in force at the position, or null.
     * @ghidraAddress NTSC-U/C: 0x001d77e0
     * @ghidraAddress PAL: 0x001dd6c0
     */
    Riff *GetRiff(int nTick, int nLevel) const;

    /**
     * @param nBar The bar, mapped through PlayMap::MapBar().
     * @param nOffset The offset within the bar.
     * @param nLevel The difficulty level.
     * @return The riff of that level in the riff set in force at the offset, or null.
     * @ghidraAddress NTSC-U/C: 0x001d7858
     * @ghidraAddress PAL: 0x001dd738
     */
    Riff *GetRiffInMappedBar(int nBar, int nOffset, int nLevel) const;

    /**
     * @param nBar The index into mBars, not mapped.
     * @param nOffset The offset within the bar.
     * @param nLevel The difficulty level.
     * @return The riff of that level in the riff set in force at the offset, or null.
     * @ghidraAddress NTSC-U/C: 0x001d78d0
     * @ghidraAddress PAL: 0x001dd7b0
     */
    Riff *GetRiffInBar(int nBar, int nOffset, int nLevel) const;

    /**
     * @param nTick The song position, in MIDI ticks.
     * @return The gem at exactly the position, or -1.
     * @ghidraAddress NTSC-U/C: 0x001d7948
     * @ghidraAddress PAL: 0x001dd828
     */
    int GetGemAt(int nTick) const;

    /**
     * @param nBar The bar, mapped through PlayMap::MapBar().
     * @return The bar's quantisation.
     * @ghidraAddress NTSC-U/C: 0x001d79a8
     * @ghidraAddress PAL: 0x001dd888
     */
    int GetQuant(int nBar) const;

    /**
     * @param nBar The bar.
     * @return PlayMap::IsStepStart() for the bar.
     * @ghidraAddress NTSC-U/C: 0x001d79d0
     * @ghidraAddress PAL: 0x001dd8b0
     */
    int IsStepStart(int nBar) const;

    /**
     * @param nBar The bar.
     * @return PlayMap::StepStartBar() for the bar.
     * @ghidraAddress NTSC-U/C: 0x001d79f0
     * @ghidraAddress PAL: 0x001dd8d0
     */
    int StepStartBar(int nBar) const;

    /**
     * @param nBar The bar.
     * @return PlayMap::NextStepBar() for the bar.
     * @ghidraAddress NTSC-U/C: 0x001d7a10
     * @ghidraAddress PAL: 0x001dd8f0
     */
    int NextStepBar(int nBar) const;

    /**
     * @param nBar The bar.
     * @return PlayMap::FollowingStepBar() for the bar.
     * @ghidraAddress NTSC-U/C: 0x001d7a30
     * @ghidraAddress PAL: 0x001dd910
     */
    int FollowingStepBar(int nBar) const;

    /**
     * Ask the gamer about a bar, with this track's index.
     *
     * @param nBar The bar.
     * @return The gamer's answer.
     * @ghidraAddress NTSC-U/C: 0x001d7a50
     * @ghidraAddress PAL: 0x001dd930
     */
    int QueryBar(int nBar) const;

    /**
     * @param nBar The bar, mapped through PlayMap::MapBar().
     * @return The bar's points.
     * @ghidraAddress NTSC-U/C: 0x001d7a78
     * @ghidraAddress PAL: 0x001dd958
     */
    int GetPoints(int nBar) const;

    /**
     * @param nBar The bar, mapped through PlayMap::MapBar().
     * @return The bar's mCatchPoints.
     * @ghidraAddress NTSC-U/C: 0x001d7aa0
     * @ghidraAddress PAL: 0x001dd980
     */
    int GetCatchPoints(int nBar) const;

    /**
     * @param nBar The index into mBars, not mapped.
     * @return The bar's MIDI messages.
     * @ghidraAddress NTSC-U/C: 0x001d7ac8
     * @ghidraAddress PAL: 0x001dd9a8
     */
    const std::vector<TickObj<MuseMsg *> > *GetMidiInBar(int nBar) const;

    /**
     * @param nBar The index into mBars, not mapped.
     * @return The bar's gems.
     * @ghidraAddress NTSC-U/C: 0x001d7ae0
     * @ghidraAddress PAL: 0x001dd9c0
     */
    const std::vector<TickObj<int> > *GetGemsInBar(int nBar) const;

    /**
     * @param nBar The bar, mapped through PlayMap::MapBar().
     * @return The bar's MIDI messages.
     * @ghidraAddress NTSC-U/C: 0x001d7af8
     * @ghidraAddress PAL: 0x001dd9d8
     */
    const std::vector<TickObj<MuseMsg *> > *GetMidi(int nBar) const;

    /**
     * @param nBar The bar, mapped through PlayMap::MapBar().
     * @return The bar's gems.
     * @ghidraAddress NTSC-U/C: 0x001d7b20
     * @ghidraAddress PAL: 0x001dda00
     */
    const std::vector<TickObj<int> > *GetGems(int nBar) const;

    /**
     * Find the bar of a song position, discarding the offset.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pBar Receives the bar.
     * @ghidraAddress NTSC-U/C: 0x001d7b48
     * @ghidraAddress PAL: 0x001dda28
     */
    void Locate(int nTick, Bar *&pBar);

    /**
     * Find the mapped bar of a song position, discarding the offset.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pBar Receives the bar.
     * @ghidraAddress NTSC-U/C: 0x001d7b70
     * @ghidraAddress PAL: 0x001dda50
     */
    void LocateMapped(int nTick, const Bar *&pBar) const;

    /**
     * Find a bar by its bar number, mapped through PlayMap::MapBar().
     *
     * @param nBar The bar.
     * @param pBar Receives the bar.
     * @ghidraAddress NTSC-U/C: 0x001d7b98
     * @ghidraAddress PAL: 0x001dda78
     */
    void GetBar(int nBar, const Bar *&pBar) const;

    /**
     * @param nBar The bar.
     * @return The index of the step at or before the bar, through PlayMap::FindStepIndex().
     * @ghidraAddress NTSC-U/C: 0x001d7bf0
     * @ghidraAddress PAL: 0x001ddad0
     */
    int FindStepIndex(int nBar) const;

    Gamer *mGamer; /*!< The gamer the track reports to. Not written by the constructor. +0x00 */
    int mIndex;    /*!< The track's index. Copied into ScoreTrackGraph's first member. +0x04 */
    unsigned char mChannel;      /*!< MIDI channel the stage sends controller changes on. +0x08 */
    unsigned char mPaddingByte;  /*!< No recovered routine accesses it. +0x09 */
    unsigned short mPaddingHalf; /*!< No recovered routine accesses it. +0x0a */
    int mKind; /*!< A TrackMode. 2 selects a NotePitcher and 3 a Scratcher. +0x0c */

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
    // NTSC-U/C: 0x001d2e08, PAL: 0x001d8cc0
    static int ScoreGems(const std::vector<TickObj<int> > &gems);

    // Fills the weights and thresholds ScoreGems() uses from the configuration, once.
    // NTSC-U/C: 0x001d2ca8, PAL: 0x001d8b60
    static void InitScoreTables();

    HxStr mName;                            // +0x14
    PlayMap *mMap;                          // +0x1c
    std::vector<Bar> mBars;                 // +0x20
    int mBarLength;                         // +0x2c, 1920 ticks
    int mGemSearchBars;                     // +0x30, the bars FindGemAtOrAfter() searches, 4
    RiffSet *mCurrentRiffSet;               // +0x34
    int mCurrentRiffTick;                   // +0x38, starts at -1
    std::vector<RiffSet *> mRiffSetsOwned;  // +0x3c
    std::vector<Harmony *> mHarmoniesOwned; // +0x48
};
