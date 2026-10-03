#ifndef EZMPEG_H
#define EZMPEG_H

#include <libmpeg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sony's ezmpeg sample: the video decoder, the audio decoder, the stream reader, and the display
 * the cutscene player drives, under the sample's names.
 */

/**
 * Video decoder, 0xb8 bytes.
 *
 * The decoder state follows the embedded sceMpeg and the input ring. The two handler identifiers
 * the cutscene player registers close the structure.
 */
typedef struct {
    sceMpeg mpeg;              /*!< Decoder state. */
    unsigned char vibuf[0x60]; /*!< Input ring, treated as a ViBuf by the decoder unit. */
    int state;                 /*!< Decoder state, such as VD_STATE_ABORT or VD_STATE_END. */
    int reserved;              /*!< Never read or written by the decoder unit. +0xac */
    int hid_endimage;          /*!< Identifier of the end-of-image interrupt handler. */
    int hid_vblank;            /*!< Identifier of the vertical blank interrupt handler. */
} VideoDec;

/**
 * Audio decoder, 0x5c bytes.
 *
 * The structure includes the transfer stage, the staging buffer on the Emotion Engine side, and
 * the two regions on the IOP side, with the counts the transfers advance.
 */
typedef struct {
    int state;                  /*!< Transfer stage (idle, priming, streaming, or stopping). */
    unsigned char header[0x28]; /*!< Stream header, filled byte by byte before the samples. */
    int headerCount;            /*!< Header bytes received so far. */
    unsigned char *buffer;      /*!< Staging buffer on the Emotion Engine side. */
    int put;                    /*!< Staging write offset. */
    int count;                  /*!< Staged bytes not yet transferred. */
    int bufferSize;             /*!< Staging buffer size in bytes. */
    int totalBytes;             /*!< Bytes staged since the last reset. */
    int iopBuffer;              /*!< Buffer address on the IOP side. */
    int iopBufferSize;          /*!< Buffer size on the IOP side. */
    int iopOffset;              /*!< Write offset into the buffer on the IOP side. */
    int iopPauseOffset;         /*!< Play offset the driver reported at the stop call. */
    int totalBytesSent;         /*!< Bytes sent to the IOP side since the last reset. */
    int iopExtra;               /*!< Second region on the IOP side, with the preset block. */
} AudioDec;

/** Ring buffer between the file reader and the demultiplexer, 0x5000c bytes. */
typedef struct {
    unsigned char data[0x50000]; /*!< The ring. */
    int put;                     /*!< Write offset. */
    int count;                   /*!< Bytes in the ring. */
    int size;                    /*!< Capacity in bytes. */
} ReadBuf;

/**
 * An open stream, 0x38 bytes.
 *
 * A stream on the disc is read through the CD streaming functions and any other stream through
 * fd.
 */
typedef struct {
    int isOnCD;                 /*!< Nonzero for a stream on the disc. */
    int size;                   /*!< Size of the stream in bytes. */
    unsigned char cdFile[0x28]; /*!< The sceCdlFILE entry, then the IOP heap pointer at +0x2c. */
    int fd;                     /*!< File descriptor of a stream not on the disc. */
    int reserved;               /*!< Never read or written by the stream unit. +0x34 */
} StrFile;

/**
 * Decoded-frame queue, 0x14 bytes.
 *
 * The display's interrupt handler lowers the count while the decode worker waits on it.
 */
typedef struct {
    void *data;         /*!< Frame data of each slot. */
    void *tag;          /*!< DMA tags of each slot. */
    int write;          /*!< Slot the decoder fills next. */
    volatile int count; /*!< Decoded frames waiting for display. */
    int size;           /*!< Count of slots. */
} VoBuf;

/** Values videoDecGetState() returns and videoDecAbort() stores. */
#define VD_STATE_ABORT 1 /*!< Playback was aborted. */
#define VD_STATE_END 3   /*!< Decoding finished and the queue drained. */

/** Stream callback of the video decoder. */
typedef int (*VideoDecCallback)(sceMpeg *pMpeg, void *pCallbackData, void *pData);

/** Interrupt handler of the display. */
typedef int (*InterruptHandler)(int nCause);

