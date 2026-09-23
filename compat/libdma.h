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

// The channel register block. Only pointers to it are used.
typedef struct sceDmaChan sceDmaChan;

sceDmaChan *sceDmaGetChan(int nChannel);
void sceDmaSendN(sceDmaChan *pChannel, void *pAddress, int nQuadwords);

#ifdef __cplusplus
}
#endif

#endif
