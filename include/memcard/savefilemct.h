#pragma once

#include <libmc.h>

#include "memcard/memcardtask.h"
#include "os/hxstr.h"

/**
 * Steps SaveFileMCT::mStep selects between.
 *
 * The member stores the step to run next, and every step body ends by writing the step after it.
 * SaveFileMCT::RunStep() dispatches through the jump table at `0x007da140`, which has eleven
 * entries, so any other value returns without work.
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

/** Bytes the Shift-JIS title buffer reserves, which is the stack run BuildIconSys() fills. */
constexpr int kIconTitleBufferSize = 64;

/**
 * Convert ASCII text to the Shift-JIS bytes `icon.sys` stores a title as.
 *
 * A code the table does not cover is reported through the log as `bad ASCII code 0x%x`. The routine
 * is declared here because `SaveFileMCT::BuildIconSys()` is its only caller in the image, and the
 * name is inferred from that use and from the `.Kanji.` table the body indexes.
 *
 * @param pszAscii The text to convert.
 * @param pszDest The destination, of at least kIconTitleBufferSize bytes.
 * @ghidraAddress 0x00556a20
 */
void AsciiToShiftJis(const char *pszAscii, char *pszDest);

/**
 * Write one payload to a card, together with the save directory and its browser icon.
 *
 * `11SaveFileMCT` in the RTTI descriptor at `0x008ef590`, single inheritance from `MemcardTask` at
 * offset 0. An instance is 0x40c bytes, which `SavePersonasMCT::~SavePersonasMCT()` pins by placing
 * its own first member at 0x40c. The vtable is at `0x007daed8`.
 *
 * The task is a nine-step sequence, driven one step per operation report. It creates the
 * directory, writes `icon.sys`, writes `freq1.ico`, writes a two-byte marker file under the
 * directory's own name, and finally writes the payload. mSkipIconFiles jumps straight from the
 * directory creation to the payload, which is what a save into a directory that already holds an
 * icon does.
 *
 * Three subclasses exist, `SavePersonasMCT`, `SaveGlobalSettingsMCT` and
 * `SaveJukeboxPlayListMCT`. `SaveRemixMCT` instead owns one of these tasks and receives its report
 * as a `MemcardUser`.
 *
 * The method titles Save(), RunStep() and BuildIconSys() are inferred. No string in the image
 * identifies any of them.
 */
class SaveFileMCT : public MemcardTask {
public:
    /**
     * Construct an idle save task.
     *
     * The compiler inlined this body into all three subclass constructors, so no address of its
     * own survives.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     */
    SaveFileMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie);

    /** @ghidraAddress 0x00184658 */
    virtual ~SaveFileMCT();

    /**
     * Record what to write and start the sequence.
     *
     * @param dirName The save directory, which is also the name of the marker file written inside
     *                it.
     * @param fileName The payload file, appended to dirName.
     * @param iconTitle The text the browser shows for the save.
     * @param pData The payload.
     * @param nLength The payload length.
     * @param bSkipIconFiles Non-zero to write the payload alone, with no icon files.
     * @ghidraAddress 0x001859e8
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
     * @ghidraAddress 0x001776e8
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
     * @ghidraAddress 0x00177ca0
     */
    void BuildIconSys(const char *pszTitle);

    /** @ghidraAddress 0x00185c20 */
    virtual void OnCreateDir(CreateDirOp *pOp);

    /** @ghidraAddress 0x00185b40 */
    virtual void OnWrite(WriteOp *pOp);

    /** @ghidraAddress 0x00185ae0 */
    virtual void OnOpenWrite(OpenWriteOp *pOp);

    /** @ghidraAddress 0x00185bc0 */
    virtual void OnClose(CloseOp *pOp);

    /** @ghidraAddress 0x00185a88 */
    virtual void Finish();

    /** @ghidraAddress 0x00185ac0 */
    virtual void Execute();

protected:
    // The descriptor the current step writes through. +0x1c
    int mFile;

    // The browser icon header, built in place and written as one 964-byte block. +0x20
    mcIcon mIconSys;

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
};
