#ifndef EZMPEG_AUDIODEC_H
#define EZMPEG_AUDIODEC_H

#ifdef __cplusplus
extern "C" {
#endif

// The audio decoder of Sony's ezmpeg sample, audiodec.c. The unit stages decoded audio in an
// Emotion Engine buffer and moves it to the Input Output Processor through the sound driver and
// the SIF direct memory access channel. The structure and the six entry points are declared in
// <ezmpeg.h>, which this header includes so that consumers of the unit gain the declarations
// with a single include.

#include <ezmpeg.h>

#ifdef __cplusplus
}
#endif

#endif
