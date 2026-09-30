#ifndef SIF_H
#define SIF_H

#ifdef __cplusplus
extern "C" {
#endif

/** SIF DMA services of the resident sifman library. */

/** Initialise the SIF DMA interface. */
void sceSifInit(void);

/**
 * Report whether the SIF DMA interface is initialised.
 *
 * @return Nonzero once sceSifInit() has run.
 */
int sceSifCheckInit(void);

#ifdef __cplusplus
}
#endif

#endif
