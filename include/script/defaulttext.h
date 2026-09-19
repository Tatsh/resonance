#pragma once

/**
 * Text that stands in for a null string.
 *
 * Every reader of `HxStr::mStr` in the image substitutes this pointer when the buffer is null, so
 * an empty HxStr arrives at a C interface as this text rather than as a null pointer. The value is
 * the empty string, at `0x008211b8`, which sits two words ahead of the `HxStr.cpp` allocator tag
 * and the `mStr != ` assert text. The global therefore belongs to HxStr rather than to the script
 * layer, and it is declared here only because the script layer and the PyCXX binding are its
 * heaviest readers. It should move to `os/hxstr.h`.
 *
 * @ghidraAddress 0x006fbd10
 */
extern const char *g_pszDefaultText;
