#ifndef LIBDMA_H
#define LIBDMA_H

// Build support for the open-source SDK. Sony's libdma is not part of ps2sdk. The entry points the
// reconstruction calls are declared here with the signatures the shipped program's call sites
// prove. Nothing here is reconstructed source.

#ifdef __cplusplus
extern "C" {
#endif

// Channel numbers sceDmaGetChan() accepts, of which the reconstruction uses two.
#define SCE_DMA_VIF1 1
#define SCE_DMA_GIF 2

// The channel register block, of which the game touches only CHCR at its start.
typedef struct sceDmaChan {
    volatile unsigned int chcr;
} sceDmaChan;

// The controller settings sceDmaGetEnv() copies out and sceDmaPutEnv() validates and writes back,
// 0x14 bytes. The game changes only the halfword at +6, a mask with one bit per channel.
typedef struct {
    unsigned char mUnknown00[6];
    unsigned short mChannelMask;
    unsigned char mUnknown08[12];
} sceDmaEnv;

sceDmaChan *sceDmaGetChan(int nChannel);
void sceDmaSendN(sceDmaChan *pChannel, void *pAddress, int nQuadwords);

// With nMode 1 returns whether the channel is still running. Otherwise waits for it to stop, a
// zero nTimeout selecting the default limit.
int sceDmaSync(sceDmaChan *pChannel, int nMode, int nTimeout);

// Resets every channel and returns the previous enable state.
int sceDmaReset(int nMode);

// Copies the saved settings into pEnv and returns pEnv.
sceDmaEnv *sceDmaGetEnv(sceDmaEnv *pEnv);

// Returns 0, or -1 when a field is out of range.
int sceDmaPutEnv(sceDmaEnv *pEnv);

// Starts a source-chain transfer from the DMA tag at pTag.
void sceDmaSend(sceDmaChan *pChannel, void *pTag);

#ifdef __cplusplus
}
#endif

#endif
