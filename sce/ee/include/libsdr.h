#ifndef LIBSDR_H
#define LIBSDR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Remote access to the IOP sound library through the sdrdrv server. sceSdRemote() sends one libsd
 * call, identified by its rSd command, and returns the call's result. The register entry values
 * are the values the game sends.
 */

/** Commands of sceSdRemote(), one per libsd call. */
enum {
    rSdInit = 0x8000,           /*!< sceSdInit(flag). */
    rSdSetParam = 0x8010,       /*!< sceSdSetParam(entry, value). */
    rSdGetParam = 0x8020,       /*!< sceSdGetParam(entry). */
    rSdSetSwitch = 0x8030,      /*!< sceSdSetSwitch(entry, value). */
    rSdGetSwitch = 0x8040,      /*!< sceSdGetSwitch(entry). */
    rSdSetAddr = 0x8050,        /*!< sceSdSetAddr(entry, value). */
    rSdGetAddr = 0x8060,        /*!< sceSdGetAddr(entry). */
    rSdSetCoreAttr = 0x8070,    /*!< sceSdSetCoreAttr(entry, value). */
    rSdGetCoreAttr = 0x8080,    /*!< sceSdGetCoreAttr(entry). */
    rSdSetEffectAttr = 0x8130,  /*!< sceSdSetEffectAttr(core, attr). */
};

/** Core selectors of an entry value. */
#define SD_CORE_0 0
#define SD_CORE_1 1

/** The entry value of voice @p v of core @p core. */
#define SD_VOICE(core, v) ((core) | ((v) << 1))

/** Voice parameters. */
#define SD_VP_VOLL (0x00 << 8)
#define SD_VP_VOLR (0x01 << 8)
#define SD_VP_PITCH (0x02 << 8)
#define SD_VP_ADSR1 (0x03 << 8)
#define SD_VP_ADSR2 (0x04 << 8)
#define SD_VP_ENVX (0x05 << 8)
#define SD_VP_VOLXL (0x06 << 8)
#define SD_VP_VOLXR (0x07 << 8)

/** Core parameters. */
#define SD_P_MMIX (0x08 << 8)
#define SD_P_MVOLL ((0x09 << 8) | 0x80)
#define SD_P_MVOLR ((0x0a << 8) | 0x80)
#define SD_P_EVOLL ((0x0b << 8) | 0x80)
#define SD_P_EVOLR ((0x0c << 8) | 0x80)

/** Core switches, one bit per voice. */
#define SD_S_KON (0x15 << 8)
#define SD_S_KOFF (0x16 << 8)
#define SD_S_ENDX (0x17 << 8)
#define SD_S_VMIXL (0x18 << 8)
#define SD_S_VMIXEL (0x19 << 8)
#define SD_S_VMIXR (0x1a << 8)
#define SD_S_VMIXER (0x1b << 8)

/** Core address of the end of the effect work area. */
#define SD_A_EEA (0x1d << 8)

/** Voice addresses. */
#define SD_VA_SSA ((0x20 << 8) | 0x40)
#define SD_VA_NAX ((0x22 << 8) | 0x40)

/** Core attribute that enables the effect processor. */
#define SD_C_EFFECT_ENABLE 0x2

/** Effect mode bit that clears the effect work area. */
#define SD_EFFECT_MODE_CLEAR 0x100

/** Transfer directions and modes of sceSdVoiceTrans(). */
#define SD_TRANS_MODE_WRITE 0
#define SD_TRANS_BY_DMA 0

/** Poll flag of sceSdVoiceTransStatus(). */
#define SD_TRANS_STATUS_CHECK 0

/** Reverb settings of a core, sent with #rSdSetEffectAttr. */
typedef struct {
    int core;      /*!< Core number. */
    int mode;      /*!< Reverb mode, with #SD_EFFECT_MODE_CLEAR to clear the work area. */
    short depth_L; /*!< Left depth. */
    short depth_R; /*!< Right depth. */
    int delay;     /*!< Delay of the echo modes. */
    int feedback;  /*!< Feedback of the echo modes. */
} sceSdEffectAttr;

/**
 * Bind the sdrdrv server.
 *
 * @return Zero.
 */
int sceSdRemoteInit(void);

/**
 * Send one libsd call to the IOP.
 *
 * @param arg Nonzero to wait for the result. An rSd command and the call's arguments follow.
 * @return The call's result when @p arg is nonzero.
 */
int sceSdRemote(int arg, ...);

#ifdef __cplusplus
}
#endif

#endif
