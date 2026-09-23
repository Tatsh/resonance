#ifndef LIBDEV_H
#define LIBDEV_H

// Build support for the open-source SDK. Sony's libdev is not part of ps2sdk. The entry points
// the reconstruction calls are declared here with the signatures the shipped program's bodies and
// call sites prove. Nothing here is reconstructed source.

#ifdef __cplusplus
extern "C" {
#endif

// Writes 1 to VIF0_FBRST and 6 to VIF0_ERR.
void sceDevVif0Reset(void);

// Sets the VU0 reset bit through the COP2 control register.
void sceDevVu0Reset(void);

// Clears the four console slots.
void sceDevConsInit(void);

// Opens a console of nColumns by nRows character cells at a GS primitive position and returns its
// handle, or 0 when no slot is free.
int sceDevConsOpen(unsigned int nGsX, unsigned int nGsY, unsigned int nColumns, unsigned int nRows);

// Fills every cell of a console with a space of attribute 7.
void sceDevConsClear(int nConsole);

#ifdef __cplusplus
}
#endif

#endif
