#ifndef LIBCDVD_H
#define LIBCDVD_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The drive library: disc reads, seeks, the tray, the real-time clock, the ISO 9660 file search,
 * and sector streaming, each sent to the drive servers on the IOP by RPC.
 */

/** Modes of sceCdInit(). */
enum {
    SCECdINIT = 0, /*!< Initialise the library and wait for the drive. */
    SCECdINoD = 1, /*!< Initialise the library without waiting for a disc. */
    SCECdEXIT = 5, /*!< Release the semaphores and the power-off handler. */
};

/** Modes of sceCdSync(), sceCdSyncS(), and sceCdDiskReady(). */
enum {
    SCECdBlock = 0,    /*!< Wait until the drive is free. */
    SCECdNonblock = 1, /*!< Report the drive state at once. */
};

/** Results of sceCdDiskReady(). */
enum {
    SCECdComplete = 2, /*!< The drive is ready for a command. */
    SCECdNotReady = 6, /*!< The drive is busy or does not have a readable disc. */
};

/** Disc types sceCdGetDiskType() reports. */
enum {
    SCECdNODISC = 0x00,  /*!< No disc. */
    SCECdPS2CD = 0x12,   /*!< PlayStation 2 compact disc without audio tracks. */
    SCECdPS2CDDA = 0x13, /*!< PlayStation 2 compact disc with audio tracks. */
    SCECdPS2DVD = 0x14,  /*!< PlayStation 2 DVD. */
};

/** Drive errors sceCdGetError() reports. */
enum {
    SCECdErTRMOPN = 0x31, /*!< The tray opened during the command. */
};

/** Requests of sceCdTrayReq(). */
enum {
    SCECdTrayOpen = 0,  /*!< Open the tray. */
    SCECdTrayClose = 1, /*!< Close the tray. */
    SCECdTrayCheck = 2, /*!< Report whether the tray has moved. */
};

/** Media of sceCdMmode(). */
enum {
    SCECdCD = 1,  /*!< Compact disc. */
    SCECdDVD = 2, /*!< DVD. */
};

/** Function codes the completion callback receives. */
enum {
    SCECdFuncRead = 1,     /*!< sceCdRead() finished. */
    SCECdFuncReadCDDA = 2, /*!< A digital audio read finished. */
    SCECdFuncGetToc = 3,   /*!< A table of contents read finished. */
    SCECdFuncSeek = 4,     /*!< sceCdSeek() finished. */
    SCECdFuncStandby = 5,  /*!< A standby request finished. */
    SCECdFuncStop = 6,     /*!< sceCdStop() finished. */
    SCECdFuncPause = 7,    /*!< A pause request finished. */
    SCECdFuncBreak = 8,    /*!< A break request finished. */
};

/** Sector sizes of sceCdRMode::datapattern. */
enum {
    SCECdSecS2048 = 0, /*!< 2048-byte user data. */
    SCECdSecS2328 = 1, /*!< 2328-byte sectors. */
    SCECdSecS2340 = 2, /*!< 2340-byte sectors. */
};

/** Modes of sceCdStRead(). */
enum {
    STMNBLK = 0, /*!< Return with the sectors already buffered. */
    STMBLK = 1,  /*!< Wait until every requested sector has arrived. */
};

/** How a read runs. */
typedef struct {
    unsigned char trycount;    /*!< Retries on a read error, where zero means the default. */
    unsigned char spindlctrl;  /*!< Spindle speed control. */
    unsigned char datapattern; /*!< Sector size, one of the SCECdSecS values. */
    unsigned char pad;         /*!< Padding. */
} sceCdRMode;

/** A file record sceCdSearchFile() fills. */
typedef struct {
    unsigned int lsn;      /*!< First sector of the file. */
    unsigned int size;     /*!< Size in bytes. */
    char name[16];         /*!< File name. */
    unsigned char date[8]; /*!< Recording date. */
    unsigned int flag;     /*!< Directory record flags. */
} sceCdlFILE;

/** The real-time clock, each field in binary-coded decimal. */
typedef struct {
    unsigned char stat;   /*!< Read status. */
    unsigned char second; /*!< Second. */
    unsigned char minute; /*!< Minute. */
    unsigned char hour;   /*!< Hour. */
    unsigned char pad;    /*!< Padding. */
    unsigned char day;    /*!< Day of the month. */
    unsigned char month;  /*!< Month. */
    unsigned char year;   /*!< Year within the century. */
} sceCdCLOCK;

/** A completion callback. It receives the function code of the finished command. */
typedef void (*sceCdCBFunc)(int function);

/** A power-off callback. It receives the argument given to sceCdPOffCallback(). */
typedef void (*sceCdPOffFunc)(void *addr);

/**
 * Initialise or release the library.
 *
 * @param init_mode #SCECdINIT, #SCECdINoD, or #SCECdEXIT.
 * @return 1 on success, 2 when the IOP modules predate version 2, and 0 when the drive is busy or
 * the call fails.
 */
int sceCdInit(int init_mode);

/**
 * Select the medium the drive reads.
 *
 * @param media #SCECdCD or #SCECdDVD.
 * @return 1 on success, or 0 when the drive is busy or the call fails.
 */
int sceCdMmode(int media);

/**
 * Wait for, or report, the command in flight on the N-command server.
 *
 * @param mode #SCECdBlock to wait, #SCECdNonblock to report.
 * @return 0 when the server is free, 1 while a command runs.
 */
int sceCdSync(int mode);

/**
 * Wait for, or report, the command in flight on the S-command server.
 *
 * @param mode #SCECdBlock to wait, #SCECdNonblock to report.
 * @return 0 when the server is free, nonzero while a command runs.
 */
