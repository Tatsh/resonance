#ifndef SIFCMD_H
#define SIFCMD_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SIF commands, the packet layer the RPC service runs on. A packet travels to the IOP by SIF DMA,
 * and packets from the IOP arrive through the SIF0 channel, whose interrupt handler dispatches
 * each one to the handler registered for its command code.
 */

/** Command code bit that selects the system handler table rather than the user table. */
#define SIF_CMDC_SYSTEM 0x80000000

/** System command that reports a new command buffer address. */
#define SIF_CMDC_CHANGE_SADDR 0x80000000
/** System command that sets a software register. */
#define SIF_CMDC_SET_SREG 0x80000001
/** System command that initialises the command layer, or the RPC layer when its option is set. */
#define SIF_CMDC_INIT_CMD 0x80000002
/** System command that resets the IOP. */
#define SIF_CMDC_RESET_CMD 0x80000003

/** Send mode bit that selects the interrupt-handler form of the DMA calls. */
#define SIF_CMDM_INTR 0x01
/** Send mode bit that writes the extra data back from the data cache before sending. */
#define SIF_CMDM_WBDC 0x04

/** Size of the largest command packet in bytes. */
#define SIF_CMD_PACKET_MAX 112

/** The header every command packet begins with. */
typedef struct {
    unsigned int psize : 8;  /*!< Packet size in bytes. */
    unsigned int dsize : 24; /*!< Size of the extra data sent with the packet. */
    unsigned int daddr;      /*!< IOP destination of the extra data. */
    unsigned int fcode;      /*!< Command code. */
    unsigned int opt;        /*!< Command-defined option word. */
} sceSifCmdHdr;

/** A command handler. It receives the packet and the argument it was registered with. */
typedef void (*sceSifCmdHandler)(void *packet, void *data);

/** One slot of a handler table. */
typedef struct {
    sceSifCmdHandler func; /*!< Handler, or null for an empty slot. */
    void *data;            /*!< Argument the handler receives. */
} sceSifCmdData;

/** The packet of #SIF_CMDC_CHANGE_SADDR and #SIF_CMDC_INIT_CMD. */
typedef struct {
    sceSifCmdHdr chdr;    /*!< Header. */
    unsigned int newaddr; /*!< Address of the sender's command buffer. */
} sceSifCmdCSData;

/** The packet of #SIF_CMDC_SET_SREG. */
typedef struct {
    sceSifCmdHdr chdr;  /*!< Header. */
    int rno;            /*!< Software register number. */
    unsigned int value; /*!< Value. */
} sceSifCmdSRData;

/** Size of the argument string of #SIF_CMDC_RESET_CMD. */
#define SIF_CMD_RESET_ARG_MAX 80

/** The packet of #SIF_CMDC_RESET_CMD. */
typedef struct {
    sceSifCmdHdr chdr;               /*!< Header. */
    int size;                        /*!< Length of @ref arg, without a terminator. */
    int flag;                        /*!< Reset mode. */
    char arg[SIF_CMD_RESET_ARG_MAX]; /*!< Argument string, not terminated. */
} sceSifCmdResetData;

/**
 * Read a software register the IOP sets with #SIF_CMDC_SET_SREG.
 *
 * @param reg Register number, below 32.
 * @return The register value.
 * @ghidraAddress NTSC-U/C: 0x005d3588
 * @ghidraAddress PAL: 0x006155f0
 */
int sceSifGetSreg(int reg);

/**
 * Set a software register.
 *
 * @param reg Register number, below 32.
 * @param value Value.
 * @return @p value.
 * @ghidraAddress NTSC-U/C: 0x005d35a0
 * @ghidraAddress PAL: 0x00615608
 */
int sceSifSetSreg(int reg, int value);

/**
 * Initialise the command layer and tell the IOP where to send packets. A second call does
 * nothing.
 *
 * @ghidraAddress NTSC-U/C: 0x005d35d0
 * @ghidraAddress PAL: 0x00615638
 */
