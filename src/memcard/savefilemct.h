#pragma once

#include <libmc.h>

#ifdef VIDEO_STANDARD_PAL
#include <vector>
#endif

#include "memcard/memcardtask.h"
#include "os/hxstr.h"

#ifdef VIDEO_STANDARD_PAL
// The header of SaveSpaceCheckerMCT includes this one for SaveFileMCT and SaveFileEntry.
class SaveSpaceCheckerMCT;

/**
 * Steps SaveFileMCT::mStep selects between in the European release.
 *
 * The member stores the step to run next. SaveFileMCT::RunStep() dispatches through the six-entry
 * jump table at `0x0081e000`. Any other value returns without work.
 *
 * Every title is inferred from the body the table entry addresses.
 */
enum SaveFileStep {
    kSaveFileStepCheckSpace = 0, /*!< Start a SaveSpaceCheckerMCT on the save directory. */
    kSaveFileStepCreateDir = 1,  /*!< Delete the checker and create the save directory. */
    kSaveFileStepOpenFile = 2,   /*!< Open `<dir>/<name>` of the current SaveFileEntry. */
    kSaveFileStepWriteFile = 3,  /*!< Write the current SaveFileEntry, then close. */
    kSaveFileStepReport = 4,     /*!< Call Finish(). */
    kSaveFileStepDone = 5        /*!< Terminal. */
};

/**
 * One file SaveFileMCT writes into the save directory.
 *
 * The European release replaces the fixed write sequence with a vector of SaveFileEntry records.
 * The field titles are inferred.
 */
struct SaveFileEntry {
    /** The file name, without the directory or a leading `/`. +0x00 */
    HxStr mName;

    /** The bytes to write. +0x08 */
    const void *mData;

    /** The number of bytes to write. +0x0c */
    int mLength;
};
#else
/**
 * Steps SaveFileMCT::mStep selects between.
 *
 * The member stores the step to run next, and every step body ends by writing the step after it.
 * SaveFileMCT::RunStep() dispatches through the eleven-entry jump table at `0x007da140`. Any other
 * value returns without work.
 *
 * Every title is inferred from the body the table entry addresses. No string in the image
 * identifies any of them.
 */
enum SaveFileStep {
    kSaveFileStepCreateDir = 0,      /*!< Create the save directory. */
    kSaveFileStepOpenIconSys = 1,    /*!< Open `<dir>/icon.sys` for writing. */
    kSaveFileStepWriteIconSys = 2,   /*!< Build the icon header and write it, then close. */
    kSaveFileStepOpenIconImage = 3,  /*!< Open `<dir>/freq1.ico` for writing. */
    kSaveFileStepWriteIconImage = 4, /*!< Write the loaded icon mesh, then close. */
    kSaveFileStepOpenMarker = 5,     /*!< Open `<dir><dir>` for writing. */
    kSaveFileStepWriteMarker = 6,    /*!< Write the two-byte marker, then close. */
    kSaveFileStepOpenData = 7,       /*!< Open `<dir><file>` for writing. */
    kSaveFileStepWriteData = 8,      /*!< Write the payload, then close. */
    kSaveFileStepReport = 9,         /*!< Call Finish(). */
    kSaveFileStepDone = 10           /*!< Terminal. The body only releases its temporary. */
};

/**
 * Free clusters SavePersonasMCT, SaveGlobalSettingsMCT, and SaveJukeboxPlayListMCT require before
 * they save.
 */
constexpr int kSaveFileMinimumFreeClusters = 60;
#endif

/** Bytes the Shift-JIS title buffer reserves on the stack for BuildIconSys() to fill. */
constexpr int kIconTitleBufferSize = 64;

