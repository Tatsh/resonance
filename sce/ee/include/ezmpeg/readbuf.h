#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// The reader ring buffer of Sony's ezmpeg sample, readbuf.c. The file reader fills the buffer
// through the put entry points while the demultiplexer drains it through the get entry points.
// The structure and the six entry points are declared in <ezmpeg.h>, which this header includes
// so that consumers of the unit gain the declarations with a single include.

#include <ezmpeg.h>

#ifdef __cplusplus
}
#endif