void sceSifInitCmd(void);

/**
 * Stop the command layer and remove its SIF0 interrupt handler.
 *
 * @ghidraAddress NTSC-U/C: 0x005d3850
 * @ghidraAddress PAL: 0x006158b8
 */
void sceSifExitCmd(void);

/**
 * Install the user handler table.
 *
 * @param db Table.
 * @param size Number of slots.
 * @return The previous table.
 * @ghidraAddress NTSC-U/C: 0x005d3888
 * @ghidraAddress PAL: 0x006158f0
 */
sceSifCmdData *sceSifSetCmdBuffer(sceSifCmdData *db, int size);

/**
 * Install the system handler table.
 *
 * @param db Table.
 * @param size Number of slots.
 * @return The previous table.
 * @ghidraAddress NTSC-U/C: 0x005d38a0
 * @ghidraAddress PAL: 0x00615908
 */
sceSifCmdData *sceSifSetSysCmdBuffer(sceSifCmdData *db, int size);

/**
 * Register a handler for a command code.
 *
 * @param fcode Command code. With #SIF_CMDC_SYSTEM set it indexes the system table.
 * @param handler Handler.
 * @param data Argument the handler receives.
 * @ghidraAddress NTSC-U/C: 0x005d38b8
 * @ghidraAddress PAL: 0x00615920
 */
void sceSifAddCmdHandler(unsigned int fcode, sceSifCmdHandler handler, void *data);

/**
 * Remove the handler of a command code.
 *
 * @param fcode Command code.
 * @ghidraAddress NTSC-U/C: 0x005d38e8
 * @ghidraAddress PAL: 0x00615950
 */
void sceSifRemoveCmdHandler(unsigned int fcode);

/**
 * Send a command packet to the IOP, with optional extra data sent ahead of it.
 *
 * @param fcode Command code.
 * @param packet Packet, beginning with a #sceSifCmdHdr, 16-byte aligned.
 * @param packet_size Packet size in bytes, 16 to #SIF_CMD_PACKET_MAX.
 * @param src_extra Extra data in main memory.
 * @param dest_extra IOP destination of the extra data.
 * @param size_extra Size of the extra data, or zero for none.
 * @return A transfer identifier, or zero when the packet size is out of range or the DMA queue is
 *     full.
 * @ghidraAddress NTSC-U/C: 0x005d3a48
 * @ghidraAddress PAL: 0x00615ab0
 */
unsigned int sceSifSendCmd(unsigned int fcode,
                           void *packet,
                           int packet_size,
                           void *src_extra,
                           void *dest_extra,
                           int size_extra);

/**
 * sceSifSendCmd() for an interrupt handler.
 *
 * @param fcode Command code.
 * @param packet Packet, beginning with a #sceSifCmdHdr, 16-byte aligned.
 * @param packet_size Packet size in bytes, 16 to #SIF_CMD_PACKET_MAX.
 * @param src_extra Extra data in main memory.
 * @param dest_extra IOP destination of the extra data.
 * @param size_extra Size of the extra data, or zero for none.
 * @return As sceSifSendCmd().
 * @ghidraAddress NTSC-U/C: 0x005d3a88
 * @ghidraAddress PAL: 0x00615af0
 */
unsigned int isceSifSendCmd(unsigned int fcode,
                            void *packet,
                            int packet_size,
                            void *src_extra,
                            void *dest_extra,
                            int size_extra);

/**
 * Write a range back from the data cache and invalidate it, before the IOP reads it by DMA.
 *
 * @param addr Start of the range.
 * @param size Size in bytes. Nothing happens when it is not positive.
 * @ghidraAddress NTSC-U/C: 0x005d3bf0
 * @ghidraAddress PAL: 0x00615c58
 */
void sceSifWriteBackDCache(void *addr, int size);

#ifdef __cplusplus
}
#endif

#endif
