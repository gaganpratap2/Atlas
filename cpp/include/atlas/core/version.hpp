#pragma once    // It tells the compiler to include this header file only once within a single translation unit.

namespace atlas {   // A namespace organizes related variables, functions, and classes.

inline constexpr int version_major = 0;   // constexpr tells the compiler that this variable can be evaluated at compile time.
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

inline constexpr const char* version = "0.1.0";

} // namespace atlas