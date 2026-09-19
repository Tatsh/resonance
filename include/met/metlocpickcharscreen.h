#pragma once

#include <vector>

#include "met/metmemdetectscreen.h"
#include "rnd/object.h"

/**
 * Local-multiplayer screen that picks a character.
 *
 * `20MetLocPickCharScreen` in the RTTI descriptor at `0x00901bf0`, with MetMemDetectScreen as its
 * one public non-virtual base at offset 0. The object is at least 0x140 bytes. The 44-entry
 * primary vtable is at `0x007f9a88` and the 21-entry MemcardUser table at `0x007f99d8` adjusts
 * `this` by `-140`. The primary is the same length as the MetMemDetectScreen table, so the class
 * declares no virtual of its own.
 *
 * The constructor at `0x002b1270` takes only the renderer and the load priority. It runs the
 * MetMemDetectScreen constructor with `mpc` for the screen name, `metagame/_Local` for the
 * directory, and `character_loc` for the container, and pushes `loc_pc` into the container
 * object-name vector that MetScreen owns. It then empties eight vectors and zeroes seven further
 * words. This is the widest own-member run in the subsystem.
 *
 * The destructor at `0x002b19b0` runs eleven vector teardowns, restores both vptrs, runs the
 * MetMemDetectScreen destructor, and releases the object with the tag `MsgSink`.
 *
 * Twelve slots differ from the MetMemDetectScreen table, and only the destructor has a recovered
 * name. The others are 5 `0x002b29e8`, 15 `0x002ba178`, 19 `0x002b2370`, 20 `0x002b9fe8`,
 * 22 `0x002b9e58`, 23 `0x002b9f48`, 24 `0x002b9f98`, 26 `0x002ba038`, 36 `0x002b3e18`,
 * 38 `0x002b1e00`, 39 `0x002b4068`, 40 `0x002ba148`, 41 `0x002b4378`, and 42 `0x002b4738`.
 */
class MetLocPickCharScreen : public MetMemDetectScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002b1270
     */
    MetLocPickCharScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002b19b0
     */
    virtual ~MetLocPickCharScreen();

    /**
     * @ghidraAddress 0x002b9fe8
     */
    virtual void PlaySlideSound();

    /**
     * @ghidraAddress 0x002b9e58
     */
    virtual void PlayHighSound();

    /**
     * @ghidraAddress 0x002b9f48
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x002b9f98
     */
    virtual void PlayCycleRightSound();

private:
    int mUnknowna0;                         // +0xa0
    int mUnknowna4;                         // +0xa4
    std::vector<Rnd::Object *> mUnknowna8;  // +0xa8
    std::vector<Rnd::Object *> mUnknownb4;  // +0xb4
    std::vector<Rnd::Object *> mUnknownc0;  // +0xc0
    std::vector<Rnd::Object *> mUnknowncc;  // +0xcc
    std::vector<Rnd::Object *> mUnknownd8;  // +0xd8
    std::vector<Rnd::Object *> mUnknowne4;  // +0xe4
    int mUnknownf0;                         // +0xf0
    std::vector<Rnd::Object *> mUnknownf4;  // +0xf4
    std::vector<Rnd::Object *> mUnknown108; // +0x108
    std::vector<Rnd::Object *> mUnknown114; // +0x114
    int mUnknown120;                        // +0x120
    int mUnknown124;                        // +0x124
    int mUnknown128;                        // +0x128
    // Never written by the constructor and not recovered.
    unsigned char mUnknown12c[8];           // +0x12c
    std::vector<Rnd::Object *> mUnknown134; // +0x134
};
