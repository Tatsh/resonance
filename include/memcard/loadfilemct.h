#pragma once

#include "memcard/memcardtask.h"
#include "os/hxstr.h"

/**
 * States LoadFileMCT::mState selects between.
 *
 * LoadFileMCT::RunStep() tests the member against these three values and returns without work for
 * any other, which is what makes kLoadFileStateDone terminal. The four values are 20 apart and
 * nothing in the image explains the spacing.
 *
 * Every title is inferred from the branch the value selects. No string in the image identifies any
 * of them.
 */
enum LoadFileState {
    kLoadFileStateOpen = 0x7b,   /*!< Open the file for reading. */
    kLoadFileStateRead = 0x8f,   /*!< Read the payload, then close. */
    kLoadFileStateReport = 0xa3, /*!< Call Finish(). */
    kLoadFileStateDone = 0xb7    /*!< Terminal. RunStep() performs no work. */
};

/**
 * Read one file from a card into a caller-supplied buffer.
 *
 * `11LoadFileMCT` in the RTTI descriptor at `0x0090e2b0`, single inheritance from `MemcardTask` at
 * offset 0. An instance is 0x38 bytes and the vtable is at `0x007dae38`.
 *
 * The task enquires about the card, opens the file, reads it, and closes it, advancing one step
 * per operation report. Three subclasses exist, `LoadPersonasMCT`, `LoadGlobalSettingsMCT` and
 * `LoadJukeboxPlayListMCT`. `LoadRemixMCT` and `ListRemixesMCT` instead own one of these tasks and
 * receive its report as a `MemcardUser`.
 *
 * The enquiry tolerates every status except kMemcardStatusUnknown, so a load from an unformatted
 * card still proceeds to the open and fails there instead.
 *
 * The method titles Load(), RunStep() and SetState() are inferred. No string in the image
 * identifies any of them.
 */
class LoadFileMCT : public MemcardTask {
public:
    /**
     * Construct an idle load task.
     *
     * The compiler inlined this body into all three subclass constructors, so no address of its
     * own survives.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     */
    LoadFileMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie);

    /** @ghidraAddress 0x001847e0 */
    virtual ~LoadFileMCT();

    /**
     * Record what to read and start the sequence.
     *
     * @param path The file to read.
     * @param pBuffer The destination.
     * @param nLength The number of bytes to read.
     * @ghidraAddress 0x00185e20
     */
    void Load(const HxStr &path, void *pBuffer, int nLength);

    /**
     * Run the step mState selects and advance to the state after it.
     *
     * @ghidraAddress 0x00185f08
     */
    void RunStep();

    /**
     * Record the next state.
     *
     * The body is a single store. It survives as one out-of-line copy because this compiler emits
     * an inline member into every translation unit that needs it, and the copy the linker retained
     * came from another unit.
     *
     * @param nState One of LoadFileState.
     * @ghidraAddress 0x001f61b0
     */
    void SetState(int nState);

    /** @ghidraAddress 0x00185db8 */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /** @ghidraAddress 0x00185d00 */
    virtual void OnRead(ReadOp *pOp);

    /** @ghidraAddress 0x00185ca0 */
    virtual void OnOpenRead(OpenReadOp *pOp);

    /** @ghidraAddress 0x00185d58 */
    virtual void OnClose(CloseOp *pOp);

    /** @ghidraAddress 0x00185ec0 */
    virtual void Finish();

    /** @ghidraAddress 0x00185e80 */
    virtual void Execute();

    /**
     * Bytes the read transferred, valid once the task has reported kMemcardStatusOk.
     *
     * Public because the derived tasks and the screens that own them read it directly, and the
     * image exposes no accessor.
     *
     * +0x34
     */
    int mBytesRead;

private:
    // One of LoadFileState. +0x1c
    int mState;

    // The destination. +0x20
    void *mBuffer;

    // The number of bytes to read. +0x24
    int mLength;

    // The file to read. +0x28
    HxStr mPath;

    // The descriptor the read and the close use. +0x30
    int mFile;
};
