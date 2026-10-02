#ifndef EZMIDI_EZMIDI_H
#define EZMIDI_EZMIDI_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Types the three units of the EZMIDI IOP module share. The layouts and names follow the stabs
 * the shipped EZMIDI.IRX records. Addresses in the comments below are offsets into EZMIDI.IRX.
 */

/** SIF RPC server identifier of the module. */
#define EZMIDI_RPC_SERVER 0x12346

/** Bits of an RPC function number that select the port. */
#define EZMIDI_CMD_PORT_MASK 0xf

/** Bits of an RPC function number that select the command. */
#define EZMIDI_CMD_MASK 0xfff0

/** RPC commands. The argument buffer is an int unless noted. */
enum {
    EZMIDI_CMD_ALL_NOTES_OFF = 0xc0,     /*!< Silence every channel. */
    EZMIDI_CMD_INFO = 0xd0,              /*!< Diagnostic request, see HardSynthInfo(). */
    EZMIDI_CMD_PAUSE = 0xf0,             /*!< Nonzero pauses, zero resumes. */
    EZMIDI_CMD_REMIX = 0x100,            /*!< Switch remix mode. */
    EZMIDI_CMD_MONO = 0x110,             /*!< Switch mono output. */
    EZMIDI_CMD_INVALIDATE_HD = 0x120,    /*!< Detach the bank slot using a header address. */
    EZMIDI_CMD_ATTACH_HD = 0x1050,       /*!< Attach a bank; the buffer is an EZMIDI_BANK. */
    EZMIDI_CMD_LOAD_BD = 0x1070,         /*!< Load a bank body; the buffer is an EZMIDI_BANK. */
    EZMIDI_CMD_CONFIG = 0x10e0,          /*!< Replace the settings; the buffer is sSynthConfig. */
    EZMIDI_CMD_INIT = 0x8010,            /*!< Initialise; replies with the MIDI buffer address. */
    EZMIDI_CMD_INVALIDATE_BANK = 0x8130, /*!< Detach a bank slot. */
};

/**
 * Enable the SPU2 DMA interrupts and serve #EZMIDI_RPC_SERVER. It does not return.
 *
 * @return Zero.
 */
int sce_midi_loop(void);

/** One sound bank the EE describes to the module. */
typedef struct {
    unsigned int hdAddr;  /*!< IOP address of the bank header. */
    unsigned int bdAddr;  /*!< IOP address of the bank body. */
    unsigned int bdSize;  /*!< Byte size of the bank body. */
    unsigned int spuAddr; /*!< SPU2 address the body loads to. */
    int bank;             /*!< Bank slot. */
    char bdName[108];     /*!< Body file name. */
} EZMIDI_BANK;

/** Synthesiser settings the EE sends with HardSynthConfig(). */
typedef struct {
    unsigned short stt_speed;            /*!< Frames a volume slide takes. */
    unsigned short stt_limit;            /*!< Smallest volume change that slides. */
    unsigned char stt_type;              /*!< Slide curve. */
    unsigned char stt_pad[3];            /*!< Padding. */
    unsigned int chorus_rate[2];         /*!< Chorus rate per core. */
    unsigned int chorus_depth[2];        /*!< Chorus depth per core. */
    unsigned char chorus_shape[2];       /*!< Chorus curve per core. */
    unsigned char chorus_pad[2];         /*!< Padding. */
    unsigned int echo_pad;               /*!< Padding. */
    unsigned short rnd_nopause_channels; /*!< Channels that continue playing while paused. */
    unsigned short rnd_pad;              /*!< Padding. */
} sSynthConfig;

#ifdef __cplusplus
}
#endif

#endif
