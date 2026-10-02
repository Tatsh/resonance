#pragma once

/**
 * Bring up the GS for the debug console, independently of GfxDevice.
 *
 * Resets VIF0, VU0, the GS path, and the DMA controller, sets the channel mask of the DMA
 * settings to the VIF1 bit, enables tag transfer on the VIF1 channel, and resets the GS to
 * interlaced NTSC in frame mode. It then fills a 640 by 224 SDK double buffer with a clear colour
 * of (0x40, 0x40, 0x80), sends a default alpha environment through a VIF1 DIRECT packet built in
 * scratchpad memory, and waits until sceGsSyncV() reports field 1. InitDebugConsole() is the only
 * caller.
 *
 * The name is inferred.
 *
 * @return Always 1, which the caller discards.
 * @ghidraAddress 0x005e5d08
 */
int InitDebugGs();

/**
 * Create the debug console of 75 columns by 30 rows and clear it, without touching the GS.
 *
 * This is InitDebugConsole() without its call to InitDebugGs(). No caller survives in the shipped
 * program. The name is inferred.
 *
 * @ghidraAddress 0x005e5ed8
 */
void OpenDebugConsole();

/**
 * Clear every cell of the debug console.
 *
 * No caller survives in the shipped program. The name is inferred.
 *
 * @ghidraAddress 0x005e5f60
 */
void ClearDebugConsole();
