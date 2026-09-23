#ifndef CSL_H
#define CSL_H

// Sony's Component Sound Library context types, as the official SDK's csl.h declared them. The
// open-source ps2sdk has no counterpart, so this header declares the three structures the game
// fills in and the MIDI stream buffer its input module reads. The layouts are the ones the game's
// initialiser at 0x00462290 writes and sceMSIn_Init() at 0x005e4570 validates. This file is build
// support and is not part of the reconstructed source.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int sema;
    void *buff;
} sceCslBuffCtx;

typedef struct {
    int buffNum;
    sceCslBuffCtx *buffCtx;
} sceCslBuffGrp;

typedef struct {
    int buffGrpNum;
    sceCslBuffGrp *buffGrp;
    void *conf;
    void *callBack;
    char **extmod;
} sceCslCtx;

typedef struct {
    unsigned int buffsize;
    unsigned int validsize;
    unsigned char data[];
} sceCslMidiStream;

#ifdef __cplusplus
}
#endif

#endif
