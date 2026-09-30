#ifndef LIBCCONSOLE_H
#define LIBCCONSOLE_H

// The console device of the original C library. The device moves standard input and output over
// the DECI2 TTY protocol. The routines have no published names and no Sony header. They are
// defined in sce/ee/src/tty.c, and the C library's standard descriptors route to them there.
// This file is build support for the open-source SDK, not part of the reconstructed source.

#ifdef __cplusplus
extern "C" {
#endif

// Reads a line from the console into pBuffer, for standard input only. Returns the number of bytes
// read, or -1 for any other handle.
int LibcConsoleRead(int nFile, void *pBuffer, int nLength);

// Writes pBuffer to the console, for standard output and standard error only. Returns the number
// of bytes consumed, or -1 for any other handle.
int LibcConsoleWrite(int nFile, const void *pBuffer, int nLength);

// Always returns -1.
int LibcConsoleClose(int nFile);

// Always returns -1.
int LibcConsoleLseek(int nFile, int nOffset, int nOrigin);

// Always returns 1.
int LibcConsoleIsatty(int nFile);

// Marks the console closed. The next read or write opens the DECI2 TTY again.
void LibcConsoleReset(void);

#ifdef __cplusplus
}
#endif

#endif
