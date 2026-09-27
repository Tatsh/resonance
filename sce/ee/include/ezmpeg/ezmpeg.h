#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// The reconstructed Sony ezmpeg sample units. Each unit header declares what its translation unit
// provides beyond <ezmpeg.h>. Including this header gains every unit with a single include.

#include "ezmpeg/audiodec.h"
#include "ezmpeg/disp.h"
#include "ezmpeg/readbuf.h"
#include "ezmpeg/strfile.h"
#include "ezmpeg/vibuf.h"
#include "ezmpeg/videodec.h"
#include "ezmpeg/vobuf.h"

#ifdef __cplusplus
}
#endif
