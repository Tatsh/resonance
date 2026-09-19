#pragma once

/**
 * Converted events of one track of a level.
 *
 * The class emits no RTTI descriptor, so it is not polymorphic. Its name is attested rather than
 * inferred: the RTTI records Catcher's constructor signature as
 * `__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick`, whose fourth
 * parameter demangles to `const TrackData *`.
 *
 * This header is a stub. It declares the destructor with its address, and the four members the
 * four gameplay stage classes read. The appenders at `0x001d4088`, `0x001d3d10`, `0x001d7758`, and
 * `0x001d4308` that LevelBuilder forwards to are unrecovered here, and so is every member past
 * `+0x0c`.
 *
 * The four members come from the stage constructors and their start and stop paths.
 * ScoreTrackGraph's constructor at `0x001cee50` copies the word at `+0x04` into its own first
 * member, each stage's slot 2 and slot 3 read the byte at `+0x08` as the MIDI channel it sends a
 * controller change on, and PitchingSTG's constructor branches on the word at `+0x0c` to choose a
 * NotePitcher for 2 and a Scratcher for 3. The members are public because every reader is outside
 * the class and no accessor exists in the image.
 *
 * The destructor takes an in-charge argument, which this toolchain emits for a destructor reached
 * through a deleting call. LevelBuilder's deleter at `0x001ec328` passes 3. A class with no
 * descriptor would not normally take one, and that is recorded rather than explained.
 */
class TrackData {
public:
    /**
     * @ghidraAddress 0x001d3900
     */
    ~TrackData();

    int mUnknown00;            /*!< +0x00 */
    int mUnknown04;            /*!< Copied into ScoreTrackGraph's first member. +0x04 */
    unsigned char mChannel;    /*!< MIDI channel the stage sends controller changes on. +0x08 */
    unsigned char mUnknown09;  /*!< +0x09 */
    unsigned short mUnknown0a; /*!< +0x0a */
    int mKind;                 /*!< 2 selects a NotePitcher and 3 a Scratcher. +0x0c */
};
