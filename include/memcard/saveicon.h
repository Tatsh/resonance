#pragma once

/**
 * Load the memory-card save icon.
 *
 * The routine opens `mc/freq1.icn` for reading, reads 0x9088 bytes into the buffer at
 * `0x008889a8`, and closes the file. Recovery has barely started, and this declaration exists to
 * satisfy the reference from Application::Run().
 *
 * @ghidraAddress 0x00177570
 */
void LoadSaveIcon();
