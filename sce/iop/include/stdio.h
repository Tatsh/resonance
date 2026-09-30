#ifndef STDIO_H
#define STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

/** Console output of the resident stdio library. */

/**
 * Print formatted text to the console.
 *
 * @param format Format string.
 * @return The number of characters printed.
 */
int printf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
