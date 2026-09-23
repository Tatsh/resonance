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
