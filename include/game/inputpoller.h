#pragma once

#include <list>
#include <vector>

class Joypad;
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
 * The constructor finishes in Init(), and the destructor begins with Shutdown().
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
     * The Joypad whose player is nPort receives both levels, and a player with no Joypad is
     * ignored. ForceFeedbackMgr::ApplyMotors() is the recovered caller.
     *
     * @param nPort The player, from 1.
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
     * Runs ReadControllers() and then OnUnknown001e1c58(). GameManagerImpl::PollPlayback() is the
     * caller. The title is inferred.
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
     * Do nothing.
     *
     * The body is inline, and the one out-of-line copy has no caller. Its place between the
     * other InputPoller accessor copies is the only evidence for the class. The title is inferred.
     *
     * @ghidraAddress 0x001e1970
     */
    void OnUnknown001e1970() {
    }

    /**
     * Do nothing.
     *
     * Recorded on the same evidence as OnUnknown001e1970(). The title is inferred.
     *
     * @ghidraAddress 0x001e1978
     */
    void OnUnknown001e1978() {
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

    /**
     * Fill the control mask table and open the controllers.
     *
     * Called by the constructor. The table does not depend on the object, and the pointer is
     * forwarded to Setup() unread. The title is inferred.
     *
     * @ghidraAddress 0x001e19b8
     */
    void Init();

    /**
     * Open a Joypad on each of the four multitap slots of port 0 and on slot 0 of port 1.
     *
     * Each Joypad takes the next id and an empty Entry, and mJoypadPlayers gains one zeroed word
     * per Joypad. With a multitap on port 0 the Joypads are numbered as players 1 onward in order,
     * the last one excepted. Without one, the first Joypad is player 1 and, unless a multitap
     * sits on port 1, the port 1 Joypad is player 2. The title is inferred.
     *
     * @ghidraAddress 0x001df248
     */
    void Setup();

    /**
     * Close and delete every Joypad, and empty mJoypads and mEntries.
     *
     * Nothing happens when mJoypads is already empty. The destructor is the caller. The title is
     * inferred.
     *
     * @ghidraAddress 0x001df9d8
     */
    void Shutdown();

    /**
     * Follow a multitap being connected or removed on either port.
     *
     * Does nothing while mActive is clear. A change on port 1 restarts every Joypad. A multitap
     * newly on port 0 renumbers the players in order and restarts every Joypad, and one newly
     * gone restores the single-pad numbering. Named after the file-private
     * FindJoypadConnectionsCmd.
     *
     * @ghidraAddress 0x001df798
     */
    void FindJoypadConnections();

    /**
     * Read every Joypad and send the changes to mController.
     *
     * Each pressed or released control goes out as a `joy ` reading with the Joypad's player,
     * the control number from 1, and 0.99 or 0, and each moved stick axis as a reading with its
     * axis control and its position scaled to 0 through 1. The four face buttons send only
     * the first of them pressed while any stays held. A Joypad that reports 0 during a game pauses
     * the game when its player is one of the world's local players, and one that reports 1 sets
     * mUnknown50. The routine returns at the first ready Joypad that has no player. The title is
     * inferred.
     *
     * @ghidraAddress 0x001dfab0
     */
    void ReadControllers();

    /**
     * Do nothing. Poll() calls it after ReadControllers().
     *
     * @ghidraAddress 0x001e1c58
     */
    void OnUnknown001e1c58();

    /**
     * Restart every Joypad's setup state machine.
     *
     * Inline. FindJoypadConnections() expands it three times, and the out-of-line copy has no
     * caller. The title is inferred.
     *
     * @ghidraAddress 0x001e1a88
     */
    void ResetJoypads();

    /**
     * Number the connected Joypads as players 1 onward, in order.
     *
     * A Joypad that is not connected keeps its player. The routine has no caller. The title is
     * inferred.
     *
     * @ghidraAddress 0x001e1ad8
     */
    void NumberConnectedJoypads();

    // The controls, in the order of the mask table the control numbers index.
    static constexpr int kControlCount = 16;

    // Bytes of stick position per reading, one per axis.
    static constexpr int kAxisCount = 4;

    // One record per Joypad, the last reading it sent. The class emits no RTTI, has no constructor
    // or destructor of its own, and is copied into the vector byte for byte, so no name survives.
    struct Entry {
        int mUnknown00[kControlCount]; // +0x00, zeroed by Setup() and never read
        char mAxes[kAxisCount];        // +0x40
        unsigned int mButtons;         // +0x44
        // Set while a face button's press has gone out, cleared once all four are up.
        int mFaceButtonHeld; // +0x48
    };

    std::vector<Entry> mEntries;     // +0x00
    std::vector<int> mJoypadPlayers; // +0x0c, the player of each Joypad from 1, or 0 for none
    int mNextJoypadId;               // +0x18
    std::vector<Joypad *> mJoypads;  // +0x1c
    // Set from the return of 0x00558d10, which is titled as a static-initialisation stub and
    // cannot be one, because a stub does not return a value a caller stores.
    int mUnknown28;            // +0x28
    std::list<int> mUnknown2c; // +0x2c element type not recovered, 16-byte node
    int mUnknown30;            // +0x30
    int mActive;               // +0x3c starts at 1
    int mMultitap0;            // +0x40, a multitap is on port 0
    int mMultitap1;            // +0x44, a multitap is on port 1
    // Set by SetPaused() and by ReadControllers() when it pauses the game.
    int mPaused; // +0x48
    // Starts at 1. The reading routine at 0x001dfab0 tests it at 0x001dfb7c before it hands a
    // reading to a game world. +0x4c
    int mGameInputEnabled;
    int mUnknown50; // +0x50, set when a Joypad's read reports 1
    // The receiver of the readings, which SetController() installs.
    RawController *mController; // +0x54
};
