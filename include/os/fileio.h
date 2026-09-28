#pragma once

/** Request that reports whether an asynchronous operation on a file is still running. */
constexpr int kSceFsExecuting = 1;

extern "C" {

/**
 * Read from the debug console, for standard input only.
 *
 * Toolchain C library code with no published name. The file layer's read sends every handle that
 * is neither an ark stream nor a file-service handle here.
 *
 * @param nFile The console handle.
 * @param pBuffer The destination.
 * @param nLength The number of bytes to read.
 * @return The number of bytes read, or -1 for any handle other than standard input.
 * @ghidraAddress 0x00596500
 */
int LibcConsoleRead(int nFile, void *pBuffer, int nLength);

/**
 * Write to the debug console, for standard output and standard error only.
 *
 * Toolchain C library code with no published name.
 *
 * @param nFile The console handle.
 * @param pBuffer The source.
 * @param nLength The number of bytes to write.
 * @return The number of bytes written, or -1 for any other handle.
 * @ghidraAddress 0x00596480
 */
int LibcConsoleWrite(int nFile, const void *pBuffer, int nLength);

/**
 * Close a console handle. The call always reports -1.
 *
 * Toolchain C library code, linked as shipped, with no published name.
 *
 * @param nFile The console handle.
 * @return -1.
 * @ghidraAddress 0x005965a0
 */
int LibcConsoleClose(int nFile);

/**
 * Seek a console handle. The call always reports -1.
 *
 * Toolchain C library code, linked as shipped, with no published name.
 *
 * @param nFile The console handle.
 * @param nOffset The offset.
 * @param nOrigin The origin.
 * @return -1.
 * @ghidraAddress 0x005965b0
 */
int LibcConsoleLseek(int nFile, int nOffset, int nOrigin);

/**
 * Report that a console handle is a terminal.
 *
 * Toolchain C library code, linked as shipped, with no published name.
 *
 * @param nFile The console handle.
 * @return 1.
 * @ghidraAddress 0x00596668
 */
int LibcConsoleIsatty(int nFile);

} // extern "C"
