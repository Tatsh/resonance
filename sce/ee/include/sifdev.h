#ifndef SIFDEV_H
#define SIFDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SIF DMA and registers, the IOP heap, module loading, IOP reset and reboot, and the file service
 * the IOP provides to the Emotion Engine.
 */

/** One SIF DMA transfer from the Emotion Engine to the IOP. */
typedef struct {
    unsigned int data; /*!< Source address in main memory, 16-byte aligned. */
    unsigned int addr; /*!< Destination address in IOP memory. */
    unsigned int size; /*!< Size in bytes. */
    unsigned int mode; /*!< Transfer attribute bits such as #SIF_DMA_INT_O. */
} sceSifDmaData;

/** Transfer attribute that interrupts the IOP once the transfer ends. */
#define SIF_DMA_INT_O 0x04
/** Transfer attribute that ends the DMA chain with the transfer. */
#define SIF_DMA_ERT 0x40

/** SIF register of the EE command buffer address. The Emotion Engine writes it. */
#define SIF_REG_MAINADDR 1
/** SIF register of the IOP command buffer address. The IOP writes it. */
#define SIF_REG_SUBADDR 2
/** SIF register of flags the Emotion Engine sets for the IOP. */
#define SIF_REG_MSFLAG 3
/** SIF register of flags the IOP sets for the Emotion Engine. Writing a bit clears it. */
#define SIF_REG_SMFLAG 4

/** Software register of the IOP command buffer address the command layer sends to. */
#define SIF_SYSREG_SUBADDR 0x80000000
/** Software register of the address of the EE command layer state. */
#define SIF_SYSREG_MAINADDR 0x80000001
/** Software register that is nonzero once the IOP RPC layer has initialised. */
#define SIF_SYSREG_RPCINIT 0x80000002

/** Bit of #SIF_REG_SMFLAG the IOP sets once SIF has initialised. */
#define SIF_STAT_SIFINIT 0x10000
/** Bit of #SIF_REG_SMFLAG the IOP sets once its command layer has initialised. */
#define SIF_STAT_CMDINIT 0x20000
/** Bit of #SIF_REG_SMFLAG the IOP sets once it has booted. */
#define SIF_STAT_BOOTEND 0x40000

/** Width of the value sceSifGetIopValue() and sceSifSetIopValue() move. */
enum {
    SIF_IOP_VALUE_BYTE = 0, /*!< One byte. */
    SIF_IOP_VALUE_HALF = 1, /*!< Two bytes. */
    SIF_IOP_VALUE_WORD = 2, /*!< Four bytes. */
};

/** The entry point and global pointer of an executable the IOP loaded into main memory. */
typedef struct {
    unsigned int epc;   /*!< Entry point. */
    unsigned int gp;    /*!< Global pointer. */
    unsigned int sp;    /*!< Stack pointer, unused by the loader. */
    unsigned int dummy; /*!< Padding. */
} sceExecData;

/** Open for reading. */
#define SCE_RDONLY 0x0001
/** Open for writing. */
#define SCE_WRONLY 0x0002
/** Open for reading and writing. */
#define SCE_RDWR 0x0003
/** Create the file when it does not exist. */
#define SCE_CREAT 0x0200
/** Truncate the file on opening. */
#define SCE_TRUNC 0x0400

/** Seek origins of sceLseek(). */
#define SCE_SEEK_SET 0
#define SCE_SEEK_CUR 1
#define SCE_SEEK_END 2

/**
 * Stop SIF DMA.
 *
 * @ghidraAddress NTSC-U/C: 0x00536db0
 * @ghidraAddress PAL: 0x00576670
 */
void sceSifStopDma(void);

/**
 * Report the state of a SIF DMA transfer.
 *
 * @param id Transfer identifier from sceSifSetDma().
 * @return Negative once the transfer has finished.
 * @ghidraAddress NTSC-U/C: 0x00536e80
 * @ghidraAddress PAL: 0x00576740
 */
int sceSifDmaStat(unsigned int id);

/**
 * Report the state of a SIF DMA transfer from an interrupt handler.
 *
 * @param id Transfer identifier from isceSifSetDma().
 * @return Negative once the transfer has finished.
 * @ghidraAddress NTSC-U/C: 0x00536e90
 * @ghidraAddress PAL: 0x00576750
 */