/**
 * Create the decoder context, install the decoder callbacks, and create the input ring.
 *
 * @param pVideoDec The video decoder.
 * @param pWork Work area of the decoder context.
 * @param nWorkSize Size of pWork in bytes.
 * @param pData Data area of the input ring.
 * @param pTag DMA tag area of the input ring.
 * @param nTagSize Count of DMA tags.
 * @param pTimeStamps Time stamp area of the input ring.
 * @param nTimeStamps Count of time stamps.
 * @ghidraAddress NTSC-U/C: 0x005692f8
 * @ghidraAddress PAL: 0x005a97c0
 */
void videoDecCreate(VideoDec *pVideoDec,
                    unsigned char *pWork,
                    int nWorkSize,
                    void *pData,
                    void *pTag,
                    int nTagSize,
                    void *pTimeStamps,
                    int nTimeStamps);

/**
 * Register a stream callback with the decoder.
 *
 * @param pVideoDec The video decoder.
 * @param nType The stream type.
 * @param nChannel The channel.
 * @param pfnCallback The callback.
 * @param pData Data passed to the callback.
 * @return Always 1.
 * @ghidraAddress NTSC-U/C: 0x00569600
 * @ghidraAddress PAL: 0x005a9ac8
 */
int videoDecSetStream(
    VideoDec *pVideoDec, int nType, int nChannel, VideoDecCallback pfnCallback, void *pData);

/**
 * Report the decoder state.
 *
 * @param pVideoDec The video decoder.
 * @return The state.
 * @ghidraAddress NTSC-U/C: 0x00569440
 * @ghidraAddress PAL: 0x005a9908
 */
int videoDecGetState(VideoDec *pVideoDec);

/**
 * Store VD_STATE_ABORT in the decoder state.
 *
 * @param pVideoDec The video decoder.
 * @ghidraAddress NTSC-U/C: 0x00569430
 * @ghidraAddress PAL: 0x005a98f8
 */
void videoDecAbort(VideoDec *pVideoDec);

/**
 * Append a sequence end code to the input ring and flush the ring.
 *
 * @param pVideoDec The video decoder.
 * @return 1, or 0 when the ring does not have room for the end code.
 * @ghidraAddress NTSC-U/C: 0x005694d0
 * @ghidraAddress PAL: 0x005a9998
 */
int videoDecFlush(VideoDec *pVideoDec);

/**
 * Report whether the input ring is empty and no reference picture remains.
 *
 * @param pVideoDec The video decoder.
 * @return 1 when flushed, otherwise 0.
 * @ghidraAddress NTSC-U/C: 0x005695b0
 * @ghidraAddress PAL: 0x005a9a78
 */
int videoDecIsFlushed(VideoDec *pVideoDec);

/**
 * Delete the input ring and the decoder.
 *
 * @param pVideoDec The video decoder.
 * @return Always 1.
 * @ghidraAddress NTSC-U/C: 0x005693f8
 * @ghidraAddress PAL: 0x005a98c0
 */
int videoDecDelete(VideoDec *pVideoDec);

/**
 * Decode the stream, wait for the display to drain the frame queue, and store VD_STATE_END.
 *
 * @param pVideoDec The video decoder.
 * @ghidraAddress NTSC-U/C: 0x005696a0
 * @ghidraAddress PAL: 0x005a9b68
 */
void videoDecMain(VideoDec *pVideoDec);

/**
 * Copy a video packet from the read buffer into the decoder's input ring with its time stamps.
 *
 * @param pMpeg The decoder.
 * @param pCallbackData The sceMpegCbDataStr packet.
 * @param pData The ReadBuf the packet addresses.
 * @return 1 when bytes were copied, otherwise 0.
 * @ghidraAddress NTSC-U/C: 0x0059afa0
 * @ghidraAddress PAL: 0x005de438
 */
int videoCallback(sceMpeg *pMpeg, void *pCallbackData, void *pData);

/**
 * Copy a PCM packet, less its four-byte header, from the read buffer into the audio decoder.
 *
 * @param pMpeg The decoder.
 * @param pCallbackData The sceMpegCbDataStr packet.
 * @param pData The ReadBuf the packet addresses.
 * @return 1 when bytes were copied, otherwise 0.
 * @ghidraAddress NTSC-U/C: 0x0059b0c8
 * @ghidraAddress PAL: 0x005de560
 */
int pcmCallback(sceMpeg *pMpeg, void *pCallbackData, void *pData);

/**
 * Clear an audio decoder, allocate both regions on the IOP side, and upload the preset block.
 *
 * @param pAudioDec The audio decoder.
 * @param pBuffer Staging buffer.
 * @param nBufferSize Size of pBuffer in bytes.
 * @param nIopBufferSize Size of the buffer on the IOP side in bytes.
 * @return 1, or 0 when an allocation fails.
 * @ghidraAddress NTSC-U/C: 0x00567820
 * @ghidraAddress PAL: 0x005a7ce8
 */
