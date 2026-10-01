#ifndef EZMPEG_H
#define EZMPEG_H

// Build support for Sony's ezmpeg sample. The sample's decoder, reader, and display units
// reconstruct under src/ezmpeg/, and cutscene.c drives them. The types and entry points
// cutscene.c touches are declared here with the layouts and signatures the shipped program's
// bodies and call sites prove, under the sample's own names.

#include <libmpeg.h>

#ifdef __cplusplus
extern "C" {
#endif

// The video decoder, 0xb8 bytes. The decoder state follows the embedded sceMpeg and the stream
// set, and the two handler identifiers cutscene.c registers close the structure.
typedef struct {
    sceMpeg mpeg;
    unsigned char vibuf[0x60]; // Input ring, cast to a ViBuf by the decoder unit. +0x48
    int state;
    int reserved; // Never read or written by the decoder unit. +0xac
    int hid_endimage;
    int hid_vblank;
} VideoDec;

// The audio decoder, 0x5c bytes. The transfer stage, the staging buffer on the Emotion Engine
// side, and the two processor side regions live here with the counts the transfers advance.
typedef struct {
    int state; // Stage of the transfer (idle, priming, streaming, or stopping). +0x00
    unsigned char header[0x28]; // Stream header, filled byte by byte before the samples. +0x04
    int headerCount; // Header bytes received so far. +0x2c
    unsigned char *buffer; // Staging buffer on the Emotion Engine side. +0x30
    int put; // Staging write offset. +0x34
    int count; // Staged bytes not yet transferred, drained by each transfer. +0x38
    int bufferSize; // Staging buffer size in bytes. +0x3c
    int totalBytes; // Bytes staged since the last reset. +0x40
    int iopBuffer; // Buffer address on the Input Output Processor side. +0x44
    int iopBufferSize; // Buffer size on the Input Output Processor side. +0x48
    int iopOffset; // Write offset into the processor-side buffer. +0x4c
    int iopPauseOffset; // Play offset the driver reported at the stop call. +0x50
    int totalBytesSent; // Bytes handed to the processor side since the last reset. +0x54
    int iopExtra; // Second processor-side region, with the preset block. +0x58
} AudioDec;

// The ring buffer between the file reader and the demultiplexer, 0x5000c bytes. The data comes
// first, then the put offset, the byte count, and the capacity.
typedef struct {
    unsigned char data[0x50000];
    int put;
    int count;
    int size;
} ReadBuf;

// An open stream, 0x38 bytes. A stream on the disc is read through the CD streaming functions and
// any other stream through the file descriptor at +0x30.
typedef struct {
    int isOnCD;
    int size;
    unsigned char cdFile[0x28]; // The sceCdlFILE entry, then the IOP heap pointer at +0x2c. +0x08
    int fd;
    int reserved; // Never read or written by the stream unit. +0x34
} StrFile;

// The decoded-frame queue, 0x14 bytes. voBufIsFull() compares count with size. The display's
// interrupt handler lowers the count while the decode worker waits on it.
typedef struct {
    void *data;
    void *tag;
    int write; // Slot the decoder fills next.
    volatile int count;
    int size;
} VoBuf;

// Values videoDecGetState() returns and videoDecAbort() stores.
#define VD_STATE_ABORT 1
#define VD_STATE_END 3

// The sample's callback shapes.
typedef int (*VideoDecCallback)(sceMpeg *pMpeg, void *pCallbackData, void *pData);
typedef int (*InterruptHandler)(int nCause);

void videoDecCreate(VideoDec *pVideoDec,
                    unsigned char *pWork,
                    int nWorkSize,
                    void *pData,
                    void *pTag,
                    int nTagSize,
                    void *pTimeStamps,
                    int nTimeStamps);
int videoDecSetStream(
    VideoDec *pVideoDec, int nType, int nChannel, VideoDecCallback pfnCallback, void *pData);
int videoDecGetState(VideoDec *pVideoDec);
void videoDecAbort(VideoDec *pVideoDec);
int videoDecFlush(VideoDec *pVideoDec);
int videoDecIsFlushed(VideoDec *pVideoDec);
int videoDecDelete(VideoDec *pVideoDec);
void videoDecMain(VideoDec *pVideoDec);
int videoCallback(sceMpeg *pMpeg, void *pCallbackData, void *pData);
int pcmCallback(sceMpeg *pMpeg, void *pCallbackData, void *pData);

int audioDecCreate(AudioDec *pAudioDec,
                   unsigned char *pBuffer,
                   int nBufferSize,
                   int nIopBufferSize);
int audioDecDelete(AudioDec *pAudioDec);
int audioDecSendToIOP(AudioDec *pAudioDec);
int audioDecIsPreset(AudioDec *pAudioDec);
void audioDecStart(AudioDec *pAudioDec);
void audioDecReset(AudioDec *pAudioDec);

void readBufCreate(ReadBuf *pReadBuf);
void readBufDelete(ReadBuf *pReadBuf);
int readBufBeginPut(ReadBuf *pReadBuf, unsigned char **ppPut);
int readBufEndPut(ReadBuf *pReadBuf, int nSize);
int readBufBeginGet(ReadBuf *pReadBuf, unsigned char **ppGet);
int readBufEndGet(ReadBuf *pReadBuf, int nSize);

int strFileOpen(StrFile *pFile, const char *pszName);
int strFileClose(StrFile *pFile);
int strFileRead(StrFile *pFile, void *pBuffer, int nSize);

void voBufCreate(VoBuf *pVoBuf, void *pData, void *pTag, int nFrames);
void voBufDelete(VoBuf *pVoBuf);
int voBufIsFull(VoBuf *pVoBuf);

void clearGsMem(int nRed, int nGreen, int nBlue, int nWidth, int nHeight);
void startDisplay(int nWaitField);
void endDisplay(void);
int handler_endimage(int nCause);
int vblankHandler(int nCause);

// Defined by the game in cutscene.c and called by the sample units.
void ErrMessage(char *pszMessage);
void switchThread(void);

#ifdef __cplusplus
}
#endif

#endif
