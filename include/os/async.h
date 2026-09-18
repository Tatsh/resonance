#pragma once

/**
 * Bring up the asynchronous file-I/O layer.
 *
 * Allocates the request ring (512 requests) and resets the queue bookkeeping.
 *
 * @ghidraAddress 0x0045f000
 */
void InitAsync();
