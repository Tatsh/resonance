#ifndef SYSCLIB_H
#define SYSCLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Memory routines of the resident sysclib library. */

/**
 * Copy memory between regions that do not overlap.
 *
 * @param dest Destination.
 * @param src Source.
 * @param n Byte count.
 * @return @p dest.
 */
void *memcpy(void *dest, const void *src, size_t n);

#ifdef __cplusplus
}
#endif

#endif
