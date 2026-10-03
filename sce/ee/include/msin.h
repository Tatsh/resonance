#ifndef MSIN_H
#define MSIN_H

#include <csl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** MIDI stream input module of the Component Sound Library. */

/**
 * Validate a module context.
 *
 * @param ctx The module context.
 * @return 0, or -1 when the context does not have a second buffer group or a stream buffer.
 * @ghidraAddress NTSC-U/C: 0x005e4570
 * @ghidraAddress PAL: 0x00626730
 */
int sceMSIn_Init(sceCslCtx *ctx);

/**
 * Queue a two-byte or three-byte channel message by its status.
 *
 * @param ctx The module context.
 * @param port The input port, an index into the second buffer group.
 * @param msg The message, with the status in the least significant byte.
 * @return 0, or -1 for a status the module does not accept, a null context, a port out of range,
 * or a full stream buffer.
 * @ghidraAddress NTSC-U/C: 0x005e4698
 * @ghidraAddress PAL: 0x00626858
 */
int sceMSIn_PutMsg(sceCslCtx *ctx, unsigned int port, unsigned int msg);

#ifdef __cplusplus
}
#endif

#endif