/**
 * Write one payload to a card, together with the save directory and its browser icon.
 *
 * Its RTTI descriptor is at `0x008ef590`. It has single inheritance from `MemcardTask` at offset 0.
 * `SavePersonasMCT::~SavePersonasMCT()` pins an instance at 0x40c bytes by placing the first
 * SavePersonasMCT member at 0x40c. The vtable is at `0x007daed8`.
 *
 * The task is a nine-step sequence, driven one step per operation report. It creates the
 * directory, writes `icon.sys`, writes `freq1.ico`, writes a two-byte marker file under the
 * directory's name, and finally writes the payload. mSkipIconFiles jumps directly from the
 * directory creation to the payload. A save into a directory that already has an icon takes this
 * path.
 *
 * Three subclasses exist, `SavePersonasMCT`, `SaveGlobalSettingsMCT`, and
 * `SaveJukeboxPlayListMCT`. `SaveRemixMCT` instead has one of these tasks and receives its report
 * as a `MemcardUser`.
 *
 * The European release rewrites the task. An instance is 0x41c bytes, from the allocation in
 * `DeleteRemixMCT`, and the vtable is at `0x0081ed98`. Execute() lists the payload, the marker
 * file, `freq1.ico`, and `icon.sys` as SaveFileEntry records in mFiles and enquires about the card.
 * OnCheckInfo() records the free clusters, and a SaveSpaceCheckerMCT then measures the kilobytes
 * the files need beyond the space the directory's present files occupy. A card without that room
 * fails with kMemcardStatusCardFull and the missing kilobytes in mKilobytes. Every save task passes
 * mKilobytes on to its MemcardUser. Otherwise the task creates the directory and writes each file
 * in turn. mSkipIconFiles is stored but has no effect. `SaveRemixMCT` derives from the class in
 * that release.
 *
 * The method titles Save(), RunStep(), and BuildIconSys() are inferred. No string in the image
 * identifies any of them.
 */
class SaveFileMCT : public MemcardTask {
public:
    /**
     * Construct an idle save task.
     *
     * The compiler inlined this body into all three subclass constructors. No North American
     * address of the body remains, and the European release also has an out-of-line copy.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @ghidraAddress PAL: 0x00189900
     */
    SaveFileMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x00184658
     * @ghidraAddress PAL: 0x00189968
     */
    virtual ~SaveFileMCT();

    /**
     * Record what to write and start the sequence.
     *
     * The European release records fileName, pData, and nLength as the one SaveFileEntry of mFiles,
     * replacing any earlier entries, and starts through the virtual Execute().
     *
     * @param dirName The save directory, also the name of the marker file written inside it.
     * @param fileName The payload file, appended to dirName.
     * @param iconTitle The text the browser shows for the save.
     * @param pData The payload.
     * @param nLength The payload length.
     * @param bSkipIconFiles Non-zero to write the payload alone, with no icon files. The European
     *                       release stores it and otherwise ignores it.
     * @ghidraAddress NTSC-U/C: 0x001859e8
     * @ghidraAddress PAL: 0x0017a1c0
     */
    void Save(const HxStr &dirName,
              const HxStr &fileName,
              const HxStr &iconTitle,
              const void *pData,
              int nLength,
              int bSkipIconFiles);

    /**
     * Run the step mStep selects and advance to the step after it.
     *
     * @ghidraAddress NTSC-U/C: 0x001776e8
     * @ghidraAddress PAL: 0x0017a6f0
     */
    void RunStep();

    /**
     * Fill mIconSys with the browser icon header.
     *
     * The background is a four-corner gradient at half intensity, the three light directions and
     * colours are file-scope constants, and all three icon file-name fields receive `freq1.ico`.
     * The title is converted from ASCII to Shift-JIS first, because `icon.sys` stores it that way.
     *
     * @param pszTitle The text the browser shows, in ASCII.
     * @ghidraAddress NTSC-U/C: 0x00177ca0
     * @ghidraAddress PAL: 0x0017ab40
     */
    void BuildIconSys(const char *pszTitle);

#ifdef VIDEO_STANDARD_PAL
    /**
     * Record the free clusters and start the space check, or abandon the task.
     *
     * A status other than kMemcardStatusNotFormatted and kMemcardStatusUnknown records
     * CheckInfoOp::mFree in mFreeClusters and runs kSaveFileStepCheckSpace. Either of those two
     * statuses abandons the task with that status.
     *
     * @param pOp The finished enquiry.
     * @ghidraAddress PAL: 0x0018b5d8
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /**
     * Take the kilobytes the files need, and abandon the task when the card is short of them.
     *
     * mKilobytes receives nKilobytes. When mFreeClusters is smaller, mKilobytes receives the
     * shortfall instead and the task finishes with kMemcardStatusCardFull. Otherwise
     * kSaveFileStepCreateDir runs. SaveSpaceCheckerMCT::OnListDir() has this body inlined, and the
     * out-of-line copy is never called. The name is inferred.
     *
     * @param nKilobytes The kilobytes the files need, from SaveSpaceCheckerMCT.
     * @ghidraAddress PAL: 0x0018b660
     */
    void OnSpaceChecked(int nKilobytes);

