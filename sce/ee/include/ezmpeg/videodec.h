#ifndef EZMPEG_VIDEODEC_H
#define EZMPEG_VIDEODEC_H

// The video decoder of Sony's ezmpeg sample, videodec.c. The decoder consumes the input ring of
// vibuf.c and stages pictures into the frame queue of vobuf.c, while the playback driver owns the
// decoder instance. The decoder entry points declared in <ezmpeg.h> are not repeated here. This
// header declares the input entry points the stream callbacks of the callback unit call.

#include <ezmpeg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Report the number of input bytes queued for the demultiplexer.
 *
 * The count is read from the input ring embedded in the decoder.
 *
 * @param pVideoDec The decoder.
 * @return The queued byte count.
 * @ghidraAddress NTSC-U/C: 0x00569458
 * @ghidraAddress PAL: 0x005a9920
 */
int videoDecInputCount(VideoDec *pVideoDec);

/**
 * Report the free space left in the input ring.
 *
 * The free region can wrap around the end of the ring, so both halves are summed.
 *
 * @param pVideoDec The decoder.
 * @return The free byte count.
 * @ghidraAddress NTSC-U/C: 0x00569478
 * @ghidraAddress PAL: 0x005a9940
 */
int videoDecInputSpaceCount(VideoDec *pVideoDec);

/**
 * Set how many pictures of each coding type the decoder decodes.
 *
 * The three limits are forwarded to sceMpegSetDecodeMode(). That routine records them in the
 * decoder context. A limit of -1 decodes every picture of that type.
 *
 * @param pVideoDec The decoder.
 * @param nIntra The intra picture limit.
 * @param nPredicted The predicted picture limit.
 * @param nBidirectional The bidirectional picture limit.
 * @ghidraAddress NTSC-U/C: 0x005694b0
 * @ghidraAddress PAL: 0x005a9978
 */
void videoDecSetDecodeMode(VideoDec *pVideoDec, int nIntra, int nPredicted, int nBidirectional);

/**
 * Expose the free input regions for writing.
 *
 * The region can wrap around the end of the ring, in which case the second pair describes the
 * wrapped head.
 *
 * @param pVideoDec The decoder.
 * @param ppPut The free region.
 * @param pPutSize The size of the free region.
 * @param ppWrappedPut The wrapped head, or a null pointer when nothing wraps.
 * @param pWrappedSize The size of the wrapped head.
 * @ghidraAddress NTSC-U/C: 0x00569620
 * @ghidraAddress PAL: 0x005a9ae8
 */
void videoDecBeginPut(VideoDec *pVideoDec,
                      unsigned char **ppPut,
                      int *pPutSize,
                      unsigned char **ppWrappedPut,
                      int *pWrappedSize);

/**
 * Commit written input bytes to the ring.
 *
 * @param pVideoDec The decoder.
 * @param nSize The number of bytes written.
 * @ghidraAddress NTSC-U/C: 0x00569640
 * @ghidraAddress PAL: 0x005a9b08
 */
void videoDecEndPut(VideoDec *pVideoDec, int nSize);

/**
 * Queue a time stamp for the input bytes just committed.
 *
 * The stamp travels with the bytes through the demultiplexer so the picture can be presented on
 * time.
 *
 * @param pVideoDec The decoder.
 * @param nFirstStamp The first stamp value.
 * @param nSecondStamp The second stamp value.
 * @param pOffset The committed bytes.
 * @param nSize The number of committed bytes.
 * @return Non-zero when the stamp was queued.
 * @ghidraAddress NTSC-U/C: 0x00569660
 * @ghidraAddress PAL: 0x005a9b28
 */
int videoDecPutTs(VideoDec *pVideoDec,
                  long long nFirstStamp,
                  long long nSecondStamp,
                  unsigned char *pOffset,
                  int nSize);

/**
 * Copy two source spans into two destination spans.
 *
 * The stream callbacks call the byte-identical copy at `0x0059b1a0`; this definition serves both.
 * Returns zero when the destinations hold fewer bytes than the sources.
 *
 * @param pDestA First destination span.
 * @param nDestA First destination length.
 * @param pDestB Second destination span.
 * @param nDestB Second destination length.
 * @param pSrcA First source span.
 * @param nSrcA First source length.
 * @param pSrcB Second source span.
 * @param nSrcB Second source length.
 * @return The copied byte count, or zero when the destinations are short.
 * @ghidraAddress NTSC-U/C: 0x005697f0
 * @ghidraAddress PAL: 0x005a9cb8
 */
int cpy2area(unsigned char *pDestA,
             int nDestA,
             unsigned char *pDestB,
             int nDestB,
             unsigned char *pSrcA,
             int nSrcA,
             unsigned char *pSrcB,
             int nSrcB);

#ifdef __cplusplus
}
#endif

#endif
