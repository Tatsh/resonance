#ifndef LIBPAD_H
#define LIBPAD_H

// The game was built against Sony's official SDK, whose pad API uses the scePad prefix. The
// open-source ps2sdk declares most entry points under pad* with no aliases. This shim forwards
// those names rather than redeclaring them, and reconstructed source retains the official prefix
// the original wrote. Three entry points disagree in type with their counterparts and stay
// declared below for reconstruction.
//
// Add ps2sdk/ee/rpc/pad/include to the include path after this directory. The include below
// then resolves to the real header. This file is build support and is not part of the reconstructed
// source.

#include_next <libpad.h>

#define scePadInit padInit
#define scePadPortOpen padPortOpen
#define scePadPortClose padPortClose
#define scePadGetState padGetState
#define scePadInfoMode padInfoMode
#define scePadSetMainMode padSetMainMode
#define scePadGetReqState padGetReqState
#define scePadInfoAct padInfoAct
#define scePadInfoPressMode padInfoPressMode
#define scePadEnterPressMode padEnterPressMode

#ifdef __cplusplus
extern "C" {
#endif

// Copies the latest report of the pad at nPort and nSlot into pData and returns a positive value
// when a report was copied. Bytes 2 and 3 of a report are the button bits, active low.
int scePadRead(int nPort, int nSlot, unsigned char *pData);

// Sends the six actuator bytes in pData to the pad at nPort and nSlot.
int scePadSetActDirect(int nPort, int nSlot, const unsigned char *pData);

// Assigns each of the six actuators a byte of the direct actuator buffer.
int scePadSetActAlign(int nPort, int nSlot, const unsigned char *pAlign);

#ifdef __cplusplus
}
#endif

#endif
