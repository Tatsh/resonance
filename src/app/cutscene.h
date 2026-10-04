#ifndef APP_CUTSCENE_H
#define APP_CUTSCENE_H

#include <ezmpeg.h>
#include <libgraph.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The decoded frame queue. The decoder stages pictures into it, and the display handlers take
 * entries from it and release them through it.
 *
 * The queue starts a cache line of its own. Its cached updates then never share a line with the
 * display buffer placed before it.
 *
 * @ghidraAddress NTSC-U/C: 0x0070cae8
 * @ghidraAddress PAL: 0x007509d8
 */
extern VoBuf voBuf;

/**
 * The display double buffer. The end-of-image handler swaps its halves as fields are shown.
 *
 * The vertical blank handler writes the draw environments through the uncached segment while the
 * GIF channel reads the buffer. The buffer therefore starts on a cache line.
 *
 * @ghidraAddress NTSC-U/C: 0x0070cb00
 * @ghidraAddress PAL: 0x007509f0
 */
extern sceGsDBuff db;

/**
 * The video decoder instance. The decoder and stream callbacks address it directly, because the
 * sample registers them with a null context.
 *
 * @ghidraAddress NTSC-U/C: 0x0070cd78
 * @ghidraAddress PAL: 0x00750c68
 */
extern VideoDec videoDec;

/**
 * The audio decoder instance. The audio callback addresses it directly.
 *
 * @ghidraAddress NTSC-U/C: 0x0070ce30
 * @ghidraAddress PAL: 0x00750d20
 */
extern AudioDec audioDec;

/**
 * Play a PSS movie to the end, or until the player skips it.
 *
 * The unit is Sony's ezmpeg sample driver adapted by the game. It is a C file, as its `__FILE__`
 * literal "cutscene.c" records. A name with a device prefix is first opened as a host file after
 * the colon, and the routine returns at once when that fails. Otherwise it allocates one 0x57bc00
 * byte block for every decoder buffer, resets the GS, plays the stream, and frees the block.
 *
 * Pressing and releasing Cross or Start after the eleventh decoded frame aborts the playback.
 *
 * The name is inferred. The front end's Sony screen and one other caller both pass a with-audio
 * flag of 1.
 *
 * @param name The stream to play, with its device prefix.
 * @param with_audio Non-zero to decode and play the PCM stream alongside the video.
 * @ghidraAddress NTSC-U/C: 0x00511010
 * @ghidraAddress PAL: 0x00551288
 */
void play_cutscene(const char *name, int with_audio);

#ifdef __cplusplus
}
#endif

#endif
