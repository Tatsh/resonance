#ifndef LIBCCONSOLE_H
#define LIBCCONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Console device of the C library, moving standard input and output over the DECI2 TTY protocol.
 *
 * The routines have no published names. The C library's standard descriptors route to them.
 */

/**
 * Read a line from the console, for standard input only.
 *
 * @param nFile The descriptor.
 * @param pBuffer Receives the line.
 * @param nLength Size of pBuffer in bytes.
 * @return The number of bytes read, or -1 for any other descriptor.
 * @ghidraAddress NTSC-U/C: 0x00596500
 * @ghidraAddress PAL: 0x005d9908
 */
int LibcConsoleRead(int nFile, void *pBuffer, int nLength);

/**
 * Write to the console, for standard output and standard error only.
 *
 * @param nFile The descriptor.
 * @param pBuffer The bytes to write.
 * @param nLength Size of pBuffer in bytes.
 * @return The number of bytes consumed, or -1 for any other descriptor.
 * @ghidraAddress NTSC-U/C: 0x00596480
 * @ghidraAddress PAL: 0x005d9888
 */
int LibcConsoleWrite(int nFile, const void *pBuffer, int nLength);

/**
 * Close a console descriptor.
 *
 * @param nFile The descriptor.
 * @return Always -1.
 * @ghidraAddress NTSC-U/C: 0x005965a0
 * @ghidraAddress PAL: 0x005d99a8
 */
int LibcConsoleClose(int nFile);

/**
 * Seek a console descriptor.
 *
 * @param nFile The descriptor.
 * @param nOffset The offset.
 * @param nOrigin The origin of nOffset.
 * @return Always -1.
 * @ghidraAddress NTSC-U/C: 0x005965b0
 * @ghidraAddress PAL: 0x005d99b8
 */
int LibcConsoleLseek(int nFile, int nOffset, int nOrigin);

/**
 * Report whether a descriptor is a terminal.
 *
 * @param nFile The descriptor.
 * @return Always 1.
 * @ghidraAddress NTSC-U/C: 0x00596668
 * @ghidraAddress PAL: 0x005d9a70
 */
int LibcConsoleIsatty(int nFile);

/**
 * Mark the console closed. The next read or write opens the DECI2 TTY again.
 *
 * @ghidraAddress NTSC-U/C: 0x005963d0
 * @ghidraAddress PAL: 0x005d97d8
 */
void LibcConsoleReset(void);

#ifdef __cplusplus
}
#endif

#endif