    /**
     * Start a SaveSpaceCheckerMCT over mFiles and mDirName and store it in mSpaceChecker.
     *
     * RunStep() has this body inlined, and the out-of-line copy is never called. The name is
     * inferred.
     *
     * @ghidraAddress PAL: 0x0018b6d8
     */
    void StartSpaceCheck();
#endif

    /**
     * @ghidraAddress NTSC-U/C: 0x00185c20
     * @ghidraAddress PAL: 0x0018b558
     */
    virtual void OnCreateDir(CreateDirOp *pOp);

    /**
     * @ghidraAddress NTSC-U/C: 0x00185b40
     * @ghidraAddress PAL: 0x0018b4a8
     */
    virtual void OnWrite(WriteOp *pOp);

    /**
     * @ghidraAddress NTSC-U/C: 0x00185ae0
     * @ghidraAddress PAL: 0x0018b448
     */
    virtual void OnOpenWrite(OpenWriteOp *pOp);

    /**
     * @ghidraAddress NTSC-U/C: 0x00185bc0
     * @ghidraAddress PAL: 0x0018b4f8
     */
    virtual void OnClose(CloseOp *pOp);

    /**
     * @ghidraAddress NTSC-U/C: 0x00185a88
     * @ghidraAddress PAL: 0x0018b410
     */
    virtual void Finish();

    /**
     * Start the sequence.
     *
     * The European release first builds the icon header, appends the marker file, `freq1.ico`, and
     * `icon.sys` to mFiles, and enquires about the card. Each subclass appends its payload before
     * calling it.
     *
     * @ghidraAddress NTSC-U/C: 0x00185ac0
     * @ghidraAddress PAL: 0x0017a390
     */
    virtual void Execute();

protected:
#ifdef VIDEO_STANDARD_PAL
    // The kilobytes the files need, or once a check fails the kilobytes the card lacks. +0x1c
    int mKilobytes;

    // The descriptor the current step writes through. +0x20
    int mFile;

    // The browser icon header, built in place and written as one 964-byte block. +0x24
    sceMcIconSys mIconSys;

    // The files to write, in order. +0x3e8
    std::vector<SaveFileEntry> mFiles;

    // The save directory. Without its leading `/` it is also the marker file name. +0x3f4
    HxStr mDirName;

    // The text the browser shows. +0x3fc
    HxStr mIconTitle;

    // One of SaveFileStep, storing the step to run next. +0x404
    int mStep;

    // The space check, deleted by kSaveFileStepCreateDir. +0x408
    SaveSpaceCheckerMCT *mSpaceChecker;

    // Non-zero to write the payload alone. Stored by Save() and never acted on. +0x40c
    int mSkipIconFiles;

    // Set to 1 by Execute() and never read. +0x410
    int mExecuted;

    // The index in mFiles of the file being written. +0x414
    int mFileIndex;

    // The free clusters the card enquiry reported. +0x418
    int mFreeClusters;
#else
    // The descriptor the current step writes through. +0x1c
    int mFile;

    // The browser icon header, built in place and written as one 964-byte block. +0x20
    sceMcIconSys mIconSys;

    // The save directory, and the name of the marker file written inside it. +0x3e4
    HxStr mDirName;

    // The payload file, appended to mDirName. +0x3ec
    HxStr mFileName;

    // The text the browser shows. +0x3f4
    HxStr mIconTitle;

    // One of SaveFileStep, storing the step to run next. +0x3fc
    int mStep;

    // The payload. +0x400
    const void *mData;

    // The payload length. +0x404
    int mDataLength;

    // Non-zero to write the payload alone. +0x408
    int mSkipIconFiles;
#endif
};