int isceSifDmaStat(unsigned int id);

/**
 * Queue SIF DMA transfers to the IOP.
 *
 * @param sdd Transfers.
 * @param len Number of transfers.
 * @return A transfer identifier, or zero when the queue is full.
 * @ghidraAddress NTSC-U/C: 0x00536ea0
 * @ghidraAddress PAL: 0x00576760
 */
unsigned int sceSifSetDma(sceSifDmaData *sdd, int len);

/**
 * Queue SIF DMA transfers to the IOP from an interrupt handler.
 *
 * @param sdd Transfers.
 * @param len Number of transfers.
 * @return A transfer identifier, or zero when the queue is full.
 * @ghidraAddress NTSC-U/C: 0x00536eb0
 * @ghidraAddress PAL: 0x00576770
 */
unsigned int isceSifSetDma(sceSifDmaData *sdd, int len);

/**
 * Start the SIF0 channel in chain mode, ready for the next packet from the IOP.
 *
 * @ghidraAddress NTSC-U/C: 0x00536ec0
 * @ghidraAddress PAL: 0x00576780
 */
void sceSifSetDChain(void);

/**
 * Start the SIF0 channel in chain mode from an interrupt handler.
 *
 * @ghidraAddress NTSC-U/C: 0x00536ed0
 * @ghidraAddress PAL: 0x00576790
 */
void isceSifSetDChain(void);

/**
 * Write a SIF register.
 *
 * @param reg Register number such as #SIF_REG_SMFLAG or #SIF_SYSREG_SUBADDR.
 * @param val Value.
 * @return A kernel-defined value.
 * @ghidraAddress NTSC-U/C: 0x00536ee0
 * @ghidraAddress PAL: 0x005767a0
 */
int sceSifSetReg(unsigned int reg, int val);

/**
 * Read a SIF register.
 *
 * @param reg Register number such as #SIF_REG_SMFLAG or #SIF_SYSREG_SUBADDR.
 * @return The register value.
 * @ghidraAddress NTSC-U/C: 0x00536ef0
 * @ghidraAddress PAL: 0x005767b0
 */
int sceSifGetReg(unsigned int reg);

/**
 * Bind the IOP heap service, retrying until the server exists.
 *
 * @return Zero, or -1 when the bind request could not be sent.
 * @ghidraAddress NTSC-U/C: 0x005e5f88
 * @ghidraAddress PAL: 0x00628170
 */
int sceSifInitIopHeap(void);

/**
 * Allocate IOP memory.
 *
 * @param size Size in bytes.
 * @return The IOP address, or null when the service is not bound or the call fails.
 * @ghidraAddress NTSC-U/C: 0x005e6010
 * @ghidraAddress PAL: 0x006281f8
 */
void *sceSifAllocIopHeap(unsigned int size);

/**
 * Allocate IOP memory with the placement rules of the IOP memory allocator.
 *
 * @param type Placement mode of the allocator.
 * @param size Size in bytes.
 * @param addr Requested address, for modes that take one.
 * @return The IOP address, or null when the service is not bound or the call fails.
 * @ghidraAddress NTSC-U/C: 0x005e6080
 * @ghidraAddress PAL: 0x00628268
 */
void *sceSifAllocSysMemory(int type, unsigned int size, void *addr);

/**
 * Release IOP memory.
 *
 * @param addr IOP address from sceSifAllocIopHeap().
 * @return The server's result, zero when the service is not bound, or -1 when the call fails.
 * @ghidraAddress NTSC-U/C: 0x005e6100
 * @ghidraAddress PAL: 0x006282e8
 */
int sceSifFreeIopHeap(void *addr);

/**
 * Release IOP memory from sceSifAllocSysMemory(). The routine is sceSifFreeIopHeap() under
 * another name.
 *
 * @param addr IOP address.
 * @return As sceSifFreeIopHeap().
 * @ghidraAddress NTSC-U/C: 0x005e6178
 * @ghidraAddress PAL: 0x00628360
 */
int sceSifFreeSysMemory(void *addr);

