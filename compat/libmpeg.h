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
    int alignmentPadding; /**< +0x0c. Aligns the stamps; never read or written. */
    long long pts; /**< +0x10. Presentation time stamp, or -1 when absent. */
    long long dts; /**< +0x18. Decoding time stamp, or -1 when absent. */
    unsigned long long flags; /**< +0x20. Picture header flags. */
    long long pts2nd; /**< +0x28. Second field presentation time stamp, or -1. */
    long long dts2nd; /**< +0x30. Second field decoding time stamp, or -1. */
    unsigned long long flags2nd; /**< +0x38. Second field picture header flags. */
    void *pContext; /**< +0x40. Decoder context. Inferred. */
    int tailPadding; /**< +0x44. Pads the structure to 0x48 bytes; never read or written. */
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

// Reports whether the decoder has met the sequence end code. The decode worker spins on it.
int sceMpegIsEnd(void *pDecoder);

// Decodes pictures until one is output, colour converted into the buffer of nMacroblocks
// macroblocks at pPicture. Negative when the picture buffer is misaligned.
int sceMpegGetPicture(void *pDecoder, void *pPicture, int nMacroblocks);

// Resets the decoder state and the IPU after the input ends.
void sceMpegReset(void *pDecoder);

// Creates the decoder context over the work area and returns the committed write pointer,
// which the sample ignores. The work area starts with seven callback slots, the stream table
// pointer, and the stream count ahead of the decoder state and the input ring.
void *sceMpegCreateDecoderContext(void *pDecoder, void *pWork, int nWorkSize);

// Registers a callback in the given slot and returns the previous callback.
void *sceMpegSetCallbackSlot(void *pDecoder, int nSlot, void *pfnCallback, void *pData);

// Invokes the slot selected by the entry key with the decoder, the entry, and the slot data,
// returning the callback result or zero when any link is missing.
int sceMpegInvokeCallbackSlot(void *pDecoder, void *pEntry);

// Deletes the decoder, which has nothing to release, and reports one.
int sceMpegDelete(void *pDecoder);

// Sets how many pictures of each coding type to decode, -1 for all of them.
void sceMpegSetDecodeMode(void *pDecoder, int nIntra, int nPredicted, int nBidirectional);

// Reports whether no picture has been decoded since the last flush.
int sceMpegIsRefBuffEmpty(void *pDecoder);

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

// Decodes one picture like sceMpegGetPicture() but copies the raw macroblocks without colour
// conversion.
int sceMpegGetPictureRAW8(void *pDecoder, void *pPicture, int nMacroblocks);

// Decodes one picture like sceMpegGetPictureRAW8() into a buffer of nMbWidth by nMbHeight
// macroblocks, which bounds the picture by width and height instead of by count.
int sceMpegGetPictureRAW8xy(void *pDecoder, void *pPicture, int nMbWidth, int nMbHeight);

// Selects the IPU's MPEG-1 mode and places the two motion compensation buffers in the scratchpad.
void sceMpegResetMcBuffers(void);

// Stops the IPU transfer channels and resets the IPU for the decoder.
void sceMpegResetIpuChannels(void *pDecoder);

// Assumes an MPEG-1 stream until a sequence extension arrives.
void sceMpegSelectMpeg1(void);

// Reports a decoder error with the given message, through the slot callback when one is
// installed and through the error line otherwise.
void sceMpegRaiseError(const char *pFormat);

// Prints a decoder error line in the stock "[MPEG ERROR]%s" format.
void sceMpegPrintErrorLine(const char *pMessage);

// Reports a picture error with the given format and arguments.
void sceMpegReportErrorFormatted(const char *pFormat, ...);

// Parses headers up to the next picture header and returns its picture_coding_type, or 0 at
// the sequence end code.
int sceMpegNextPictureHeader(void);

// Resets the IPU and loads the default quantiser matrices, the colour lookup table, and the
// threshold, returning the final IPU control word.
int sceIpuResetAndLoadTables(void);

#ifdef __cplusplus
}
#endif

#endif