int audioDecCreate(AudioDec *pAudioDec,
                   unsigned char *pBuffer,
                   int nBufferSize,
                   int nIopBufferSize);

/**
 * Free both regions on the IOP side and silence the voices.
 *
 * @param pAudioDec The audio decoder.
 * @return Always 1.
 * @ghidraAddress NTSC-U/C: 0x005678e0
 * @ghidraAddress PAL: 0x005a7da8
 */
int audioDecDelete(AudioDec *pAudioDec);

/**
 * Send staged bytes to the IOP side according to the transfer stage.
 *
 * @param pAudioDec The audio decoder.
 * @return The bytes sent.
 * @ghidraAddress NTSC-U/C: 0x00567670
 * @ghidraAddress PAL: 0x005a7b38
 */
int audioDecSendToIOP(AudioDec *pAudioDec);

/**
 * Report whether the bytes sent have filled the buffer on the IOP side.
 *
 * @param pAudioDec The audio decoder.
 * @return 1 when filled, otherwise 0.
 * @ghidraAddress NTSC-U/C: 0x00567a50
 * @ghidraAddress PAL: 0x005a7f18
 */
int audioDecIsPreset(AudioDec *pAudioDec);

/**
 * Raise the input volume, pass the buffer range to the sound driver, and enter streaming.
 *
 * @param pAudioDec The audio decoder.
 * @ghidraAddress NTSC-U/C: 0x00567a68
 * @ghidraAddress PAL: 0x005a7f30
 */
void audioDecStart(AudioDec *pAudioDec);

/**
 * Stop the sound driver and clear the audio decoder back to idle.
 *
 * @param pAudioDec The audio decoder.
 * @ghidraAddress NTSC-U/C: 0x00567ad8
 * @ghidraAddress PAL: 0x005a7fa0
 */
void audioDecReset(AudioDec *pAudioDec);

/**
 * Reset the offsets and restore the full capacity.
 *
 * @param pReadBuf The read buffer.
 * @ghidraAddress NTSC-U/C: 0x005cb2b8
 * @ghidraAddress PAL: 0x0060d218
 */
void readBufCreate(ReadBuf *pReadBuf);

/**
 * Delete a read buffer. The buffer has nothing to release.
 *
 * @param pReadBuf The read buffer.
 * @ghidraAddress NTSC-U/C: 0x005cb2d0
 * @ghidraAddress PAL: 0x0060d230
 */
void readBufDelete(ReadBuf *pReadBuf);

/**
 * Offer the write position.
 *
 * @param pReadBuf The read buffer.
 * @param ppPut Receives the write position when the ring has room.
 * @return The free bytes.
 * @ghidraAddress NTSC-U/C: 0x005cb2d8
 * @ghidraAddress PAL: 0x0060d238
 */
int readBufBeginPut(ReadBuf *pReadBuf, unsigned char **ppPut);

/**
 * Advance the write position past stored bytes, wrapping at the capacity.
 *
 * @param pReadBuf The read buffer.
 * @param nSize Bytes stored.
 * @return The bytes accepted, at most the free bytes.
 * @ghidraAddress NTSC-U/C: 0x005cb308
 * @ghidraAddress PAL: 0x0060d268
 */
int readBufEndPut(ReadBuf *pReadBuf, int nSize);

/**
 * Offer the read position.
 *
 * @param pReadBuf The read buffer.
 * @param ppGet Receives the read position when the ring is not empty.
 * @return The bytes in the ring.
 * @ghidraAddress NTSC-U/C: 0x005cb350
 * @ghidraAddress PAL: 0x0060d2b0
 */
int readBufBeginGet(ReadBuf *pReadBuf, unsigned char **ppGet);

/**
 * Discard consumed bytes.
 *
 * @param pReadBuf The read buffer.
 * @param nSize Bytes consumed.
 * @return The bytes discarded, at most the bytes in the ring.
 * @ghidraAddress NTSC-U/C: 0x005cb398
 * @ghidraAddress PAL: 0x0060d2f8
 */
int readBufEndGet(ReadBuf *pReadBuf, int nSize);