/**
 * Load a file into IOP memory.
 *
 * @param filename Path, cut to 251 characters.
 * @param addr IOP address to load at.
 * @return The server's result, zero when the service is not bound, or -1 when the call fails.
 * @ghidraAddress NTSC-U/C: 0x005e6198
 * @ghidraAddress PAL: 0x00628380
 */
int sceSifLoadIopHeap(const char *filename, void *addr);

/**
 * Forget the module loader binding. The next loader call binds the server again.
 *
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x005fb3c8
 * @ghidraAddress PAL: 0x0063c0d8
 */
int sceSifLoadFileReset(void);

/**
 * Load and start an IOP module from a file.
 *
 * @param filename Module path, cut to 251 characters.
 * @param args Size of @p argp in bytes. At most 252 bytes are sent.
 * @param argp Arguments, each terminated by a zero byte, or null.
 * @return The module identifier, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fbc38
 * @ghidraAddress PAL: 0x0063c948
 */
int sceSifLoadModule(const char *filename, int args, const char *argp);

/**
 * Load and start an IOP module from a file, and report the result of its entry point.
 *
 * @param filename Module path, cut to 251 characters.
 * @param args Size of @p argp in bytes. At most 252 bytes are sent.
 * @param argp Arguments, each terminated by a zero byte, or null.
 * @param result Receives the result of the module entry point.
 * @return The module identifier, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fbc58
 * @ghidraAddress PAL: 0x0063c968
 */
int sceSifLoadStartModule(const char *filename, int args, const char *argp, int *result);

/**
 * Load and start an IOP module already in IOP memory.
 *
 * @param addr IOP address of the module image.
 * @param args Size of @p argp in bytes. At most 252 bytes are sent.
 * @param argp Arguments, each terminated by a zero byte, or null.
 * @return The module identifier, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fb9d0
 * @ghidraAddress PAL: 0x0063c6e0
 */
int sceSifLoadModuleBuffer(const void *addr, int args, const char *argp);

/**
 * Load and start an IOP module already in IOP memory, and report the result of its entry point.
 *
 * @param addr IOP address of the module image.
 * @param args Size of @p argp in bytes. At most 252 bytes are sent.
 * @param argp Arguments, each terminated by a zero byte, or null.
 * @param result Receives the result of the module entry point.
 * @return The module identifier, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fb9f0
 * @ghidraAddress PAL: 0x0063c700
 */
int sceSifLoadStartModuleBuffer(const void *addr, int args, const char *argp, int *result);

/**
 * Stop an IOP module.
 *
 * @param modid Module identifier.
 * @param args Size of @p argp in bytes. At most 252 bytes are sent.
 * @param argp Arguments, each terminated by a zero byte, or null.
 * @param result Receives the result of the module stop routine.
 * @return The module identifier, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fb608
 * @ghidraAddress PAL: 0x0063c318
 */
int sceSifStopModule(int modid, int args, const char *argp, int *result);

/**
 * Unload a stopped IOP module.
 *
 * @param modid Module identifier.
 * @return The module identifier, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fb810
 * @ghidraAddress PAL: 0x0063c520
 */
int sceSifUnloadModule(int modid);

/**
 * Find a loaded IOP module by name.
 *
 * @param modulename Module name, cut to 251 characters.
 * @return The module identifier, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fb8a0
 * @ghidraAddress PAL: 0x0063c5b0
 */
int sceSifSearchModuleByName(const char *modulename);

/**
 * Find the loaded IOP module that includes an IOP address.
 *
 * @param addr IOP address.
 * @return The module identifier, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fb940
 * @ghidraAddress PAL: 0x0063c650
 */
int sceSifSearchModuleByAddress(const void *addr);

/**
 * Load one section of an executable into main memory through the IOP.
 *
 * @param name Executable path, cut to 251 characters.
 * @param secname Section name, cut to 251 characters, or `all`.
 * @param data Receives the entry point and global pointer.
 * @return Zero, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fbd80
 * @ghidraAddress PAL: 0x0063ca90
 */
int sceSifLoadElfPart(const char *name, const char *secname, sceExecData *data);

/**
 * Load a whole executable into main memory through the IOP.
 *
 * @param name Executable path, cut to 251 characters.
 * @param data Receives the entry point and global pointer.
 * @return Zero, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fbda0
 * @ghidraAddress PAL: 0x0063cab0
 */
int sceSifLoadElf(const char *name, sceExecData *data);

