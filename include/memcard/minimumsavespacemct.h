#pragma once

#include "memcard/memcardtask.h"
#include "os/hxstr.h"

/**
 * Steps MinimumSaveSpaceMCT::mStep selects between.
 *
 * RunStep() dispatches through the six-entry jump table at `0x007da200`, and any other value
 * returns without work. Every title is inferred from the body the entry addresses.
 */
enum MinimumSaveSpaceStep {
    kMinimumSaveSpaceStepCheckInfo = 0,     /*!< Enquire about the card. */
    kMinimumSaveSpaceStepOpenPersonas = 1,  /*!< Open the roster file for reading. */
    kMinimumSaveSpaceStepAfterPersonas = 2, /*!< Charge for the roster, then the settings. */
    kMinimumSaveSpaceStepOpenSettings = 3,  /*!< Open the settings file for reading. */
    kMinimumSaveSpaceStepAfterSettings = 4, /*!< Charge for the settings, then report. */
    kMinimumSaveSpaceStepReport = 5         /*!< Charge for a missing file and report. */
};

/**
 * Clusters a save would need if a file already exists on the card.
 *
 * Inferred from the two additions. No string in the image identifies either figure.
 */
constexpr int kMinimumSaveSpaceExistingFile = 10;

/** Clusters a save would need if a file has to be created. */
constexpr int kMinimumSaveSpaceNewFile = 50;

/**
 * Measure the space a full save would need on one card.
 *
 * `19MinimumSaveSpaceMCT` in the RTTI descriptor at `0x008ef280`, single inheritance from
 * `MemcardTask` at offset 0. An instance is 0x38 bytes and the vtable is at `0x007dac58`.
 *
 * The task probes rather than counts. It tries to open the roster file and the settings file in
 * turn and charges kMinimumSaveSpaceExistingFile for each one that opens and
 * kMinimumSaveSpaceNewFile for each one that does not, starting from a base the game supplies.
 * The total goes to the user through `MemcardUser::OnMinimumSaveSpace()`, which is the one
 * notification that receives a measurement rather than a status.
 *
 * Notably the task never reads the card's free-cluster count. Its OnCheckInfo() ignores the
 * finished enquiry entirely, not even reading its status. `SaveRemixMCT` is the only task in the
 * subsystem that reads `CheckInfoOp::mFree`, so the asymmetry is real rather than a gap in this
 * reconstruction.
 *
 * Both paths are built from the file-scope string globals at `0x00888940` onward, so the roster
 * path is `/BASCUS-97125` plus `gen` plus `/pers.dat` and the settings path is `/BASCUS-97125`
 * plus `glo` plus `/globset.dat`.
 */
class MinimumSaveSpaceMCT : public MemcardTask {
public:
    /**
     * Construct an idle measurement.
     *
     * The constructor is inlined at every site and no address of its own survives.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     */
    MinimumSaveSpaceMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie);

    /** @ghidraAddress 0x00184c18 */
    virtual ~MinimumSaveSpaceMCT();

    /**
     * Run the step mStep selects and advance to the step after it.
     *
     * @ghidraAddress 0x00178660
     */
    void RunStep();

    /**
     * Ignore the finished enquiry and run the next step.
     *
     * The body reads nothing from the operation, not even its status, which is what establishes
     * that the free-cluster count plays no part in the measurement.
     *
     * @ghidraAddress 0x00186200
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /** @ghidraAddress 0x00186220 */
    virtual void OnOpenRead(OpenReadOp *pOp);

    /** @ghidraAddress 0x00186250 */
    virtual void OnClose(CloseOp *pOp);

    /** @ghidraAddress 0x001861c0 */
    virtual void Finish();

    /**
     * Build both paths, seed the measurement, and run the first step.
     *
     * Understood but not written. The body seeds mSpace from a game-wide singleton vended by
     * `0x0018b9c8`, which returns the pointer stored at `0x0067ea38` and reads a count at its
     * `+0x6c` before subtracting 100. That singleton belongs to no reconstructed subsystem yet, so
     * its type is unavailable and the body waits on it. Everything else the body does is recovered
     * and recorded in the class documentation above.
     *
     * @ghidraAddress 0x00178328
     */
    virtual void Execute();

private:
    // One of MinimumSaveSpaceStep, storing the step to run next. +0x1c
    int mStep;

    // The descriptor the current open delivered, negative when the file is absent. +0x20
    int mFile;

    // Clusters the save is estimated to need. Reported to the user by Finish(). +0x24
    int mSpace;

    // `/BASCUS-97125` plus `gen` plus `/pers.dat`. +0x28
    HxStr mPersonaPath;

    // `/BASCUS-97125` plus `glo` plus `/globset.dat`. +0x30
    HxStr mSettingsPath;
};
