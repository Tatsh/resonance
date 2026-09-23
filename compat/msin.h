#ifndef MSIN_H
#define MSIN_H

// Sony's MIDI stream input module of the Component Sound Library, as the official SDK's msin.h
// declared it. The open-source ps2sdk has no counterpart, so this header declares the two entry
// points the game calls. The library itself is linked from the SDK and is not reconstructed. This
// file is build support and is not part of the reconstructed source.

#include <csl.h>

#ifdef __cplusplus
extern "C" {
#endif

// 0x005e4570. Returns 0, or -1 when the context lacks a second buffer group or a stream buffer.
int sceMSIn_Init(sceCslCtx *ctx);

// 0x005e4698. Queues a two- or three-byte channel message by its status; returns -1 for a status
// the module does not accept.
int sceMSIn_PutMsg(sceCslCtx *ctx, unsigned int port, unsigned int msg);

#ifdef __cplusplus
}
#endif

#endif
