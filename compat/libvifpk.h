#ifndef LIBVIFPK_H
#define LIBVIFPK_H

// Build support for the open-source SDK. Sony's libvifpk is not part of ps2sdk. The entry points
// the reconstruction calls are declared here with the signatures the shipped program's bodies and
// call sites prove. Nothing here is reconstructed source.

#include <libgraph.h>

#ifdef __cplusplus
extern "C" {
#endif

// A VIF1 packet under construction, 32 bytes on the target. sceVif1PkInit() sets the first two
// words to the buffer. The game reads the write pointer to build in place and the base to send the
// finished chain. The remaining words track the open DMA tag, VIF code, and GIFtag.
typedef struct {
    unsigned int *pCurrent;
    void *pBase;
    unsigned int mOtherWords[6];
} sceVif1Packet;

void sceVif1PkInit(sceVif1Packet *pPacket, void *pBase);
void sceVif1PkReset(sceVif1Packet *pPacket);
void sceVif1PkCnt(sceVif1Packet *pPacket, unsigned int nOption);
void sceVif1PkOpenDirectCode(sceVif1Packet *pPacket, int bStall);
void sceVif1PkOpenGifTag(sceVif1Packet *pPacket, sceGifTag gifTag);
unsigned int *sceVif1PkReserve(sceVif1Packet *pPacket, unsigned int nWords);
void sceVif1PkCloseGifTag(sceVif1Packet *pPacket);
void sceVif1PkCloseDirectCode(sceVif1Packet *pPacket);
void sceVif1PkEnd(sceVif1Packet *pPacket, unsigned int nOption);
unsigned int *sceVif1PkTerminate(sceVif1Packet *pPacket);

#ifdef __cplusplus
}
#endif

#endif
