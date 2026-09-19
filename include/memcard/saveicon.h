#pragma once

/** Bytes LoadSaveIcon() requests, which is the size of the buffer it fills. */
constexpr int kSaveIconBufferSize = 0x9088;

/**
 * Load the memory-card save icon.
 *
 * The routine builds the path from the host prefix and `mc/freq1.icn`, opens it through the C
 * library, reads kSaveIconBufferSize bytes into g_abSaveIcon, records the transferred count in
 * g_nSaveIconLength, and closes the file. A path that fails to open skips the read, which is what
 * makes the length zero on a disc with no icon.
 *
 * `SaveFileMCT` writes g_abSaveIcon to the card as `freq1.ico`.
 *
 * @ghidraAddress 0x00177570
 */
void LoadSaveIcon();

/**
 * The icon mesh, as `mc/freq1.icn` stores it.
 *
 * @ghidraAddress 0x008889a8
 */
extern unsigned char g_abSaveIcon[kSaveIconBufferSize];

/**
 * Bytes LoadSaveIcon() transferred into g_abSaveIcon, and the length written to the card.
 *
 * Zero until LoadSaveIcon() has run.
 *
 * @ghidraAddress 0x0067bfd8
 */
extern int g_nSaveIconLength;
