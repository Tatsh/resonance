#ifndef LIBPAD_H
#define LIBPAD_H

// Build support for the open-source SDK. ps2sdk's pad library names its entry points differently
// from Sony's libpad. The entry point the reconstruction calls is declared here with the signature
// the shipped program's body and call site prove. Nothing here is reconstructed source.

#ifdef __cplusplus
extern "C" {
#endif

// Copies the latest report of the pad at nPort and nSlot into pData and returns a positive value
// when a report was copied. Bytes 2 and 3 of a report are the button bits, active low.
int scePadRead(int nPort, int nSlot, unsigned char *pData);

// Starts the pad library. PadRecord::Open() passes 0, once per run.
int scePadInit(int nMode);

// Opens the pad at nPort and nSlot with pDmaArea as its 256-byte, 64-byte-aligned DMA area.
int scePadPortOpen(int nPort, int nSlot, void *pDmaArea);

// Closes the pad at nPort and nSlot.
int scePadPortClose(int nPort, int nSlot);

// Reports the pad's state. 0 is disconnected, 5 is busy, 6 is stable, and 99 is closed.
int scePadGetState(int nPort, int nSlot);

// Sends the six actuator bytes in pData to the pad at nPort and nSlot.
int scePadSetActDirect(int nPort, int nSlot, const unsigned char *pData);

#ifdef __cplusplus
}
#endif

#endif
