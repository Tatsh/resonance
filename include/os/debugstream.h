#pragma once

#include <iostream.h>

/**
 * Stream that start-up diagnostics and failure text are written to.
 *
 * The game predates namespaced iostreams, so the type is the global `ostream` of the g++ 2.x
 * library.
 *
 * @ghidraAddress 0x0071a7c8
 */
extern ostream g_debugStream;
