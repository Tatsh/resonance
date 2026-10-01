#ifndef LIBGIFPK_H
#define LIBGIFPK_H

// Build support for the open-source SDK. Sony's libgifpk is not part of ps2sdk. The entry points
// the reconstruction calls are declared here with the signatures the shipped program's call sites
// prove. Nothing here is reconstructed source.

#ifdef __cplusplus
extern "C" {
#endif

// The GIF packet work area, 16 bytes. Only the current position, the base, the pending tag, and
// the open tag address are observed. Inferred.
typedef struct {
    void *mCurrent; // +0x00, current write position. Inferred.
    void *mBase; // +0x04, packet base set at initialisation. Inferred.
    void *mDmaTag; // +0x08, tag under construction. Inferred.
    void *mPrevious; // +0x0c, start of the open tag. Inferred.
} sceGifPkData;

void sceGifPkInit(sceGifPkData *pPacket, void *pBuffer);
void sceGifPkReset(sceGifPkData *pPacket);
void sceGifPkCnt(sceGifPkData *pPacket, int nLoop, int nEop, int nPre);
void sceGifPkEnd(sceGifPkData *pPacket, int nLoop, int nEop, int nPre);
void sceGifPkOpenGsAD(sceGifPkData *pPacket, const void *pData);
void sceGifPkAddGsAD(sceGifPkData *pPacket, int nAddress, unsigned long long nData);
void sceGifPkCloseGifTag(sceGifPkData *pPacket);
void sceGifPkTerminate(sceGifPkData *pPacket);
unsigned long long *sceGifPkReserve(sceGifPkData *pPacket, int nWords);
void sceGifPkRef(
    sceGifPkData *pPacket, void *pData, int nQuadwords, int nOption1, int nOption2, int nFlag);

#ifdef __cplusplus
}
#endif

#endif
