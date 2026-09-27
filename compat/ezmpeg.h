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
    unsigned char mUnknown48[0x60];
    int state;
    int mUnknownac;
    int hid_vblank;
    int hid_endimage;
} VideoDec;

// The audio decoder, 0x5c bytes. The transfer stage, the staging buffer on the Emotion Engine
// side, and the two processor side regions live here with the counts the transfers advance.
typedef struct {
    int state; // +0x00: stage of the transfer, idle, priming, streaming, or stopping.
    unsigned char reserved04[0x28]; // +0x04: untouched by the decoder unit.
    int field2c; // +0x2c: cleared on creation and on reset.
    unsigned char *buffer; // +0x30: staging buffer on the Emotion Engine side.
    int field34; // +0x34: base count the block size derives from.
    int field38; // +0x38: pending count drained by each transfer.
    int bufferSize; // +0x3c: staging buffer size in bytes.
    int field40; // +0x40: cleared on creation and on reset.
    int iopBuffer; // +0x44: buffer address on the Input Output Processor side.
    int iopBufferSize; // +0x48: buffer size on the Input Output Processor side.
    int iopOffset; // +0x4c: write offset into the processor side buffer.
    int field50; // +0x50: driver reply kept across the stop call.
    int field54; // +0x54: count already handed to the processor side.
    int iopExtra; // +0x58: second processor side region, holding the preset block.
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
    unsigned char mUnknown08[0x28];
    int fd;
    int mUnknown34;
} StrFile;

// The decoded-frame queue, 0x14 bytes. voBufIsFull() compares count with size.
typedef struct {
    void *data;
    void *tag;
    int mUnknown08;
    int count;
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
void audioDecSendToIOP(AudioDec *pAudioDec);
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
