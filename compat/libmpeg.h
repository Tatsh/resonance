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

// Reads word zero of the decoder context. The decode worker spins on it.
int sceMpegGetContextWordZero(void *pDecoder);

// Decodes one picture with the given mode, negative when the picture is rejected.
int sceMpegSub005e07b0(void *pDecoder, void *pPicture, int nMode);

// Drains the decoder after the input ends.
void sceMpegSub005e08e8(void *pDecoder);

// Creates the decoder context over the work area.
void sceMpegCreateDecoderContext(void *pDecoder, void *pWork, int nWorkSize);

// Registers a callback in the given slot.
void sceMpegSetCallbackSlot(void *pDecoder, int nSlot, void *pfnCallback, void *pData);

// Reports one. Inferred.
int sceMpegReturnOne(void *pDecoder);

// Resets the decoder with three values.
void sceMpegSub005e0890(void *pDecoder, int nArgA, int nArgB, int nArgC);

// Reports whether word four of the decoder context is clear.
int sceMpegIsContextWordFourClear(void *pDecoder);

// Registers a stream callback for the given type and channel.
void sceMpegAddStrCallback(void *pDecoder, int nType, int nChannel, void *pfnCallback, void *pData);

#ifdef __cplusplus
}
#endif

#endif
