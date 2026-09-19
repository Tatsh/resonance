#ifndef LIBGRAPH_H
#define LIBGRAPH_H

// Build support for the open-source SDK. Sony's libgraph is not part of ps2sdk, so the entry
// points the reconstruction calls are declared here with the signatures the shipped program's call
// sites prove. Nothing here is reconstructed source.

#ifdef __cplusplus
extern "C" {
#endif

// Both descriptors are 96 bytes in the image, an opening GIFtag, four register-and-tag pairs for
// BITBLTBUF, TRXPOS, TRXREG, and TRXDIR, and a closing GIFtag.
typedef struct {
    unsigned long mWords[12];
} sceGsLoadImage;

typedef struct {
    unsigned long mWords[12];
} sceGsStoreImage;

int sceGsSetDefLoadImage(sceGsLoadImage *pLoadImage, short nTbp, short nTbw, short nPsm, short nSsx,
                         short nSsy, short nRrw, short nRrh);
int sceGsExecLoadImage(sceGsLoadImage *pLoadImage, const void *pSource);
int sceGsSetDefStoreImage(sceGsStoreImage *pStoreImage, short nSbp, short nSbw, short nPsm,
                          short nSsx, short nSsy, short nRrw, short nRrh);
int sceGsExecStoreImage(sceGsStoreImage *pStoreImage, void *pDest);
int sceGsSyncPath(int nMode, unsigned short nTimeout);

#ifdef __cplusplus
}
#endif

#endif