/**
 * Read a value from IOP memory through the module loader. The name is inferred from the
 * behaviour.
 *
 * @param addr IOP address.
 * @param value Receives the value.
 * @param type Width, one of #SIF_IOP_VALUE_BYTE, #SIF_IOP_VALUE_HALF, and #SIF_IOP_VALUE_WORD.
 * @return Zero, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fbdc8
 * @ghidraAddress PAL: 0x0063cad8
 */
int sceSifGetIopValue(unsigned int addr, void *value, int type);

/**
 * Write a value to IOP memory through the module loader. The name is inferred from the behaviour.
 *
 * @param addr IOP address.
 * @param value Value to write.
 * @param type Width, one of #SIF_IOP_VALUE_BYTE, #SIF_IOP_VALUE_HALF, and #SIF_IOP_VALUE_WORD.
 * @return Zero, or a negative value on failure.
 * @ghidraAddress NTSC-U/C: 0x005fbeb8
 * @ghidraAddress PAL: 0x0063cbc8
 */
int sceSifSetIopValue(unsigned int addr, const void *value, int type);

/**
 * Send the IOP a reset command that reboots it with an argument string.
 *
 * @param arg Argument string of up to 80 characters, such as `rom0:UDNL <image>`.
 * @param mode Reset mode the IOP receives.
 * @return Nonzero once the command is queued, zero when the DMA queue is full.
 * @ghidraAddress NTSC-U/C: 0x005bc6e8
 * @ghidraAddress PAL: 0x005fedc8
 */
int sceSifResetIop(const char *arg, int mode);

/**
 * Report whether SIF on the IOP has initialised.
 *
 * @return Nonzero once it has.
 * @ghidraAddress NTSC-U/C: 0x005bc828
 * @ghidraAddress PAL: 0x005fef08
 */
int sceSifIsAliveIop(void);

/**
 * Report whether the IOP has finished rebooting. The console is closed once it has, and the next
 * console write opens it again.
 *
 * @return Nonzero once it has.
 * @ghidraAddress NTSC-U/C: 0x005bc850
 * @ghidraAddress PAL: 0x005fef30
 */
int sceSifSyncIop(void);

/**
 * Reboot the IOP with a replacement image.
 *
 * @param imgname Image path. With the `rom0:UDNL ` prefix and its terminator it must fit in 80
 *     bytes.
 * @return Nonzero once the reboot request is sent, zero when the path is too long or the DMA
 *     queue is full.
 * @ghidraAddress NTSC-U/C: 0x005bc888
 * @ghidraAddress PAL: 0x005fef68
 */
int sceSifRebootIop(const char *imgname);

/**
 * Unbind the file service client after an IOP reboot. The next call binds the new server.
 *
 * @return Zero.
 */
int sceFsReset(void);

/**
 * Open a file.
 *
 * @param filename Path, with its device prefix.
 * @param flag Open flags such as #SCE_RDONLY.
 * @return A descriptor, or a negative error code.
 */
int sceOpen(const char *filename, int flag, ...);

/**
 * Close a file.
 *
 * @param fd Descriptor.
 * @return Zero, or a negative error code.
 */
int sceClose(int fd);

/**
 * Read from a file.
 *
 * @param fd Descriptor.
 * @param buf Destination.
 * @param nbyte Byte count.
 * @return The bytes read, or a negative error code.
 */
int sceRead(int fd, void *buf, int nbyte);

/**
 * Write to a file.
 *
 * @param fd Descriptor.
 * @param buf Source.
 * @param nbyte Byte count.
 * @return The bytes written, or a negative error code.
 */
int sceWrite(int fd, const void *buf, int nbyte);

/**
 * Move the position of a file.
 *
 * @param fd Descriptor.
 * @param offset Offset from @p where.
 * @param where Origin such as #SCE_SEEK_SET.
 * @return The new position, or a negative error code.
 */
int sceLseek(int fd, int offset, int where);

/**
 * Send a control request for an open file to its device.
 *
 * @param fd Descriptor.
 * @param req Request code.
 * @param arg Request argument.
 * @return A request-defined value, or a negative error code.
 */
int sceIoctl(int fd, int req, void *arg);

#ifdef __cplusplus
}
#endif

#endif
