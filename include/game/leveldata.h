#pragma once

class PlayMap;
class TrackData;

namespace Sch {
class TempoMap;
} // namespace Sch

/**
 * Read-only interface the game reads one converted level through.
 *
 * `9LevelData` in the RTTI descriptor at `0x0086f600`, a leaf with no base list, built through
 * TypeInfo__ConstructBuiltin. The class has no data member, so the vptr sits at `+0x00` and the
 * object is four bytes. Its table is at `0x007e7a50` and has ten entries with a zero terminator at
 * index 10. Slots 2 through 9 all address the shared pure-virtual stub at `0x005381a8`, so the
 * class is abstract and supplies nothing but the interface.
 *
 * LevelBuilder is the one subclass in the image and it implements all eight slots. Every
 * implementation is a single load or a subtraction and a shift, which is what makes this an
 * accessor interface over data a conversion has already produced.
 *
 * Every method name here is inferred from the LevelBuilder body behind it. RTTI in this image
 * yields class names only. The labels LevelBuilder's print routine writes, `Score Track#` and
 * `Backing Track#`, distinguish the two collections.
 */
class LevelData {
public:
    /**
     * @ghidraAddress 0x001ec388
     */
    virtual ~LevelData();

    /**
     * Report how many tracks the level has.
     *
     * Slot 2, and pure.
     *
     * @return The count.
     */
    virtual int TrackCount() = 0;

    /**
     * Report how many backing tracks the level has.
     *
     * Slot 3, and pure.
     *
     * @return The count.
     */
    virtual int BackingTrackCount() = 0;

    /**
     * Report the level's own track.
     *
     * Slot 4, and pure. LevelBuilder returns the one track it manages separately from the
     * collection the two slots above and below describe.
     *
     * @return The track.
     */
    virtual TrackData *OwnTrack() = 0;

    /**
     * Report one track by index.
     *
     * Slot 5, and pure. An index outside the collection is not tested for.
     *
     * @param nIndex The track.
     * @return The track.
     */
    virtual TrackData *TrackAt(int nIndex) = 0;

    /**
     * Report one backing track by index.
     *
     * Slot 6, and pure. An index outside the collection is not tested for. The element type comes
     * from LevelBuilder's destructor, which clears all three of its collections with the same
     * deleting function at `0x001ec328`, and that function deletes a TrackData. LevelBuilder's
     * print routine labels the collection `Backing Track#`.
     *
     * @param nIndex The track.
     * @return The track.
     */
    virtual TrackData *BackingTrackAt(int nIndex) = 0;

    /**
     * Report the level's tempo map. Slot 7, and pure.
     *
     * LevelBuilder returns the word at its own `+0x30`. Its constructor fills that word with a
     * 0x28-byte object built by `0x0052d118` with 500000 microseconds per quarter note, which is
     * the Sch::TempoMap constructor, and GrooveWorld::FinishLoad() hands the result to
     * Sch::TickClock::SetTempoMap(). The verb remains unrecovered.
     *
     * @return The tempo map.
     */
    virtual Sch::TempoMap *OnUnknownSlot7() = 0;

    /**
     * Report the level's play map. Slot 8, and pure.
     *
     * LevelBuilder returns the word at its own `+0x34`. The LevelBuilder constructor at
     * `0x001ea838` fills that word with a 0x74-byte allocation under the tag `PlayMap`, constructed
     * by `0x00127a80` with PlayMapLinear's table at `0x007d1060` written to its `+0x38`, and the
     * destructor deletes it through table slot 1. The verb remains unrecovered.
     *
     * @return The play map.
     */
    virtual PlayMap *OnUnknownSlot8() = 0;

    /**
     * Unrecovered. Slot 9, and pure.
     *
     * LevelBuilder forwards to slot 8 of the object slot 8 returns and hands back whatever that
     * reports. Both the verb and the return type are therefore unrecovered.
     */
    virtual void OnUnknownSlot9() = 0;
};
