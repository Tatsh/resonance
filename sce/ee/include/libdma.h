#ifndef LIBDMA_H
#define LIBDMA_H

#ifdef __cplusplus
extern "C" {
#endif

/** DMA controller library: channel lookup, transfers, and controller settings. */

/** Channel numbers sceDmaGetChan() accepts. The game uses these three. */
#define SCE_DMA_VIF0 0 /*!< VIF0 channel. */
#define SCE_DMA_VIF1 1 /*!< VIF1 channel. */
#define SCE_DMA_GIF 2  /*!< GIF channel. */

/** Channel register block. The game touches only CHCR at its start. */
typedef struct sceDmaChan {
    volatile unsigned int chcr; /*!< Channel control register. */
} sceDmaChan;

/**
 * Controller settings sceDmaGetEnv() copies out and sceDmaPutEnv() validates and writes back, 0x14
 * bytes.
 *
 * The game changes only the halfword at +6.
 */
typedef struct {
    unsigned char mUnknown00[6];  /*!< Undetermined. */
    unsigned short mChannelMask;  /*!< Mask with one bit per channel. */
    unsigned char mUnknown08[12]; /*!< Undetermined. */
} sceDmaEnv;

/**
 * Look up a channel's register block.
 *
 * @param nChannel The channel number.
 * @return The register block, or null for a number out of range.
 * @ghidraAddress NTSC-U/C: 0x005f36f0
 * @ghidraAddress PAL: 0x00596e10
 */
sceDmaChan *sceDmaGetChan(int nChannel);

/**
 * Wait for the channel to stop, then start a normal-mode transfer of nQuadwords quadwords from
 * pAddress.
 *
 * A wait past the spin limit prints a timeout message and clears the channel's start bit.
 *
 * @param pChannel The channel.
 * @param pAddress The source.
 * @param nQuadwords Size of the transfer in quadwords.
 * @ghidraAddress NTSC-U/C: 0x005f3b18
 * @ghidraAddress PAL: 0x00597238
 */
void sceDmaSendN(sceDmaChan *pChannel, void *pAddress, int nQuadwords);

/**
 * Report or wait for the end of a channel's transfer.
 *
 * With nMode 1, reports whether the channel is still running. Otherwise waits for the channel to
 * stop, a zero nTimeout selecting the default limit.
 *
 * @param pChannel The channel.
 * @param nMode 1 to poll, any other value to wait.
 * @param nTimeout Wait limit, or 0 for the default.
 * @return 1 when polling a running channel, otherwise 0.
 * @ghidraAddress NTSC-U/C: 0x005f3f90
 * @ghidraAddress PAL: 0x005976b0
 */
int sceDmaSync(sceDmaChan *pChannel, int nMode, int nTimeout);

/**
 * Reset every channel.
 *
 * Clears the address and control words of every enabled channel and the status bits, then
 * applies cleared controller settings.
 *
 * @param nMode 1 to enable the controller afterwards.
 * @return The previous enable state.
 * @ghidraAddress NTSC-U/C: 0x005f3718
 * @ghidraAddress PAL: 0x00596e38
 */
int sceDmaReset(int nMode);

/**
 * Copy the saved controller settings into pEnv.
 *
 * @param pEnv Receives the settings.
 * @return pEnv.
 * @ghidraAddress NTSC-U/C: 0x005f39e0
 * @ghidraAddress PAL: 0x00597100
 */
sceDmaEnv *sceDmaGetEnv(sceDmaEnv *pEnv);

/**
 * Validate the controller settings in pEnv and write them back.
 *
 * @param pEnv The settings.
 * @return 0, or -1 through -4 for the first field out of range.
 * @ghidraAddress NTSC-U/C: 0x005f3808
 * @ghidraAddress PAL: 0x00596f28
 */
int sceDmaPutEnv(sceDmaEnv *pEnv);

/**
 * Start a source-chain transfer from the DMA tag at pTag.
 *
 * Waits for the channel to stop first. A wait past the spin limit prints a timeout message and
 * clears the channel's start bit.
 *
 * @param pChannel The channel.
 * @param pTag The first DMA tag of the chain.
 * @ghidraAddress NTSC-U/C: 0x005f3a40
 * @ghidraAddress PAL: 0x00597160
 */
void sceDmaSend(sceDmaChan *pChannel, void *pTag);

#ifdef __cplusplus
}
#endif

#endif
