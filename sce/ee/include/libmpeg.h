#ifndef LIBMPEG_H
#define LIBMPEG_H

#ifdef __cplusplus
extern "C" {
#endif

/** MPEG library: program stream demultiplexing and IPU video decoding. */

/**
 * Decoder state, 0x48 bytes.
 *
 * The sample's VideoDec embeds one at its start and places its next member at +0x48. The picture
 * path fills the time stamps and flags of each picture it outputs.
 */
typedef struct {
    int width;                   /*!< Picture width. */
    int height;                  /*!< Picture height. */
    int frameCount;              /*!< Count of decoded frames. */
    int alignmentPadding;        /*!< Aligns the stamps. Never read or written. +0x0c */
    long long pts;               /*!< Presentation time stamp, or -1 when absent. */
    long long dts;               /*!< Decoding time stamp, or -1 when absent. */
    unsigned long long flags;    /*!< Picture header flags. */
    long long pts2nd;            /*!< Second field presentation time stamp, or -1. */
    long long dts2nd;            /*!< Second field decoding time stamp, or -1. */
    unsigned long long flags2nd; /*!< Second field picture header flags. */
    void *pContext;              /*!< Decoder context (inferred). */
    int tailPadding;             /*!< Pads to 0x48 bytes. Never read or written. +0x44 */
} sceMpeg;

/** Stream types. The game registers these two. */
#define sceMpegStrM2V 0 /*!< MPEG-2 video. */
#define sceMpegStrPCM 2 /*!< PCM audio. */

/** Callback type the demultiplexer reports in sceMpegCbDataStr::type for each stream packet. */
#define sceMpegCbStr 6

/**
 * One demultiplexed stream packet, as sceMpegDemuxPssRing() passes it to a stream callback.
 *
 * The pointers address the input ring and may wrap at its end.
 */
typedef struct {
    int type;              /*!< Always sceMpegCbStr. */
    unsigned char *header; /*!< The packet start code. */
    unsigned char *data;   /*!< The packet payload. */
    unsigned int len;      /*!< Payload length in bytes. */
    long long pts;         /*!< Presentation time stamp, or -1 when absent. */
    long long dts;         /*!< Decoding time stamp, or -1 when absent. */
} sceMpegCbDataStr;

/**
 * A decoder or stream callback.
 *
 * The callback data is an sceMpegCbDataStr for a stream callback. A stream callback returns zero
 * to stop the demultiplexer.
 */
typedef int (*sceMpegCallback)(sceMpeg *pMpeg, void *pCallbackData, void *pData);

/**
 * Stop the two IPU DMA channels and clear their counts, then reset the IPU and load its tables.
 *
 * @return The sceIpuResetAndLoadTables() result.
 * @ghidraAddress NTSC-U/C: 0x005e0490
 * @ghidraAddress PAL: 0x006223d0
 */
int sceMpegInit(void);

/**
 * Demultiplex a pack stream without a ring buffer, passing no buffer and -1 as its size.
 *
 * @param pMpeg The decoder.
 * @param pStart The stream data.
 * @param nSize Size of the data in bytes.
 * @return The bytes consumed.
 * @ghidraAddress NTSC-U/C: 0x005caa58
 * @ghidraAddress PAL: 0x0060c9b8
 */
int sceMpegDemuxPss(sceMpeg *pMpeg, unsigned char *pStart, int nSize);

/**
 * Demultiplex nSize bytes of a PSS stream starting at pStart inside a ring buffer.
 *
 * @param pMpeg The decoder.
 * @param pStart The stream data, inside the ring.
 * @param nSize Size of the data in bytes.
 * @param pBuffer The ring buffer.
 * @param nBufferSize Size of the ring in bytes.
 * @return The bytes consumed.
 * @ghidraAddress NTSC-U/C: 0x005ca768
 * @ghidraAddress PAL: 0x0060c6c8
 */
int sceMpegDemuxPssRing(
    sceMpeg *pMpeg, unsigned char *pStart, int nSize, unsigned char *pBuffer, int nBufferSize);

/**
 * Report whether the decoder has met the sequence end code. The decode worker spins on it.
 *
 * @param pDecoder The decoder.
 * @return Nonzero after the sequence end code.
 * @ghidraAddress NTSC-U/C: 0x005e08c8
 * @ghidraAddress PAL: 0x00622808
 */
int sceMpegIsEnd(void *pDecoder);

/**
 * Decode pictures until one is output, colour converted into a buffer of nMacroblocks macroblocks.
 *
 * @param pDecoder The decoder.
 * @param pPicture The picture buffer.
 * @param nMacroblocks Size of the buffer in macroblocks.
 * @return Negative when the picture buffer is misaligned.
 * @ghidraAddress NTSC-U/C: 0x005e07b0
 * @ghidraAddress PAL: 0x006226f0
 */
int sceMpegGetPicture(void *pDecoder, void *pPicture, int nMacroblocks);

/**
 * Reset the decoder state and the IPU after the input ends.
 *
 * @param pDecoder The decoder.
 * @ghidraAddress NTSC-U/C: 0x005e08e8
 * @ghidraAddress PAL: 0x00622828
 */
void sceMpegReset(void *pDecoder);

/**
 * Create the decoder context over a work area.
 *
 * The work area starts with seven callback slots, the stream table pointer, and the stream count
 * ahead of the decoder state and the input ring.
 *
 * @param pDecoder The decoder.
 * @param pWork The work area.
 * @param nWorkSize Size of the work area in bytes.
 * @return The committed write pointer. The sample ignores it.
 * @ghidraAddress NTSC-U/C: 0x005e0530
 * @ghidraAddress PAL: 0x00622470
 */
void *sceMpegCreateDecoderContext(void *pDecoder, void *pWork, int nWorkSize);

/**
 * Register a callback in a slot.
 *
 * @param pDecoder The decoder.
 * @param nSlot The slot.
 * @param pfnCallback The callback.
 * @param pData Data passed to the callback.
 * @return The previous callback.
 * @ghidraAddress NTSC-U/C: 0x005e0990
 * @ghidraAddress PAL: 0x006228d0
 */
void *sceMpegSetCallbackSlot(void *pDecoder, int nSlot, void *pfnCallback, void *pData);

/**
 * Invoke the slot the entry key selects, with the decoder, the entry, and the slot data.
 *
 * @param pDecoder The decoder.
 * @param pEntry The entry.
 * @return The callback result, or zero when a link is missing.
 * @ghidraAddress NTSC-U/C: 0x005e09b8
 * @ghidraAddress PAL: 0x006228f8
 */
int sceMpegInvokeCallbackSlot(void *pDecoder, void *pEntry);

/**
 * Delete the decoder. The decoder has nothing to release.
 *
 * @param pDecoder The decoder.
 * @return Always 1.
 * @ghidraAddress NTSC-U/C: 0x005e0770
 * @ghidraAddress PAL: 0x006226b0
 */
int sceMpegDelete(void *pDecoder);

/**
 * Set how many pictures of each coding type to decode.
 *
 * @param pDecoder The decoder.
 * @param nIntra Count of intra pictures, or -1 for all.
 * @param nPredicted Count of predicted pictures, or -1 for all.
 * @param nBidirectional Count of bidirectional pictures, or -1 for all.
 * @ghidraAddress NTSC-U/C: 0x005e0890
 * @ghidraAddress PAL: 0x006227d0
 */
void sceMpegSetDecodeMode(void *pDecoder, int nIntra, int nPredicted, int nBidirectional);

/**
 * Report whether no picture has been decoded since the last flush.
 *
 * @param pDecoder The decoder.
 * @return 1 when no picture has been decoded, otherwise 0.
 * @ghidraAddress NTSC-U/C: 0x005e08d8
 * @ghidraAddress PAL: 0x00622818
 */
int sceMpegIsRefBuffEmpty(void *pDecoder);

/**
 * Register a stream callback for a type and channel.
 *
 * A duplicate key overwrites the entry in place and still increments the count.
 *
 * @param pDecoder The decoder.
 * @param nType The stream type.
 * @param nChannel The channel.
 * @param pfnCallback The callback.
 * @param pData Data passed to the callback.
 * @return The previous callback for a duplicate key, or null for a new entry.
 * @ghidraAddress NTSC-U/C: 0x005caa78
 * @ghidraAddress PAL: 0x0060c9d8
 */
sceMpegCallback sceMpegAddStrCallback(
    void *pDecoder, int nType, int nChannel, sceMpegCallback pfnCallback, void *pData);

/**
 * Reset a ring to a base and size. The write and commit positions start at the base.
 *
 * @param pRing The ring.
 * @param pBase The base.
 * @param nSize Size of the ring in bytes.
 * @ghidraAddress NTSC-U/C: 0x005e0ad8
 * @ghidraAddress PAL: 0x00622a18
 */
void sceMpegResetRingPointers(void *pRing, void *pBase, int nSize);

/**
 * Commit the write position of a ring.
 *
 * @param pRing The ring.
 * @return The write position.
 * @ghidraAddress NTSC-U/C: 0x005e0af0
 * @ghidraAddress PAL: 0x00622a30
 */
int sceMpegCommitWritePointer(void *pRing);

/**
 * Rewind the write position of a ring to the committed position.
 *
 * @param pRing The ring.
 * @ghidraAddress NTSC-U/C: 0x005e0b00
 * @ghidraAddress PAL: 0x00622a40
 */
void sceMpegRewindWritePointer(void *pRing);

/**
 * Carve an aligned chunk of nNeed bytes off the write position of a ring.
 *
 * @param pRing The ring.
 * @param nNeed Size of the chunk in bytes.
 * @param nAlign Alignment of the chunk.
 * @return The chunk, or null when the buffer ends first or the alignment is zero.
 * @ghidraAddress NTSC-U/C: 0x005e0b10
 * @ghidraAddress PAL: 0x00622a50
 */
void *sceMpegCheckWorkAreaSize(void *pRing, int nNeed, int nAlign);

/**
 * Decode one picture like sceMpegGetPicture() but copy the raw macroblocks without colour
 * conversion.
 *
 * @param pDecoder The decoder.
 * @param pPicture The picture buffer.
 * @param nMacroblocks Size of the buffer in macroblocks.
 * @return Negative when the picture buffer is misaligned.
 * @ghidraAddress NTSC-U/C: 0x005e07f8
 * @ghidraAddress PAL: 0x00622738
 */
int sceMpegGetPictureRAW8(void *pDecoder, void *pPicture, int nMacroblocks);

/**
 * Decode one picture like sceMpegGetPictureRAW8() into a buffer of nMbWidth by nMbHeight
 * macroblocks.
 *
 * The buffer bounds the picture by width and height instead of by count.
 *
 * @param pDecoder The decoder.
 * @param pPicture The picture buffer.
 * @param nMbWidth Width of the buffer in macroblocks.
 * @param nMbHeight Height of the buffer in macroblocks.
 * @return Negative when the picture buffer is misaligned.
 * @ghidraAddress NTSC-U/C: 0x005e0840
 * @ghidraAddress PAL: 0x00622780
 */
int sceMpegGetPictureRAW8xy(void *pDecoder, void *pPicture, int nMbWidth, int nMbHeight);

/**
 * Select the MPEG-1 mode of the IPU and place the two motion compensation buffers in the
 * scratchpad.
 *
 * @ghidraAddress NTSC-U/C: 0x0060dd78
 * @ghidraAddress PAL: 0x0064e9e8
 */
void sceMpegResetMcBuffers(void);

/**
 * Stop the IPU transfer channels and reset the IPU for the decoder.
 *
 * @param pDecoder The decoder.
 * @ghidraAddress NTSC-U/C: 0x0060ddc8
 * @ghidraAddress PAL: 0x0064ea38
 */
void sceMpegResetIpuChannels(void *pDecoder);

/**
 * Assume an MPEG-1 stream until a sequence extension arrives.
 *
 * @ghidraAddress NTSC-U/C: 0x0060dce0
 * @ghidraAddress PAL: 0x0064e950
 */
void sceMpegSelectMpeg1(void);

/**
 * Report a decoder error.
 *
 * The report goes through the slot callback when one is installed and through the error line
 * otherwise.
 *
 * @param pFormat The message.
 * @ghidraAddress NTSC-U/C: 0x0060ded0
 * @ghidraAddress PAL: 0x0064eb40
 */
void sceMpegRaiseError(const char *pFormat);

/**
 * Print a decoder error line in the stock "[MPEG ERROR]%s" format.
 *
 * @param pMessage The message.
 * @ghidraAddress NTSC-U/C: 0x0060de90
 * @ghidraAddress PAL: 0x0064eb00
 */
void sceMpegPrintErrorLine(const char *pMessage);

/**
 * Report a picture error.
 *
 * @param pFormat The format.
 * @param ... The format arguments.
 * @ghidraAddress NTSC-U/C: 0x0060dea0
 * @ghidraAddress PAL: 0x0064eb10
 */
void sceMpegReportErrorFormatted(const char *pFormat, ...);

/**
 * Parse headers up to the next picture header.
 *
 * @return Its picture_coding_type, or 0 at the sequence end code.
 * @ghidraAddress NTSC-U/C: 0x0060ba60
 * @ghidraAddress PAL: 0x0064c6d0
 */
int sceMpegNextPictureHeader(void);

/**
 * Reset the IPU and load the default quantiser matrices, the colour lookup table, and the
 * threshold.
 *
 * @return The final IPU control word.
 * @ghidraAddress NTSC-U/C: 0x0061da40
 * @ghidraAddress PAL: 0x0065e5d0
 */
int sceIpuResetAndLoadTables(void);

#ifdef __cplusplus
}
#endif

#endif
