#ifndef APP_CUTSCENE_H
#define APP_CUTSCENE_H

#ifdef __cplusplus
extern "C" {
#endif

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
 * @ghidraAddress 0x00511010
 */
void play_cutscene(const char *name, int with_audio);

#ifdef __cplusplus
}
#endif

#endif
