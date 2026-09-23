#pragma once

#include <list>
#include <vector>

class RawController;

/**
 * Reader of the physical controllers.
 *
 * `11InputPoller` in the RTTI descriptor at `0x0086f6d8`, with no base, so the vptr lands after the
 * data at `+0x58` and the class is 0x5c bytes. Its vtable at `0x007e69b8` has two entries and a
 * zero terminator at index 2, the type function and the destructor at `0x001df080`, so the
 * destructor is the only virtual the class declares. The size comes from the allocation in the
 * GameManagerImpl constructor.
 *
 * The translation unit is `InputPollerPS2.cpp`, which the anonymous-namespace marker
 * `Q235_GLOBAL_$N$InputPollerPS2.cppXFKhgb24FindJoypadConnectionsCmd` at `0x007e6ae8` records. That
 * same marker names one file-private class, FindJoypadConnectionsCmd. The file is therefore the
 * PlayStation 2 implementation, and a reconstructed implementation belongs at that basename rather
 * than at a portable one. The string pool of the unit also holds the InputCheatDetector family and
 * the cheat phrases, so the cheat decoder shares the file.
 *
 * The member map comes from the constructor at `0x001ded98`, the destructor, and the setup routine
 * at `0x001df248` together, and it accounts for every byte up to the vptr. The purpose of each
 * member is mostly unrecovered, so those members are private.
 *
 * Two further routines the constructor and destructor call are unrecovered, `0x001e19b8` on the way
 * in and `0x001df9d8` on the way out. The first is not a member: it ignores the pointer it receives
 * and fills a 16-entry table of controller bit masks at `0x008efb60` before delegating to
 * `0x001df248`.
 */
class InputPoller {
public:
    /**
     * @ghidraAddress 0x001ded98
     */
    InputPoller();

    /**
     * @ghidraAddress 0x001df080
     */
    virtual ~InputPoller();

    /**
     * Drive the vibration motors of the controller on one port.
     *
     * The body is not written. It finds the port in the list at `+0x0c` and, when present, passes
     * both levels to the pad record at `+0x1c`. ForceFeedbackMgr::ApplyMotors() is the recovered
     * caller.
     *
     * @param nPort The controller's port, from 1.
     * @param nSmallMotor The small motor's state, 0 or 1.
     * @param nBigMotor The big motor's level.
     * @ghidraAddress 0x001e1b78
     */
    void SetVibration(int nPort, int nSmallMotor, int nBigMotor);

    /**
     * Set the controller the readings go to.
     *
     * GameManagerImpl passes the MetaGameWorld in the front end and the game world's
     * RawController part during a game, or null where it has no world. The title is inferred.
     *
     * @param pController The receiver of the readings, or null.
     * @ghidraAddress 0x001e1998
     */
    void SetController(RawController *pController);

    /**
     * Set the word at `+0x3c`, which the constructor starts at 1.
     *
     * GameManagerImpl::Start() passes 1, and the begin-game and end-game handlers pass 0. The
     * title is inferred.
     *
     * @param bActive The flag.
     * @ghidraAddress 0x001e1a80
     */
    void SetActive(int bActive);

    /**
     * Set the paused flag.
     *
     * GameManagerImpl's pause handler passes 1 and its unpause handler 0. The title is inferred.
     *
     * @param bPaused The flag.
     * @ghidraAddress 0x001e1c18
     */
    void SetPaused(int bPaused);

    /**
     * Clear the controller the readings go to, if it is pController.
     *
     * GameManagerImpl::EndGame() and the routine at `0x0010c050` run it before deleting the game
     * world. The title is inferred.
     *
     * @param pController The controller being withdrawn.
     * @ghidraAddress 0x001e19a0
     */
    void DetachController(RawController *pController);

    /**
     * Read the controllers once.
     *
     * Runs the reading routine at `0x001dfab0`, which clears mPressedThisPoll and sets it again
     * when a reading goes out, and then an empty member at `0x001e1c58`.
     * GameManagerImpl::PollPlayback() is the caller. Not reconstructed yet, because the reading
     * routine is unrecovered. The title is inferred.
     *
     * @ghidraAddress 0x001e1c28
     */
    void Poll();

