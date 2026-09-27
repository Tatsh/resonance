#ifndef EZMPEG_DISP_H
#define EZMPEG_DISP_H

// The display unit of Sony's ezmpeg sample, disp.c. The unit clears the graphics memory, builds
// the display packets for decoded pictures, shows them across the vertical blank, and hosts the
// two stream callbacks. The entry points the playback driver and the stream callbacks call
// directly are declared in <ezmpeg.h>, which this header includes so that consumers gain them
// with a single include. This header declares the image tag builder, which the decoder worker
// calls through the decode loop.

#include <ezmpeg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Build the display packet for one field of the decoded picture.
 *
 * A zero field flag uploads the image in 0x400-byte blocks before the display setup, while a
 * non-zero flag skips straight to the display setup. The width and height size the block grid
 * in 16-pixel units.
 *
 * @param pTag The tag entry receiving the packet.
 * @param pImage The decoded image bytes.
 * @param nField The field flag.
 * @param nWidth The image width.
 * @param nHeight The image height.
 * @ghidraAddress 0x005d2ab8
 */
void setImageTag(void *pTag, void *pImage, int nField, int nWidth, int nHeight);

#ifdef __cplusplus
}
#endif

#endif
