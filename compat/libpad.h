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

#ifdef __cplusplus
}
#endif

#endif