/**
 * Open a stream.
 *
 * A "cdrom0:" name is uppercased in place, its forward slashes become backslashes, and it gains
 * ";1" when it does not have a version. A name without a device opens on "host0:".
 *
 * @param pFile Receives the stream.
 * @param pszName The stream name.
 * @return 1, or 0 when the stream cannot be opened.
 * @ghidraAddress NTSC-U/C: 0x0058de70
 * @ghidraAddress PAL: 0x005d11c8
 */
int strFileOpen(StrFile *pFile, const char *pszName);

/**
 * Close a stream, stopping CD streaming and freeing its IOP heap for a stream on the disc.
 *
 * @param pFile The stream.
 * @return Always 1.
 * @ghidraAddress NTSC-U/C: 0x0058e130
 * @ghidraAddress PAL: 0x005d1488
 */
int strFileClose(StrFile *pFile);

/**
 * Read from a stream. A stream on the disc reads whole 2048-byte sectors.
 *
 * @param pFile The stream.
 * @param pBuffer Receives the data.
 * @param nSize Bytes to read.
 * @return The bytes read.
 * @ghidraAddress NTSC-U/C: 0x0058e180
 * @ghidraAddress PAL: 0x005d14d8
 */
int strFileRead(StrFile *pFile, void *pBuffer, int nSize);

/**
 * Create a frame queue and mark every slot empty.
 *
 * @param pVoBuf The frame queue.
 * @param pData Frame data area.
 * @param pTag DMA tag area.
 * @param nFrames Count of slots.
 * @ghidraAddress NTSC-U/C: 0x005d3fa0
 * @ghidraAddress PAL: 0x00616008
 */
void voBufCreate(VoBuf *pVoBuf, void *pData, void *pTag, int nFrames);

/**
 * Delete a frame queue. The queue has nothing to release.
 *
 * @param pVoBuf The frame queue.
 * @ghidraAddress NTSC-U/C: 0x005d40c0
 * @ghidraAddress PAL: 0x00616128
 */
void voBufDelete(VoBuf *pVoBuf);

/**
 * Report whether every slot of a frame queue is in use.
 *
 * @param pVoBuf The frame queue.
 * @return 1 when count equals size, otherwise 0.
 * @ghidraAddress NTSC-U/C: 0x005d3ff8
 * @ghidraAddress PAL: 0x00616060
 */
int voBufIsFull(VoBuf *pVoBuf);

/**
 * Clear GS memory to a colour.
 *
 * @param nRed Red of the colour.
 * @param nGreen Green of the colour.
 * @param nBlue Blue of the colour.
 * @param nWidth Width in pixels.
 * @param nHeight Height in pixels.
 * @ghidraAddress NTSC-U/C: 0x005d2860
 * @ghidraAddress PAL: 0x006148a8
 */
void clearGsMem(int nRed, int nGreen, int nBlue, int nWidth, int nHeight);

/**
 * Wait for the field to move off nWaitField, then start presenting decoded frames.
 *
 * @param nWaitField The field to wait out.
 * @ghidraAddress NTSC-U/C: 0x005d3008
 * @ghidraAddress PAL: 0x00615050
 */
void startDisplay(int nWaitField);

/**
 * Stop presenting decoded frames.
 *
 * @ghidraAddress NTSC-U/C: 0x005d3050
 * @ghidraAddress PAL: 0x00615098
 */
void endDisplay(void);

/**
 * Release the frame the display finished showing back to the frame queue.
 *
 * @param nCause The interrupt cause.
 * @return Always 0.
 * @ghidraAddress NTSC-U/C: 0x005d3068
 * @ghidraAddress PAL: 0x006150b0
 */
int handler_endimage(int nCause);

/**
 * Present the next field of the oldest decoded frame on each vertical blank.
 *
 * @param nCause The interrupt cause.
 * @return Always 0.
 * @ghidraAddress NTSC-U/C: 0x005d2e38
 * @ghidraAddress PAL: 0x00614e80
 */
int vblankHandler(int nCause);

/**
 * Print an error line. The game defines the routine, and the sample units call it.
 *
 * @param pszMessage The message.
 * @ghidraAddress NTSC-U/C: 0x00510f20
 * @ghidraAddress PAL: 0x00551198
 */
void ErrMessage(char *pszMessage);

/**
 * Rotate the ready queue at the game's default thread priority. The game defines the routine, and
 * the sample units call it.
 *
 * @ghidraAddress NTSC-U/C: 0x00510f48
 * @ghidraAddress PAL: 0x005511c0
 */
void switchThread(void);

#ifdef __cplusplus
}
#endif

#endif
