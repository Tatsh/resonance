#ifndef LIBSD_H
#define LIBSD_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SPU2 register access of the resident libsd library. Every entry value combines a register
 * identifier from the lists below with a voice number and a core, as SD_VOICE() and #SD_CORE_1
 * build them. The types follow the stabs the shipped EZMIDI.IRX records.
 */

/** Core selector bit of an entry value. */
#define SD_CORE_0 0
#define SD_CORE_1 1

/** Place a voice number in the voice field of an entry value. */
#define SD_VOICE(voice) ((voice) << 1)

/** Voice parameters. Each is combined with a core and a voice. */
#define SD_VP_VOLL (0x00 << 8)
#define SD_VP_VOLR (0x01 << 8)
#define SD_VP_PITCH (0x02 << 8)
#define SD_VP_ADSR1 (0x03 << 8)
#define SD_VP_ADSR2 (0x04 << 8)
#define SD_VP_ENVX (0x05 << 8)
#define SD_VP_VOLXL (0x06 << 8)
#define SD_VP_VOLXR (0x07 << 8)

/** Core parameters. Each is combined with a core. */
#define SD_P_MVOLL ((0x09 << 8) + (0x01 << 7))
#define SD_P_MVOLR ((0x0a << 8) + (0x01 << 7))
#define SD_P_EVOLL ((0x0b << 8) + (0x01 << 7))
#define SD_P_EVOLR ((0x0c << 8) + (0x01 << 7))

/** Core switches, one bit per voice. Each is combined with a core. */
#define SD_S_PMON (0x13 << 8)
#define SD_S_NON (0x14 << 8)
#define SD_S_KON (0x15 << 8)
#define SD_S_KOFF (0x16 << 8)
#define SD_S_ENDX (0x17 << 8)
#define SD_S_VMIXL (0x18 << 8)
#define SD_S_VMIXEL (0x19 << 8)
#define SD_S_VMIXR (0x1a << 8)
#define SD_S_VMIXER (0x1b << 8)

/** Voice addresses. Each is combined with a core and a voice. */
#define SD_VA_SSA ((0x20 << 8) + (0x01 << 6))
#define SD_VA_LSAX ((0x21 << 8) + (0x01 << 6))
#define SD_VA_NAX ((0x22 << 8) + (0x01 << 6))

/** Transfer directions and modes of sceSdVoiceTrans(). */
#define SD_TRANS_MODE_WRITE 0
#define SD_TRANS_MODE_READ 1
#define SD_TRANS_BY_DMA (0x0 << 3)
#define SD_TRANS_BY_IO (0x1 << 3)

/** Wait flags of sceSdVoiceTransStatus(). */
#define SD_TRANS_STATUS_WAIT 1
#define SD_TRANS_STATUS_CHECK 0

/** An interrupt handler for transfer completion. */
typedef int (*sceSdTransIntrHandler)(int channel, void *data);

/** An interrupt handler for SPU2 interrupts. */
typedef sceSdTransIntrHandler sceSdSpu2IntrHandler;

/** One command of a batch. */
typedef struct {
    unsigned short func;  /*!< Command code. */
    unsigned short entry; /*!< Entry value the command acts on. */
    unsigned int value;   /*!< Value to write, or the slot that receives a read. */
} sceSdBatch;

/** Reverb settings of a core. */
typedef struct {
    int core;      /*!< Core number. */
    int mode;      /*!< Reverb mode. */
    short depth_L; /*!< Left depth. */
    short depth_R; /*!< Right depth. */
    int delay;     /*!< Delay of the echo modes. */
    int feedback;  /*!< Feedback of the echo modes. */
} sceSdEffectAttr;

/**
 * Write a parameter register.
 *
 * @param entry Entry value.
 * @param value Value to write.
 */
void sceSdSetParam(unsigned short entry, unsigned short value);

/**
 * Read a parameter register.
 *
 * @param entry Entry value.
 * @return The register value.
 */
unsigned short sceSdGetParam(unsigned short entry);

/**
 * Write a switch register pair.
 *
 * @param entry Entry value.
 * @param value One bit per voice.
 */
void sceSdSetSwitch(unsigned short entry, unsigned int value);

/**
 * Read a switch register pair.
 *
 * @param entry Entry value.
 * @return One bit per voice.
 */
unsigned int sceSdGetSwitch(unsigned short entry);

/**
 * Write an address register pair.
 *
 * @param entry Entry value.
 * @param value SPU2 byte address.
 */
void sceSdSetAddr(unsigned short entry, unsigned int value);

/**
 * Convert a note to a pitch register value.
 *
 * @param center_note Note that plays at the sample rate.
 * @param center_fine Fine tuning of @p center_note.
 * @param note Note to play.
 * @param fine Fine tuning of @p note.
 * @return The pitch register value.
 */
unsigned short sceSdNote2Pitch(unsigned short center_note,
                               unsigned short center_fine,
                               unsigned short note,
                               short fine);

/**
 * Transfer waveform data between IOP and SPU2 memory.
 *
 * @param channel Transfer channel, also the core number.
 * @param mode Direction and method bits.
 * @param m_addr IOP address.
 * @param s_addr SPU2 address.
 * @param size Byte count.
 * @return The byte count, or a negative error code.
 */
int sceSdVoiceTrans(short channel,
                    unsigned short mode,
                    unsigned char *m_addr,
                    unsigned int s_addr,
                    unsigned int size);

/**
 * Report or wait for the end of a voice transfer.
 *
 * @param channel Transfer channel.
 * @param flag #SD_TRANS_STATUS_WAIT to wait, or #SD_TRANS_STATUS_CHECK to poll.
 * @return Nonzero once the transfer has ended.
 */
unsigned int sceSdVoiceTransStatus(short channel, short flag);

#ifdef __cplusplus
}
#endif

#endif
