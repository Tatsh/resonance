#pragma once

/** Request that reports whether an asynchronous operation on a file is still running. */
constexpr int kSceFsExecuting = 1;

/**
 * Send a control request for an open file to the IOP file server.
 *
 * The routine belongs to the PlayStation 2 SDK. The title is inferred from the body, which sends
 * remote procedure 5 of the file server (the ioctl slot) with a 0x400-byte argument block.
 *
 * @param nFile The file descriptor.
 * @param nRequest The request.
 * @param pArg The request argument, or the result for kSceFsExecuting.
 * @return The file server's result, or a negative error.
 * @ghidraAddress 0x0056b870
 */
int sceIoctl(int nFile, int nRequest, void *pArg);

extern "C" {

/**
 * Read from the debug console.
 *
 * Toolchain C library code, linked as shipped, with no published name. The file layer's read
 * sends every handle that is neither an ark stream nor a file-service handle here.
 *
 * @param nFile The console handle.
 * @param pBuffer The destination.
 * @param nLength The number of bytes to read.
 * @return The number of bytes read.
 * @ghidraAddress 0x00596500
 */
int LibcConsoleRead(int nFile, void *pBuffer, int nLength);

/**
 * Write to the debug console, for standard output and standard error only.
 *
 * Toolchain C library code, linked as shipped, with no published name.
 *
 * @param nFile The console handle.
 * @param pBuffer The source.
 * @param nLength The number of bytes to write.
 * @return The number of bytes written.
 * @ghidraAddress 0x00596480
 */
int LibcConsoleWrite(int nFile, const void *pBuffer, int nLength);

/**
 * Close a console handle, which always reports -1.
 *
 * Toolchain C library code, linked as shipped, with no published name.
 *
 * @param nFile The console handle.
 * @return -1.
 * @ghidraAddress 0x005965a0
 */
int LibcConsoleClose(int nFile);

/**
 * Seek a console handle, which always reports -1.
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
 * Report that a console handle is a terminal, which it always is.
 *
 * Toolchain C library code, linked as shipped, with no published name.
 *
 * @param nFile The console handle.
 * @return 1.
 * @ghidraAddress 0x00596668
 */
int LibcConsoleIsatty(int nFile);

} // extern "C"
