#ifndef LIBMPEG_H
#define LIBMPEG_H

// Build support for the open-source SDK. Sony's libmpeg is not part of ps2sdk. The entry points
// the reconstruction calls are declared here with the signatures the shipped program's bodies and
// call sites prove. Nothing here is reconstructed source.

#ifdef __cplusplus
extern "C" {
#endif

// The decoder state, 0x48 bytes. The sample's VideoDec embeds one at its start and places its next
// member at +0x48. The game reads only the count of decoded frames.
typedef struct {
    int mUnknown00[2];
    int frameCount;
    unsigned char mUnknown0c[0x3c];
} sceMpeg;

// Stream types, of which the game registers two.
#define sceMpegStrM2V 0
#define sceMpegStrPCM 2

int sceMpegInit(void);

// Demultiplexes size bytes of a PSS stream starting at pStart inside the ring buffer pBuffer of
// nBufferSize bytes, and returns the bytes consumed.
int sceMpegDemuxPssRing(
    sceMpeg *pMpeg, unsigned char *pStart, int nSize, unsigned char *pBuffer, int nBufferSize);

#ifdef __cplusplus
}
#endif

#endif
