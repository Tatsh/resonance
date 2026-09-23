#ifndef LIBGRAPH_H
#define LIBGRAPH_H

// Build support for the open-source SDK. Sony's libgraph is not part of ps2sdk. The entry points
// the reconstruction calls are declared here with the signatures the shipped program's call sites
// prove. Nothing here is reconstructed source.

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

// One GIFtag, 16 bytes.
typedef struct {
    unsigned long mWords[2];
} sceGifTag;

// The five display registers in the order sceGsSetDefDispEnv() fills them. The game writes each
// word to the privileged register of the same name.
typedef struct {
    unsigned long pmode;
    unsigned long smode2;
    unsigned long dispfb;
    unsigned long display;
    unsigned long bgcolor;
} sceGsDispEnv;

// Eight A+D register and address pairs, 128 bytes. sceGsSetDefDrawEnv() returns 8, the pair
// count. The game reads FRAME_1 from the first pair, writes ZBUF_1 into the second, and reads
// XYOFFSET_1 from the third. GfxDevice::SwapBuffers() copies every register word into its
// register shadow, which fixes the order of the remaining five.
typedef struct {
    unsigned long frame1;
    unsigned long frame1addr;
    unsigned long zbuf1;
    unsigned long zbuf1addr;
    unsigned long xyoffset1;
    unsigned long xyoffset1addr;
    unsigned long scissor1;
    unsigned long scissor1addr;
    unsigned long prmodecont;
    unsigned long prmodecontaddr;
    unsigned long colclamp;
    unsigned long colclampaddr;
    unsigned long dthe;
    unsigned long dtheaddr;
    unsigned long test1;
    unsigned long test1addr;
} sceGsDrawEnv1;

// Six A+D register and address pairs, 96 bytes. sceGsSetDefClear() returns 6, the pair count.
typedef struct {
    unsigned long mWords[12];
} sceGsClear;

// The Sony double buffer, 0x230 bytes. sceGsSetDefDBuff() fills the clear colour of each half
// at +0x100 and +0x1f0, which is the RGBAQ pair of each sceGsClear under this layout.
typedef struct {
    sceGsDispEnv disp[2];
    sceGifTag giftag0;
    sceGsDrawEnv1 draw0;
    sceGsClear clear0;
    sceGifTag giftag1;
    sceGsDrawEnv1 draw1;
    sceGsClear clear1;
} sceGsDBuff;

// Four A+D register and address pairs, 64 bytes: ALPHA_1, PABE, TEXA, and FBA_1 among them.
// sceGsSetDefAlphaEnv() returns 4, the pair count.
typedef struct {
    unsigned long mWords[8];
} sceGsAlphaEnv;

// Resets VIF1, VU1, and the GIF, and primes VIF1 through its FIFO.
void sceGsResetPath(void);
void sceGsResetGraph(short nMode, short nInterlace, short nOutputMode, short nFieldMode);
// Waits for the next vertical blank and returns the field it began.
int sceGsSyncV(int nMode);
int sceGsSetDefAlphaEnv(sceGsAlphaEnv *pAlpha, short nPabe);
void sceGsSetDefDispEnv(
    sceGsDispEnv *pDisp, short nPsm, short nWidth, short nHeight, short nDx, short nDy);
int sceGsSetDefDrawEnv(
    sceGsDrawEnv1 *pDraw, short nPsm, short nWidth, short nHeight, short nZTest, short nZPsm);
int sceGsSetDefClear(sceGsClear *pClear,
                     short nZTest,
                     short nX,
                     short nY,
                     short nWidth,
                     short nHeight,
                     unsigned long nRed,
                     unsigned long nGreen,
                     unsigned long nBlue,
                     unsigned long nAlpha,
                     unsigned int nZ);
int sceGsSetDefDBuff(sceGsDBuff *pDBuff,
                     short nPsm,
                     short nWidth,
                     short nHeight,
                     short nZTest,
                     short nZPsm,
                     short nClear);
int sceGsPutDrawEnv(sceGifTag *pGifTag);
int sceGsSetDefLoadImage(sceGsLoadImage *pLoadImage,
                         short nTbp,
                         short nTbw,
                         short nPsm,
                         short nSsx,
                         short nSsy,
                         short nRrw,
                         short nRrh);
int sceGsExecLoadImage(sceGsLoadImage *pLoadImage, const void *pSource);
int sceGsSetDefStoreImage(sceGsStoreImage *pStoreImage,
                          short nSbp,
                          short nSbw,
                          short nPsm,
                          short nSsx,
                          short nSsy,
                          short nRrw,
                          short nRrh);
int sceGsExecStoreImage(sceGsStoreImage *pStoreImage, void *pDest);
int sceGsSyncPath(int nMode, unsigned short nTimeout);

#ifdef __cplusplus
}
#endif

#endif