int sceCdSyncS(int mode);

/**
 * Report whether the drive can take a command.
 *
 * @param mode #SCECdBlock to wait for the drive, #SCECdNonblock to report at once.
 * @return #SCECdComplete or #SCECdNotReady. A busy library reports #SCECdNotReady, or -1 for
 * mode 8.
 */
int sceCdDiskReady(int mode);

/**
 * Report the type of the disc in the drive.
 *
 * @return One of the SCECdNODISC family of values, or 0 when the drive is busy or the call fails.
 */
int sceCdGetDiskType(void);

/**
 * Report the error of the last drive command.
 *
 * @return One of the SCECdEr values, or -1 when the drive is busy or the call fails.
 */
int sceCdGetError(void);

/**
 * Open, close, or query the tray.
 *
 * @param param #SCECdTrayOpen, #SCECdTrayClose, or #SCECdTrayCheck.
 * @param traycnt Receives the tray movement report, or null.
 * @return 1 on success, 0 when the drive refused, or -1 when the drive is busy or the call fails.
 */
int sceCdTrayReq(int param, unsigned int *traycnt);

/**
 * Start reading sectors. Completion is reported through sceCdSync() and the callback.
 *
 * @param lsn First sector.
 * @param sectors Number of sectors.
 * @param buf Destination in main memory.
 * @param mode Read mode.
 * @return 1 when the read started, or 0 when the drive is busy or the call fails.
 */
int sceCdRead(unsigned int lsn, unsigned int sectors, void *buf, sceCdRMode *mode);

/**
 * Start moving the head to a sector. Completion is reported through sceCdSync() and the callback.
 *
 * @param lsn Target sector.
 * @return 1 when the seek started, or 0 when the drive is busy or the call fails.
 */
int sceCdSeek(unsigned int lsn);

/**
 * Start stopping the spindle. Completion is reported through sceCdSync() and the callback.
 *
 * @return 1 when the stop started, or 0 when the drive is busy or the call fails.
 */
int sceCdStop(void);

/**
 * Read the real-time clock.
 *
 * @param rtc Receives the clock.
 * @return 1 on success, or 0 when the drive is busy or the call fails.
 */
int sceCdReadClock(sceCdCLOCK *rtc);

/**
 * Look up a file on the disc.
 *
 * @param fp Receives the file record.
 * @param name Path on the disc, at most 256 bytes.
 * @return 1 when found, or 0 when absent, when the drive is busy, or when the call fails.
 */
int sceCdSearchFile(sceCdlFILE *fp, const char *name);

/**
 * Start the thread that runs the completion callback, or change its priority when it exists.
 *
 * @param priority Thread priority.
 * @param stack Lowest address of the thread stack.
 * @param stacksize Stack size in bytes.
 * @return 1 when the thread was created, 0 when only its priority changed.
 */
int sceCdInitEeCB(int priority, void *stack, int stacksize);

/**
 * Register the completion callback. It runs on the thread sceCdInitEeCB() starts.
 *
 * @param func Callback, or null.
 * @return The previous callback, or null when a command is in flight and the callback is unchanged.
 */
sceCdCBFunc sceCdCallback(sceCdCBFunc func);

/**
 * Register the callback the IOP's power-off notice runs.
 *
 * @param func Callback, or null.
 * @param addr Argument the callback receives.
 * @return The previous callback.
 */
sceCdPOffFunc sceCdPOffCallback(sceCdPOffFunc func, void *addr);

/**
 * Prepare sector streaming into a ring buffer in IOP memory.
 *
 * @param bufmax Ring buffer size in sectors.
 * @param bankmax Number of banks the ring divides into.
 * @param iop_bufaddr IOP address of the ring buffer.
 * @return The server's reply, or 0 when the drive is busy or the call fails.
 */
int sceCdStInit(unsigned int bufmax, unsigned int bankmax, unsigned int iop_bufaddr);

/**
 * Start streaming from a sector.
 *
 * @param lsn First sector.
 * @param mode Read mode.
 * @return The server's reply, or 0 when the drive is busy or the call fails.
 */
int sceCdStStart(unsigned int lsn, sceCdRMode *mode);

/**
 * Move the stream to a sector without discarding the buffered sectors.
 *
 * @param lsn Target sector.
 * @return The server's reply, or 0 when the drive is busy or the call fails.
 */
int sceCdStSeekF(unsigned int lsn);

/**
 * Move the stream to a sector.
 *
 * @param lsn Target sector.
 * @return The server's reply, or 0 when the drive is busy or the call fails.
 */
int sceCdStSeek(unsigned int lsn);

/**
 * Stop streaming.
 *
 * @return The server's reply, or 0 when the drive is busy or the call fails.
 */
int sceCdStStop(void);

/**
 * Copy streamed sectors into main memory.
 *
 * @param size Number of sectors.
 * @param buf Destination.
 * @param mode #STMNBLK or #STMBLK.
 * @param err Receives the drive error, or 0.
 * @return Number of sectors copied.
 */
int sceCdStRead(unsigned int size, unsigned int *buf, unsigned int mode, unsigned int *err);

/**
 * Pause streaming.
 *
 * @return The server's reply, or 0 when the drive is busy or the call fails.
 */
int sceCdStPause(void);

/**
 * Resume streaming after sceCdStPause().
 *
 * @return The server's reply, or 0 when the drive is busy or the call fails.
 */
int sceCdStResume(void);

/**
 * Report the number of sectors buffered in the stream.
 *
 * @return The server's reply, or 0 when the drive is busy or the call fails.
 */
int sceCdStStat(void);

#ifdef __cplusplus
}
#endif

#endif
