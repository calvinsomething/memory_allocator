#pragma once

#ifdef NDEBUG

inline void log_bits(const char *mem, size_t n)
{
}

#define DEBUG_OUT(ARGS)

#else

#include <cstddef>
#include <iostream>
#include <sstream>

inline void log_bits(const char *mem, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < 8; ++j)
        {
            std::cout << !!(mem[i] & (1 << (7 - j)));
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

#define DEBUG_OUT(ARGS)                                                                                                \
    {                                                                                                                  \
        std::stringstream ss;                                                                                          \
        ss << ARGS;                                                                                                    \
        std::cout << ss.str();                                                                                         \
    }

#endif
