#pragma once

// CodeQL assumes extended floating-point types that PS2 GCC does not provide.
// Match PS2 GCC capabilities before parsing libstdc++ headers.
#ifdef __mips__
#undef __STDCPP_BFLOAT16_T__
#undef __STDCPP_FLOAT16_T__
#undef __STDCPP_FLOAT32_T__
#undef __STDCPP_FLOAT128_T__
#endif
