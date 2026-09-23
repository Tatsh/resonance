#ifndef EZMPEG_H
#define EZMPEG_H

// Build support for Sony's ezmpeg sample. The game links the sample's decoder, reader, and display
// units as shipped with the SDK, and cutscene.c drives them. They are not reconstructed. The types
// and entry points cutscene.c touches are declared here with the layouts and signatures the
// shipped program's bodies and call sites prove, under the sample's own names.

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

// The audio decoder. The game touches it only through the calls below, and the next global in
// the image bounds it at 0x5c bytes.
typedef struct {
    unsigned char mUnknown00[0x5c];
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
