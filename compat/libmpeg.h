#ifndef LIBMPEG_H
#define LIBMPEG_H

// Build support for the open-source SDK. Sony's libmpeg is not part of ps2sdk. The entry points
// the reconstruction calls are declared here with the signatures the shipped program's bodies and
// call sites prove. Nothing here is reconstructed source.

#ifdef __cplusplus
extern "C" {
#endif

// The decoder state, 0x48 bytes. The sample's VideoDec embeds one at its start and places its next
// member at +0x48. The picture path fills the time stamps and flags of each picture it outputs.
typedef struct {
    int width; /**< +0x00. Picture width. */
    int height; /**< +0x04. Picture height. */
    int frameCount; /**< +0x08. Count of decoded frames. */
    int mUnknown0C; /**< +0x0c. Not initialised at creation. */
    long long pts; /**< +0x10. Presentation time stamp, or -1 when absent. */
    long long dts; /**< +0x18. Decoding time stamp, or -1 when absent. */
    unsigned long long flags; /**< +0x20. Picture header flags. */
    long long pts2nd; /**< +0x28. Second field presentation time stamp, or -1. */
    long long dts2nd; /**< +0x30. Second field decoding time stamp, or -1. */
    unsigned long long flags2nd; /**< +0x38. Second field picture header flags. */
    void *pContext; /**< +0x40. Decoder context. Inferred. */
    int mUnknown44; /**< +0x44. */
} sceMpeg;

// Stream types, of which the game registers two.
#define sceMpegStrM2V 0
#define sceMpegStrPCM 2

// Callback type the demultiplexer reports in sceMpegCbDataStr::type for each stream packet.
#define sceMpegCbStr 6

// One demultiplexed stream packet, as sceMpegDemuxPssRing() passes it to a stream callback. The
// pointers address the input ring and may wrap at its end.
typedef struct {
    int type; /**< +0x00. Always sceMpegCbStr. */
    unsigned char *header; /**< +0x04. The packet start code. */
    unsigned char *data; /**< +0x08. The packet payload. */
    unsigned int len; /**< +0x0c. Payload length in bytes. */
    long long pts; /**< +0x10. Presentation time stamp, or -1 when absent. */
    long long dts; /**< +0x18. Decoding time stamp, or -1 when absent. */
} sceMpegCbDataStr;

// A decoder or stream callback. The callback data is an sceMpegCbDataStr for a stream callback.
// A stream callback returns zero to stop the demultiplexer.
typedef int (*sceMpegCallback)(sceMpeg *pMpeg, void *pCallbackData, void *pData);

int sceMpegInit(void);

// Demultiplexes a pack stream without a ring buffer, passing no buffer and -1 as its size.
int sceMpegDemuxPss(sceMpeg *pMpeg, unsigned char *pStart, int nSize);

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

// Creates the decoder context over the work area and returns the committed write pointer,
// which the sample ignores. The work area starts with seven callback slots, the stream table
// pointer, and the stream count ahead of the decoder state and the input ring.
void *sceMpegCreateDecoderContext(void *pDecoder, void *pWork, int nWorkSize);

// Registers a callback in the given slot and returns the previous callback.
void *sceMpegSetCallbackSlot(void *pDecoder, int nSlot, void *pfnCallback, void *pData);

// Invokes the slot selected by the entry key with the decoder, the entry, and the slot data,
// returning the callback result or zero when any link is missing.
int sceMpegInvokeCallbackSlot(void *pDecoder, void *pEntry);

// Reports one. Inferred.
int sceMpegReturnOne(void *pDecoder);

// Resets the decoder with three values.
void sceMpegSub005e0890(void *pDecoder, int nArgA, int nArgB, int nArgC);

// Reports whether word four of the decoder context is clear.
int sceMpegIsContextWordFourClear(void *pDecoder);

// Registers a stream callback for the given type and channel. A duplicate key overwrites the
// entry in place, still bumps the count, and returns the previous callback; a fresh entry
// returns null.
sceMpegCallback sceMpegAddStrCallback(
    void *pDecoder, int nType, int nChannel, sceMpegCallback pfnCallback, void *pData);

// Resets the ring to the given base and size. The write and commit positions start at base.
void sceMpegResetRingPointers(void *pRing, void *pBase, int nSize);

// Commits the write position and returns it.
int sceMpegCommitWritePointer(void *pRing);

// Rewinds the write position to the committed one.
void sceMpegRewindWritePointer(void *pRing);

// Carves a need-sized, aligned chunk off the ring write position, returning its base or null
// when the buffer ends first or the alignment is zero.
void *sceMpegCheckWorkAreaSize(void *pRing, int nNeed, int nAlign);

// Decodes one picture like sceMpegSub005e07b0 but leaves the busy word clear.
int sceMpegSub005e07f8(void *pDecoder, void *pPicture, int nMode);

// Arms a picture with scaled strides: the second clear word takes nB shifted by four, the mode
// takes nA times nB, and the first clear word takes nA shifted by four.
int sceMpegSub005e0840(void *pDecoder, void *pPicture, int nA, int nB);

// Sets IPU control bit twenty-three and resets the IPU table from the IPU base address.
void sceMpegDisableIpuControlBit(void);

// Drains the IPU fifo for the decoder.
void sceMpegSub0060ddc8(void *pDecoder);

// Enables IPU control bit twenty-three.
void sceIpuEnableControlBitTwentyThree(void);

// Reports a decoder error with the given message, through the slot callback when one is
// installed and through the error line otherwise.
void sceMpegRaiseError(const char *pFormat);

// Prints a decoder error line in the stock "[MPEG ERROR]%s" format.
void sceMpegPrintErrorLine(const char *pMessage);

// Reports a picture error with the given format and arguments.
void sceMpegReportErrorFormatted(const char *pFormat, ...);

// Polls the picture engine and returns its state.
int sceMpegSub0060ba60(void);

// Finishes decoder initialisation and returns its status.
int sceMpegSub0061da40(void);

#ifdef __cplusplus
}
#endif

#endif
