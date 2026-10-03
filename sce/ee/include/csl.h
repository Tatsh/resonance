#ifndef CSL_H
#define CSL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Context types of the Component Sound Library: the three structures the game fills in and the
 * MIDI stream buffer the MIDI stream input module reads.
 *
 * The layouts are the ones the game's sound initialiser writes and sceMSIn_Init() validates.
 */

/** One buffer of a buffer group. */
typedef struct {
    int sema;   /*!< Semaphore guarding the buffer. */
    void *buff; /*!< The buffer. */
} sceCslBuffCtx;

/** A group of buffers. */
typedef struct {
    int buffNum;            /*!< Count of entries of buffCtx. */
    sceCslBuffCtx *buffCtx; /*!< The buffers. */
} sceCslBuffGrp;

/** A module context. */
typedef struct {
    int buffGrpNum;         /*!< Count of entries of buffGrp. */
    sceCslBuffGrp *buffGrp; /*!< The buffer groups. */
    void *conf;             /*!< Module configuration. */
    void *callBack;         /*!< Module callback. */
    char **extmod;          /*!< External module table. */
} sceCslCtx;

/** A MIDI stream buffer. */
typedef struct {
    unsigned int buffsize;  /*!< Size of the buffer in bytes, this header included. */
    unsigned int validsize; /*!< Bytes of data in use. */
    unsigned char data[];   /*!< The stream bytes. */
} sceCslMidiStream;

#ifdef __cplusplus
}
#endif

#endif
