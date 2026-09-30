#ifndef CSL_H
#define CSL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Component sound library stream types. The layouts follow the stabs the shipped EZMIDI.IRX
 * records.
 */

/** Semaphore and buffer of one stream. */
typedef struct {
    int sema;   /*!< Semaphore guarding the buffer. */
    void *buff; /*!< Buffer. */
} sceCslBuffCtx;

/** A group of stream buffers. */
typedef struct {
    int buffNum;            /*!< Buffers in the group. */
    sceCslBuffCtx *buffCtx; /*!< The buffers. */
} sceCslBuffGrp;

/** A module context. */
typedef struct {
    int buffGrpNum;         /*!< Buffer groups. */
    sceCslBuffGrp *buffGrp; /*!< The buffer groups. */
    void *conf;             /*!< Module configuration. */
    void *callBack;         /*!< Module callback. */
    char **extmod;          /*!< Extension modules. */
} sceCslCtx;

/** A MIDI stream buffer. */
typedef struct {
    unsigned int buffsize;  /*!< Byte size of the buffer, header included. */
    unsigned int validsize; /*!< Bytes of MIDI data in #data. */
    unsigned char data[];   /*!< MIDI data. */
} sceCslMidiStream;

#ifdef __cplusplus
}
#endif

#endif
