#pragma once

// The shipped game is PlayStation 2 only. FREQ_PLATFORM_PS2 restricts the code that drives EE and
// IOP hardware, allowing a PC port to substitute its own implementation.
#ifndef FREQ_PLATFORM_PS2
#define FREQ_PLATFORM_PS2 1
#endif
