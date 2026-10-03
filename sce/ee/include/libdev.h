#ifndef LIBDEV_H
#define LIBDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/** Device library: VIF0 and VU0 resets and the debug text consoles. */

/**
 * Write 1 to VIF0_FBRST and 6 to VIF0_ERR.
 *
 * @ghidraAddress NTSC-U/C: 0x0062bfd0
 * @ghidraAddress PAL: 0x0066cb60
 */
void sceDevVif0Reset(void);

/**
 * Set the VU0 reset bit through the COP2 control register.
 *
 * @ghidraAddress NTSC-U/C: 0x0061dc90
 * @ghidraAddress PAL: 0x0065e820
 */
void sceDevVu0Reset(void);

/**
 * Clear the four console slots.
 *
 * @ghidraAddress NTSC-U/C: 0x00622710
 * @ghidraAddress PAL: 0x00663120
 */
void sceDevConsInit(void);

/**
 * Open a console of nColumns by nRows character cells at a GS primitive position.
 *
 * @param nGsX Horizontal GS primitive coordinate.
 * @param nGsY Vertical GS primitive coordinate.
 * @param nColumns Width in character cells.
 * @param nRows Height in character cells.
 * @return The console handle, or 0 when no slot is free.
 * @ghidraAddress NTSC-U/C: 0x00622748
 * @ghidraAddress PAL: 0x00663158
 */
int sceDevConsOpen(unsigned int nGsX, unsigned int nGsY, unsigned int nColumns, unsigned int nRows);

/**
 * Fill every cell of a console with a space of attribute 7.
 *
 * @param nConsole The console handle.
 * @ghidraAddress NTSC-U/C: 0x00622c98
 * @ghidraAddress PAL: 0x006636a8
 */
void sceDevConsClear(int nConsole);

/**
 * Free a console's buffer to the console heap and clear its size and buffer fields.
 *
 * @param nConsole The console handle.
 * @ghidraAddress NTSC-U/C: 0x00622848
 * @ghidraAddress PAL: 0x00663258
 */
void sceDevConsClose(int nConsole);

#ifdef __cplusplus
}
#endif

#endif
