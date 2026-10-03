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
    kMinimumSaveSpaceStepReport = 5         /*!< Report after the settings file closes uncharged. */
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
 * Its RTTI descriptor is at `0x008ef280`. It has single inheritance from `MemcardTask` at offset 0.
 * An instance is 0x38 bytes and the vtable is at `0x007dac58`. In the European release an instance
 * is 0x40 bytes and the vtable is at `0x0081eaa0`.
 *
 * The task probes rather than counts. It tries to open the roster file and the settings file in
 * turn and charges kMinimumSaveSpaceExistingFile for each one that opens and
 * kMinimumSaveSpaceNewFile for each one that does not, starting from a base the game supplies.
 * The total goes to the user through `MemcardUser::OnMinimumSaveSpace()`, the only notification
 * that receives a measurement rather than a status.
 *
 * The task never reads the card's free-cluster count. Its OnCheckInfo() ignores the finished
 * enquiry entirely, not even reading its status. `SaveRemixMCT` is the only task in the subsystem
 * that reads `CheckInfoOp::mFree`. The asymmetry matches the shipped program.
 *
 * Both paths are built from the file-scope string globals at `0x00888940` onward. The roster path
 * is `/BASCUS-97125` plus `gen` plus `/pers.dat` and the settings path is `/BASCUS-97125` plus
 * `glo` plus `/globset.dat`.
 */
class MinimumSaveSpaceMCT : public MemcardTask {
public:
    /**
     * Construct an idle measurement.
     *
     * The constructor is inlined at every site and the image has no out-of-line copy.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     */
    MinimumSaveSpaceMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x00184c18
     * @ghidraAddress PAL: 0x0018a140
     */
    virtual ~MinimumSaveSpaceMCT();

    /**
     * Run the step mStep selects and advance to the step after it.
     *
     * @ghidraAddress NTSC-U/C: 0x00178660
     * @ghidraAddress PAL: 0x0017bd20
     */
    void RunStep();

    /**
     * Ignore the finished enquiry and run the next step.
     *
     * The body does not read the operation, not even its status. The free-cluster count therefore
     * plays no part in the measurement.
     *
     * @ghidraAddress NTSC-U/C: 0x00186200
     * @ghidraAddress PAL: 0x0018bcc0
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /**
     * @ghidraAddress NTSC-U/C: 0x00186220
     * @ghidraAddress PAL: 0x0018bce0
     */
    virtual void OnOpenRead(OpenReadOp *pOp);

    /**
     * @ghidraAddress NTSC-U/C: 0x00186250
     * @ghidraAddress PAL: 0x0018bd10
     */
    virtual void OnClose(CloseOp *pOp);

    /**
     * Report mSpace through MemcardUser::OnMinimumSaveSpace().
     *
     * The European release also passes mSkipWarning and mCampaign.
     *
     * @ghidraAddress NTSC-U/C: 0x001861c0
     * @ghidraAddress PAL: 0x0018bc78
     */
    virtual void Finish();

    /**
     * Build both paths, seed the measurement, and run the first step.
     *
     * The body seeds mSpace from GlobalSettings::mRequiredSaveSpace less 100, builds both paths,
     * clears mStep, and runs RunStep(). The European release subtracts 87 instead, and puts a `/`
     * between each directory and its file name.
     *
     * @ghidraAddress NTSC-U/C: 0x00178328
     * @ghidraAddress PAL: 0x0017b740
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

#ifdef VIDEO_STANDARD_PAL
    // Passed to MemcardUser::OnMinimumSaveSpace() as nSkipWarning. Only the constructor writes it,
    // with zero. +0x38
    int mSkipWarning;

    // Passed to MemcardUser::OnMinimumSaveSpace() as nCampaign. Only the constructor writes it,
    // with zero. +0x3c
    int mCampaign;
#endif
};