    /**
     * Set mGameInputEnabled.
     *
     * GameManagerImpl::OnBeginGameLocal() passes 1 outside jukebox mode, and
     * GameManagerImpl::Load() passes 0. The title is inferred.
     *
     * @param bEnabled The flag.
     * @ghidraAddress 0x001e1c20
     */
    void SetGameInputEnabled(int bEnabled) {
        mGameInputEnabled = bEnabled;
    }

    /**
     * Set mUnknown34 to 1, the value the constructor starts it at.
     *
     * The body is inline. The one out-of-line copy has no caller. The title is inferred.
     *
     * @ghidraAddress 0x001e1958
     */
    void SetUnknown34() {
        mUnknown34 = 1;
    }

    /**
     * Clear mUnknown34.
     *
     * The body is inline. GameManagerImpl's constructor expands it at `0x00106024`, and the one
     * out-of-line copy has no caller. The title is inferred.
     *
     * @ghidraAddress 0x001e1968
     */
    void ClearUnknown34() {
        mUnknown34 = 0;
    }

    /**
     * Report mUnknown50.
     *
     * The body is inline, and the one out-of-line copy has no caller. The title is inferred.
     *
     * @return The word at `+0x50`.
     * @ghidraAddress 0x001e1980
     */
    int GetUnknown50() {
        return mUnknown50;
    }

    /**
     * Report whether the last Poll() sent a reading out.
     *
     * The body is inline. GameManagerImpl::PollPlayback() expands it, and the one out-of-line copy
     * has no caller. The title is inferred.
     *
     * @return mPressedThisPoll.
     * @ghidraAddress 0x001e1988
     */
    int GetPressedThisPoll() {
        return mPressedThisPoll;
    }

private:
    // Non-zero when the last Poll() sent a reading out. The reading routine at 0x001dfab0 clears
    // it on entry and sets it at 0x001dfd5c and 0x001dfda8. +0x38
    int mPressedThisPoll;

    // Starts at 1. SetUnknown34() and ClearUnknown34() are the only recovered writers, and its
    // purpose is unrecovered. +0x34
    int mUnknown34;

    // Words per Entry. The setup routine at 0x001df248 zeroes exactly this many with a word loop,
    // and the table of controller bit masks that 0x001e19b8 builds at 0x008efb60 has the same
    // count, which is what suggests one word per control. The correspondence is an inference.
    static constexpr int kEntryWordCount = 16;

    // Trailing bytes per Entry, zeroed one byte at a time rather than as a word, which is what
    // establishes them as byte-wide members rather than a single word.
    static constexpr int kEntryByteCount = 4;

    // One record per controller the setup routine finds. The class emits no RTTI, has no
    // constructor or destructor of its own, and is copied into the vector byte for byte by the
    // compiler, so no name for it survives anywhere in the image. Its allocation is billed to
    // `stl_vector` because the vector owns it. The title here records only that it is an element of
    // that vector.
    struct Entry {
        int mUnknown00[kEntryWordCount];  // +0x00
        char mUnknown40[kEntryByteCount]; // +0x40
        int mUnknown44;                   // +0x44
        int mUnknown48;                   // +0x48
    };

    std::vector<Entry> mUnknown00; // +0x00 element stride 0x4c
    std::vector<int> mUnknown0c;   // +0x0c element type not recovered
    // Incremented once per record the setup routine appends to mUnknown1c.
    int mUnknown18;              // +0x18
    std::vector<int> mUnknown1c; // +0x1c holds pointers to 8-byte polymorphic objects the setup
                                 // routine builds, whose class is not recovered
    // Set from the return of 0x00558d10, which is titled as a static-initialisation stub and
    // cannot be one, because a stub does not return a value a caller stores.
    int mUnknown28;            // +0x28
    std::list<int> mUnknown2c; // +0x2c element type not recovered, 16-byte node
    int mUnknown30;            // +0x30
    int mActive;               // +0x3c starts at 1
    int mUnknown40;            // +0x40
    int mUnknown44;            // +0x44
    int mPaused;               // +0x48
    // Starts at 1. The reading routine at 0x001dfab0 tests it at 0x001dfb7c before it hands a
    // reading to a game world. +0x4c
    int mGameInputEnabled;
    int mUnknown50; // +0x50
    // The receiver of the readings, which SetController() installs.
    RawController *mController; // +0x54
};
